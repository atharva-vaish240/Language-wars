#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

// map<string, set<pair<int, float>>>

// Split the string using the delimeter
void adv_tokenizer(string s, char del)
{
    stringstream ss(s);
    string word;
    while (!ss.eof()) {
        getline(ss, word, del);
        cout << word << endl;
    }
    cout<<"\n";
}

int main() {
    // Create a text string, which is used to output the text file
    string myText;

    // Read from the text file
    ifstream MyReadFile("data.txt");

    // Use a while loop together with the getline() function to read the file line by line
    int i=0; // rm after prototyping
    while (getline (MyReadFile, myText) && i<10) {
        // Output the text from the file
        cout << myText << "\n";
        adv_tokenizer(myText, ';');
        i++; // rm this after prototyping
    }

    // Close the file
    MyReadFile.close();
}