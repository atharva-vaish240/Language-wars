# The Geospatial Traffic Aggregator

### **The Scenario**
Your team has been granted access to the raw telemetry feed from every highway speed camera and toll booth in the country. The dataset is massive—upwards of 1 Billion rows (approx. 40 GB) of unstructured text. 

Your task is to parse this massive stream of data, aggregate the statistics for every unique highway, and output the results. The catch? You need to do it faster than anyone else.

This is an **unrestricted language**. You may use any programming language. You may use memory-mapped files (`mmap`), custom thread pools, lock-free hash maps, SIMD instructions, or custom float parsers. 

### **The Input Format**
The judge will provide a massive `.txt` file. Every line represents a single vehicle detection, formatted exactly as follows, delimited by a semicolon (`;`):

`<Timestamp>;<Highway_ID>;<Vehicle_ID>;<Speed>`

* **Timestamp:** A 10-digit UNIX epoch integer.
* **Highway_ID:** A string of varying length (1 to 64 bytes). **Warning:** This field is NOT limited to standard ASCII. It may contain complex multibyte UTF-8 characters (Kanji, Cyrillic, Emojis, European accents). 
* **Vehicle_ID:** A string formatted as `V_<integer>`.
* **Speed:** A positive floating-point number representing km/h.

**Example Input Data:**
```text
1713360300;HWY_101;V_99281;115.4
1713360301;ДОРОГА_7;V_1022;88.2
1713360301;HWY_101;V_4421;140.9
1713360305;ROUTE_66_🛣️;V_881;65.0
```

### **The Task & Output Format**
Your program must read the input file and calculate the following for **every unique Highway_ID**:
1.  The **Minimum** speed recorded.
2.  The **Maximum** speed recorded.
3.  The **Average** speed across all vehicles.
4.  The **Vehicle_ID** of the single fastest car. *(Note: If there is a tie for the exact maximum speed, output the Vehicle_ID that was encountered first in the file).*

**Formatting Rules:**
* You must output exactly one line per unique highway.
* The lines **must be sorted alphabetically** (strictly by byte-value/lexicographical order of the `Highway_ID`).
* All floating-point numbers (Min, Max, Avg) must be formatted to exactly **one decimal place**.

**Expected Output Format:**
```text
HWY_101: Min=115.4, Max=140.9, Avg=128.1, Fastest=V_4421
ROUTE_66_🛣️: Min=65.0, Max=65.0, Avg=65.0, Fastest=V_881
ДОРОГА_7: Min=88.2, Max=88.2, Avg=88.2, Fastest=V_1022
```

---

### **Execution & CLI Instructions**
Your solution must be compiled (or packaged) into an executable format. The judge system will invoke your program via the command line, passing exactly two positional arguments:
1.  The absolute path to the input data file.
2.  The absolute path where your program must write its output file.

**Examples of how the judge will call your code:**
* **C/C++/Rust/Go:** `./solution /data/measurements.txt /output/results.txt`
* **Python:** `python3 solution.py /data/measurements.txt /output/results.txt`
* **Java:** `java -jar solution.jar /data/measurements.txt /output/results.txt`
* **Node.js:** `node solution.js /data/measurements.txt /output/results.txt`

> [!NOTE] 
> In the root of the repository which you will submit, there should not be any other file starting with "solution" except the file which is the solution.

Your program **must not** print the results to standard output (`stdout`). It must write the final text to the file path specified in the second argument.

---

### **Constraints & Benchmarking**
* **Execution Environment:** Your code will be run on a dedicated bare-metal server with 96 CPU Cores and 192 GB of RAM or locally on the judge's device with 8c/16t and 16 GB of RAM.
* **Cache Clearing:** Between every submission, the judge will drop the OS page cache (`echo 3 > /proc/sys/vm/drop_caches`) to ensure cold disk reads for everyone.
* **Validation Tolerance:** Because floating-point accumulation can drift slightly depending on architecture (FPU vs. SIMD), the grader allows a mathematical drift tolerance of `±0.15` for the Min, Max, and Avg values.
