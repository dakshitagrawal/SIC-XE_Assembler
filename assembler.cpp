#include<iostream>
#include<cstdlib>
#include<fstream>
#include<string>
#include<cstdio>
#include<map>
#include<climits>

using namespace std;

#include "Pass2.cpp"

int main() {
	string filename;

	cout << "Enter the file that you want to pass through the assembler:" << endl;

	// get input file name.
	getline(cin, filename);
    fin1.open(filename);

    // check if file exists.  If so, run assembler pass1 and pass2, else notify user of missing file.
    if (fin1.good()) {

    	// run Pass 1.
    	runPass1();

    	// run Pass 2.
    	runPass2();

    	// close input file.
    	fin1.close();
    } else {
    	cout << "Please write text file name with extention.  Make sure input file is in same directory as 'assembler.cpp'." << endl;
    }
} 