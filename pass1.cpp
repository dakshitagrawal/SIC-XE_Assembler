#include<iostream>
#include<cstdlib>
#include<fstream>
#include<string>
#include<cstdio>
#include<map>

using namespace std;

#include "TABLES.cpp"
#include "HEXA_DEC.cpp"

// starting address of the program
hexa startaddress = toHex(-10000);

// location counter of assembler
hexa locctr = startaddress;

// status_flag = 0 if no fault
// status_flag = 1 if there are errors
// status_flag = 2 if start opcode present
// status_flag = 3 if end opcode present
// status_flag = 4 if beginning of program
int status_flag = 0;

ifstream fin1;
ofstream fout1, error, labelout;

bool isWhiteSpace(char a) {
    if (a == ' ') {
        return true;
    } else if (a == '\t') {
        return true;
    } else {
        return false;
    }
}


// method to extract words from a given line of program

void extract(string a, string word[]) {
    for (int i = 0; i < 3; i++) {
        word[i] = "";
    }

    int count=0;

    for (int i = 0; i < a.length(); i++) {

        if (isWhiteSpace(a[i])) {
            // if character is blank space, don't consider
            continue;
        } else if (isdigit(a[i]) && count == 0) {
            // if line begins with a number, don't consider
            continue;
        } else if (a[i] == '.') {
            // denotes comment, extraction of useful components of program line done
            return;
        } else{
            // extract word till a white space or comma is encountered.
            while (!isWhiteSpace(a[i]) && a[i] != ',' && i < a.length()) {
                word[count] += a[i];
                i++;
            }
            count++;
        }
    }
}

int runPass1() {

    // create TABLES for opcodes, registers and symbols
    create();

    string s, word[3];

    // open necessary files
    fout1.open("intermediate.txt");
    labelout.open("symtab.txt");
    error.open("error.txt");

    // flag to denote if END is present in program. If 1, then present.
    int endPresent = 0;

    // read input program file if no initial errors.

    if (fin1.good()) {

        // read the whole document line by line
        while (getline(fin1, s)) {

            // set status_flag to 0 for new line
            status_flag = 0;

            // flag to increase location counter of assembler.
            int locctrADD = 0;            

            //extract the words in the line
            extract(s, word);

            // check if we have reached END of program
            if (word[0] == "END") {
                endPresent = 1;

                status_flag = 3;
                fout1 << locctr << "\t \t";
                fout1 << status_flag << "\t";
                fout1 << s << endl;

                break;
            }

            // check if starting address has been initialized. If not, set it.
            if (startaddress == toHex(-10000)) {                
                if (word[0] == "START") {
                    status_flag = 2;

                    startaddress = toHex(atoi(word[1].c_str()));
                    locctr = startaddress;

                    fout1 << locctr << "\t \t";
                    fout1 << status_flag << "\t";
                    fout1 << s << endl;

                    continue;
                } else if (word[1] == "START") {
                    status_flag = 2;

                    startaddress = toHex(atoi(word[2].c_str()));
                    locctr = startaddress;

                    // check if label exists in symbol table. If not, make new entry.

                    if (SYMTAB[word[0]].exist == 'y') {
                        error << "Duplicate Label: " + word[0] + " at address: " + locctr + "."<< endl;
                        status_flag = 1;
                    } else {
                        SYMTAB[word[0]].address = locctr;
                        SYMTAB[word[0]].exist = 'y';
                    }

                    fout1 << locctr << "\t \t";
                    fout1 << status_flag << "\t";
                    fout1 << s << endl;

                    continue;
                } else {

                    // throw error if START command not given.
                    status_flag = 4;
                    error << "START command for program not given!" << endl;
                    startaddress = toHex(0);
                    locctr = startaddress;
                }
            }
            
            // the program line read has comment in it.

            if (word[0] == "") {
                fout1 << "$" << "  ";
                fout1 << s << endl;
                continue;
            } 

            // check if extended address is used. 
            if (word[0][0] == '+') {
                locctrADD = 4;
                word[0] = word[0].substr(1, word[0].length() - 1);
            } else if (word[1][0] == '+') {
                locctrADD = 4;
                word[1] = word[1].substr(1, word[1].length() - 1);
            }


            if (OPTAB[word[0]].exist == 'y') {

                // opcode exists in OPCODE TABLE

                fout1 << locctr << "\t \t";
                fout1 << status_flag << "\t";
                fout1 << s << endl;

                if (locctrADD != 4) {
                    locctrADD = OPTAB[word[0]].format;
                }
            } else if (word[0] == "BASE") {

                // BASE assembler directive of the program has been defined. 

                SYMTAB[word[0]].address = word[1];
                SYMTAB[word[0]].exist = 'y';
            } else if (word[0] == "NOBASE") {

                // NO BASE assembler directive of the program has been defined.

                SYMTAB[word[0]].address = locctr;
                SYMTAB[word[0]].exist = 'y';
            } else {
                // Opcode is not the first word in program line.

                // check if label exists in symbol table.  If not, make new entry.
                if (SYMTAB[word[0]].exist == 'y') {
                    error << "Duplicate Label: " + word[0] + " at address: " + locctr + "."<< endl;
                    status_flag = 1;
                } else {
                    SYMTAB[word[0]].address = locctr;
                    SYMTAB[word[0]].exist = 'y';
                }
                
                // check second word of the program line.
                if (OPTAB[word[1]].exist == 'y') {
                    // opcode is present in OPCODE TABLE.

                    if (locctrADD != 4) {
                        locctrADD = OPTAB[word[1]].format;
                    }
                } else if (word[1] == "WORD") {
                    // add size of word = 3 bytes to location counter of assembler.
                    locctrADD = 3;
                } else if (word[1] == "RESW") {
                    // add size of variable = (3* (number of words)) bytes to the location counter of assembler.
                    locctrADD = atoi(word[2].c_str())*3;
                } else if (word[1] == "RESB") {
                    // add size of variable = (number of bytes) to the location counter of assembler.
                    locctrADD = atoi(word[2].c_str());
                } else if (word[1] == "BYTE") {
                    // add size of variable = (length of next word in line - (two quotation marks) - (C or X)) to
                    // location counter of assembler.

                    locctrADD = word[2].length() - 3;

                    // if X format is used, size to be added is half of the number of characters.
                    if (word[2][0] == 'X') {
                        locctrADD /= 2;
                    }
                } else {
                    // show error of opcode not existing along with the address of program line.
                    error << "Opcode does not exist at address: " + locctr + "." << endl;
                    status_flag = 1;
                }

                fout1 << locctr << "\t \t";
                fout1 << status_flag << "\t";

                // make sure opcode is always at 3 word position in intermediate file.
                fout1 << s.substr(word[0].length(), s.length()-1) << endl;
            }               

            // increment the location counter of assembler.
            locctr = toHex(toDec(locctr) + locctrADD);            
        }
    }


    // write all labels of SYMBOL TABLE to a file.    
    for ( const auto &p : SYMTAB )
    {
       labelout << p.first << '\t' << '\t' << SYMTAB[p.first].address << endl;
    }

    // check if END of the program was given or not.
    if (endPresent == 0) {
        status_flag = 1;
        error << "END of program not given!" << endl;
    }

    // close open files to prevent dynamic leakage.
    fout1.close();
    fin1.close();
    labelout.close();
}
