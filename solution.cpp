#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
using namespace std;

// Split the string using the delimeter
vector<string> adv_tokenizer(string s, char del)
{
    stringstream ss(s);
    string word;
    vector<string> v;
    while (!ss.eof()) {
        getline(ss, word, del);
        // cout << word << endl;
        v.push_back(word);
    }
    
    return v;
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
        vector<string> v = adv_tokenizer(myText, ';');
        for(string it: v){
            cout << it << endl;
        }
        cout<<"\n";
        i++; // rm this after prototyping
    }

    // Close the file
    MyReadFile.close();
}