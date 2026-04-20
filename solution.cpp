#include <iostream>
#include <fstream>
#include <string_view>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include <iomanip>
#include <algorithm>
#include <limits>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

using namespace std;

// Class to handle memory mapping cross-platform
class MemoryMappedFile {
public:
    const char* data = nullptr;
    size_t size = 0;

#ifdef _WIN32
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hMap = NULL;
#else
    int fd = -1;
#endif

    bool open(const std::string& path) {
#ifdef _WIN32
        hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) return false;
        LARGE_INTEGER li;
        GetFileSizeEx(hFile, &li);
        size = li.QuadPart;
        hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        if (hMap == NULL) return false;
        data = (const char*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
        return data != nullptr;
#else
        fd = ::open(path.c_str(), O_RDONLY);
        if (fd < 0) return false;
        struct stat st;
        if (fstat(fd, &st) < 0) return false;
        size = st.st_size;
        data = (const char*)mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
        return data != MAP_FAILED;
#endif
    }

    ~MemoryMappedFile() {
#ifdef _WIN32
        if (data) UnmapViewOfFile(data);
        if (hMap) CloseHandle(hMap);
        if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
#else
        if (data && data != MAP_FAILED) munmap((void*)data, size);
        if (fd >= 0) ::close(fd);
#endif
    }
};

struct HighwayStats {
    double min_speed = 1e12;
    double max_speed = -1.0;
    double total_speed = 0.0;
    long long count = 0;
    std::string_view fastest_vehicle;
    size_t fastest_offset = std::numeric_limits<size_t>::max();
};

struct FastMapNode {
    std::string_view key;
    HighwayStats value;
    bool empty() const { return key.data() == nullptr; }
};

// Fast custom open-addressing Hash Map to avoid std::unordered_map allocation overhead
class FastMap {
public:
    std::vector<FastMapNode> table;
    size_t count = 0;

    FastMap(size_t capacity = 16384) {
        table.resize(capacity);
    }

    HighwayStats& operator[](std::string_view key) {
        size_t hash = 14695981039346656037ull;
        for (char c : key) {
            hash ^= static_cast<unsigned char>(c);
            hash *= 1099511628211ull;
        }

        size_t mask = table.size() - 1;
        size_t index = hash & mask;

        while (!table[index].empty()) {
            if (table[index].key == key) {
                return table[index].value;
            }
            index = (index + 1) & mask;
        }

        if (count >= table.size() / 2) {
            rehash();
            mask = table.size() - 1;
            index = hash & mask;
            while (!table[index].empty()) {
                index = (index + 1) & mask;
            }
        }

        table[index].key = key;
        count++;
        return table[index].value;
    }

    void rehash() {
        std::vector<FastMapNode> new_table(table.size() * 2);
        size_t mask = new_table.size() - 1;
        for (const auto& node : table) {
            if (!node.empty()) {
                size_t hash = 14695981039346656037ull;
                for (char c : node.key) {
                    hash ^= static_cast<unsigned char>(c);
                    hash *= 1099511628211ull;
                }
                size_t index = hash & mask;
                while (!new_table[index].empty()) {
                    index = (index + 1) & mask;
                }
                new_table[index] = node;
            }
        }
        table = std::move(new_table);
    }
};

// Lexicographical string view comparator comparing strictly unsigned bytes
struct StringViewCmp {
    bool operator()(std::string_view a, std::string_view b) const {
        size_t min_len = std::min(a.size(), b.size());
        for (size_t i = 0; i < min_len; ++i) {
            unsigned char ca = static_cast<unsigned char>(a[i]);
            unsigned char cb = static_cast<unsigned char>(b[i]);
            if (ca != cb) return ca < cb;
        }
        return a.size() < b.size();
    }
};

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>\n";
        return 1;
    }

    std::string input_file = argv[1];
    std::string output_file = argv[2];

    MemoryMappedFile mmap_file;
    if (!mmap_file.open(input_file)) {
        std::cerr << "Failed to open or memory-map input file.\n";
        return 1;
    }

    std::map<std::string_view, HighwayStats, StringViewCmp> global_map;
    std::mutex global_mtx;

    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4;

    size_t chunk_size = mmap_file.size / num_threads;
    std::vector<std::thread> threads;

    auto thread_func = [&](size_t chunk_start, size_t chunk_end) {
        FastMap local_map;
        const char* file_data = mmap_file.data;
        const char* ptr = file_data + chunk_start;
        size_t file_size = mmap_file.size;

        // Skip to beginning of next valid line
        if (chunk_start > 0) {
            while (ptr < file_data + file_size && *(ptr - 1) != '\n') {
                ptr++;
            }
        }

        while (ptr < file_data + chunk_end && ptr < file_data + file_size) {
            const char* line_start = ptr;

            while (ptr < file_data + file_size && *ptr != ';') ptr++;
            if (ptr >= file_data + file_size) break;
            ptr++;

            const char* hwy_start = ptr;
            while (ptr < file_data + file_size && *ptr != ';') ptr++;
            std::string_view hwy_id(hwy_start, ptr - hwy_start);
            ptr++;

            const char* veh_start = ptr;
            while (ptr < file_data + file_size && *ptr != ';') ptr++;
            std::string_view veh_id(veh_start, ptr - veh_start);
            ptr++;

            double speed = 0.0;
            while (ptr < file_data + file_size && *ptr >= '0' && *ptr <= '9') {
                speed = speed * 10.0 + (*ptr - '0');
                ptr++;
            }
            if (ptr < file_data + file_size && *ptr == '.') {
                ptr++;
                double frac = 0.1;
                while (ptr < file_data + file_size && *ptr >= '0' && *ptr <= '9') {
                    speed += (*ptr - '0') * frac;
                    frac *= 0.1;
                    ptr++;
                }
            }

            // Skip up to and including the newline
            while (ptr < file_data + file_size && *ptr != '\n') {
                ptr++;
            }
            if (ptr < file_data + file_size) {
                ptr++;
            }

            auto& stats = local_map[hwy_id];
            stats.count++;
            stats.total_speed += speed;
            if (speed < stats.min_speed) stats.min_speed = speed;
            
            size_t current_offset = line_start - file_data;
            if (speed > stats.max_speed) {
                stats.max_speed = speed;
                stats.fastest_vehicle = veh_id;
                stats.fastest_offset = current_offset;
            } else if (speed == stats.max_speed) {
                if (current_offset < stats.fastest_offset) {
                    stats.fastest_vehicle = veh_id;
                    stats.fastest_offset = current_offset;
                }
            }
        }

        // Merge to global map
        std::lock_guard<std::mutex> lock(global_mtx);
        for (const auto& node : local_map.table) {
            if (!node.empty()) {
                auto& stats = node.value;
                auto& global_stats = global_map[node.key];
                if (global_stats.count == 0) {
                    global_stats = stats;
                } else {
                    global_stats.count += stats.count;
                    global_stats.total_speed += stats.total_speed;
                    if (stats.min_speed < global_stats.min_speed) global_stats.min_speed = stats.min_speed;
                    if (stats.max_speed > global_stats.max_speed) {
                        global_stats.max_speed = stats.max_speed;
                        global_stats.fastest_vehicle = stats.fastest_vehicle;
                        global_stats.fastest_offset = stats.fastest_offset;
                    } else if (stats.max_speed == global_stats.max_speed) {
                        if (stats.fastest_offset < global_stats.fastest_offset) {
                            global_stats.fastest_vehicle = stats.fastest_vehicle;
                            global_stats.fastest_offset = stats.fastest_offset;
                        }
                    }
                }
            }
        }
    };

    for (unsigned int i = 0; i < num_threads; ++i) {
        size_t start = i * chunk_size;
        size_t end = (i == num_threads - 1) ? mmap_file.size : start + chunk_size;
        threads.emplace_back(thread_func, start, end);
    }

    for (auto& t : threads) {
        t.join();
    }

    std::ofstream out(output_file);
    if (!out.is_open()) {
        std::cerr << "Failed to open output file.\n";
        return 1;
    }
    
    out << std::fixed << std::setprecision(1);
    for (const auto& [hwy, stats] : global_map) {
        double avg = stats.total_speed / stats.count;
        out << hwy << ": Min=" << stats.min_speed 
            << ", Max=" << stats.max_speed 
            << ", Avg=" << avg 
            << ", Fastest=" << stats.fastest_vehicle << "\n";
    }

    return 0;
}