#include<iostream>
#include<cstdlib>
#include<fstream>
#include<string>
#include<cstdio>
#include<map>
#include<climits>

using namespace std;

#include "Pass1.cpp"

ofstream objcode, mod;
ifstream intermediate, err, modintake;

// method to get words of the given program line.  Almost same as extract method in pass1.
void getProgramLine(string a, string word[]) {
    for (int i = 0; i < 6; i++) {
        word[i] = "";
    }

    int count=0;

    for (int i = 0; i < a.length(); i++) {

        if (isWhiteSpace(a[i])) {
            continue;
        } else if (a[i] == '.') {
            return;
        } else{
            while (!isWhiteSpace(a[i]) && a[i] != ',' && i < a.length()) {
                word[count] += a[i];
                i++;
            }
            count++;
        }
    }
}

// code to generate initial text record.
string generateTextRecord(string a[], string trecord) {
	trecord = "T^";
	int j = a[0].length();

	for (int i = 6-j; i > 0; i--) {
		trecord += "0";
	}
	for (int i = 0; i < j; i++) {
		trecord += a[0][i];
	}

	return trecord;
}

// code to write text record into object code file.
void writeTextRecord(string trecord, int t_length) {
	objcode << trecord.substr(0,8) + "^";

	string length = toHex(t_length);

	int j = length.length();

	for (int i = 2-j; i > 0; i--) {
		objcode << "0";
	}
	for (int i = 0; i < j && i < 2; i++) {
		objcode << length[i];
	}

	objcode << trecord.substr(8, trecord.length()) << endl;
}

// specify the max length of a text record in object code file.
int trecordMaxLength = toDec("4D");

// method to check if text record has exceeded specified text record max length (trecordMaxLength).
void checkTrecordLength(string& trecord, int& t_length, string a[], int add) {

	// check if text record is larger than max allowed, if a new program line object code is written.
	if (trecord.length() + add > trecordMaxLength) {
		writeTextRecord(trecord, t_length);
		trecord = generateTextRecord(a, trecord);
		t_length = 0;
	}

	// check if text record is larger than max allowed, when new program line is taken.
	if ( (toDec(a[0]) - toDec(trecord.substr(2,6))) > trecordMaxLength) {
		writeTextRecord(trecord, t_length);
		trecord = generateTextRecord(a, trecord);
		t_length = 0;
	}
}

int runPass2() {

	string line, a[6], trecord;

	// specify the length of text record in bytes.
	int t_length = 0;

	// assign 0 to status_flag for pass 2.
	status_flag = 0;

	err.open("error.txt");
	intermediate.open("intermediate.txt");
    objcode.open("object.txt");
    mod.open("modification.txt");

    // check if errors are present during pass 1.  Is so, exit assembly.
	if (getline(err, line)) {
        cout<<"Errors encountered in Pass1! "<<"Have a look at the error file to know more!"<<endl;
        error.close();
        exit(1);
    }

    // read intermediate file line by line.
    while (getline(intermediate, line)) {

    	// extract words of the read program line.
    	getProgramLine(line, a);

    	// create Header record of the object code.
    	if (a[1] == "2" || a[1] == "4") {
    		if (a[3] == "START") {
    			objcode << "H^";
    			int j = a[2].length();
	    		for (int i = 0; i < j && i < 6; i++) {
	    			objcode << a[2][i];
	    		}
	    		for (int i = 6-j; i > 0; i--) {
	    			objcode << " ";
	    		}
    		} else {
	    		objcode << "H^      ";
    		}

    		objcode << "^";

    		int j = a[0].length();
    		for (int i = 6-j; i > 0; i--) {
    			objcode << "0";
    		}
    		for (int i = 0; i < j; i++) {
    			objcode << a[0][i];
    		}
    		objcode << "^";

    		string programLength = toHex(toDec(locctr) - toDec(startaddress));
    		int progLength = programLength.length();

    		for (int i = 6 - progLength; i > 0; i--) {
    			objcode << "0";
    		} 

    		for (int i =0; i < progLength && i < 6; i++) {
    			objcode << programLength[i];
    		}

    		objcode << endl;
    	}

    	// if the text record has not been initialized, generate it.
    	if (t_length == 0) {
    		trecord = generateTextRecord(a, trecord);
    	}

    	// if END of program encountered, write current text record, modification records, and end record.
    	if (a[1] == "3") {

    		// write current text record of object code.
			writeTextRecord(trecord, t_length);


			// write modification records of object code.
			mod.close();

  		    modintake.open("modification.txt");
  		    string m;

  		    while (getline(modintake, m)) {
  		    	objcode << m << endl;
  		    }

  		    // write end record of object code.
    		objcode << "E^";
    		int j = startaddress.length();

    		for (int i = 6-j; i > 0; i--) {
    			objcode << "0";
    		}
    		for (int i = 0; i < j; i++) {
    			objcode << startaddress[i];
    		}

    		objcode << endl;
    		break;
    	}

    	// if program line is start line or comment line, go to next program line.
    	if (a[1] == "2" || a[0] == "$") {
    		continue;
    	}

    	// check if text record length exceeds the max length.
    	checkTrecordLength(trecord, t_length, a, 0);

    	// if opcode is RSUB, specify the text record.
    	if (a[2] == "RSUB") {

    		checkTrecordLength(trecord, t_length, a, 6);
    		
			trecord += "^4F0000";
			t_length += 3;
			continue;
    	}

    	// if opcode is +RSUB, specify the text record.
    	if (a[2] == "+RSUB") {
    		
    		checkTrecordLength(trecord, t_length, a, 8);

			trecord += "^4F000000";
			t_length += 4;
			continue;
    	}

    	// flag to check if extended format is used in program line.
    	int flag_34 = 0;

		if (a[2][0] == '+') {
			flag_34 = 1;

			a[2] = a[2].substr(1, a[2].length());
		}

    	if (OPTAB[a[2]].exist == 'y') {
    		// opcode exists in OPCODE TABLE

    		string opcode = OPTAB[a[2]].opcode;

    		if (OPTAB[a[2]].format == 1) {
    			// instruction format is type 1.

    			checkTrecordLength(trecord, t_length, a, 2);
    			
    			trecord += "^";
    			trecord += opcode;

				t_length += 1;

    		} else if (OPTAB[a[2]].format == 2) {
    			// instruction format is type 2.
    			
    			checkTrecordLength(trecord, t_length, a, 4);

    			trecord += "^";
    			trecord += opcode;

    			// check if registers specified in instruction exist or not. If so, add to text record.
    			if (REGISTER[a[3]].exist == 'y') {
    				trecord += to_string(REGISTER[a[3]].num);

	    			if (a[4] == "") {
	    				// second register is accumulator.

	    				trecord += "0";
	    			} else if (REGISTER[a[4]].exist == 'y'){
	    				// second register exists
	    				trecord += to_string(REGISTER[a[4]].num);
	    			} else {
	    				// show error as second register is incorrect.
	    				status_flag = 1;
	    				trecord += "0";
	    				error << "Incorrect second register in operand at address: " << a[0] << endl;
	    			}
    			} else {
    				// show error as first register is incorrect.
    				status_flag = 1;
    				error << "Incorrect first register in operand at address: "<< a[0] << endl;
    			}
    			
    			t_length += 2;
    		} else {
    			// instruction in program line is type 3 or 4.

    			if (flag_34 == 1) {
    				checkTrecordLength(trecord, t_length, a, 8);
    			} else {
    				checkTrecordLength(trecord, t_length, a, 6);
    			}

    			trecord += "^";

    			// address field of text record.
    			string address;

    			// xbpe field of text record.
    			string xbpe = "0000";

    			// flag to keep track of immediate addressing.
    			int immaddr = 0;

    			// ascertain 'n' and 'i' field of text record.
    			if (a[3][0] == '#') {
    				// immediate addressing is used in program line.

    				immaddr = 1;
    				opcode = toHex(toDec(opcode) + 1);

    				a[3] = a[3].substr(1, a[3].length());

    				if (SYMTAB[a[3]].exist == 'y') {
    					address = SYMTAB[a[3]].address;
    					xbpe = "0010";
    				} else if (isdigit(a[3][0])) {
    					address = toHex(atoi(a[3].c_str()));
    				} else {
    					status_flag = 1;
    					address = "0";
    					error << "Wrong immediate addressing at address: " << a[0] << endl;
    				}
    			} else if (a[3][0] == '@') {
    				// indirect addressing is used in program line.

    				if (flag_34 == 1) {
    					status_flag = 1;
    					address = "00000";
    					error << "Indirect addressing used with extended mode of addressing at address: " <<  a[0] << endl;
    				}

    				opcode = toHex(toDec(opcode) + 2);

    				a[3] = a[3].substr(1, a[3].length());


    				if (SYMTAB[a[3]].exist == 'y') {
    					address = SYMTAB[a[3]].address;
    					xbpe = "0010";
    				} else {
    					status_flag = 1;
    					address = "0";
    					error << "Wrong indirect addressing at address: " << a[0] << endl;
    				}
    			} else {
    				// direct addressing is used in program line.

    				opcode = toHex(toDec(opcode) + 3);

    				if (SYMTAB[a[3]].exist == 'y') {
    					address = SYMTAB[a[3]].address;
    					xbpe = "0010";
    				} else {
    					status_flag = 1;
    					address = "0";
    					error << "Wrong label in operand field at address:  " << a[0] << endl;
    				}
    			}

    			// if extended format is used, set 'e' field to 1.
    			if (flag_34 == 1) {
    				xbpe = "0001";
    			}

    			// if immediate or extended format is not used, then make address PC or Base relative.
    			if (xbpe == "0010" && flag_34 == 0) {

    				int pcCheck = (toDec(address) - OPTAB[a[2]].format - toDec(a[0]));

    				// check which mode of relative addressing is possible.
    				if (pcCheck >= -2048 && pcCheck < 2048) {
    					// PC relative addressing is possible.
    					address = toHex(pcCheck);
    				} else if (SYMTAB["BASE"].exist == 'y') {

    					// find address specified by BASE assembler directive.
    					string basaddr = SYMTAB["BASE"].address;
    					basaddr = SYMTAB[basaddr].address;

    					int bcCheck = (toDec(address) - toDec(basaddr));

    					// check if NOBASE assembler directive has been used.
    					if (SYMTAB["NOBASE"].exist == 'n') {

    						// if NOBASE not used, check if address is in bounds of base addressing.
							if (bcCheck >= 0 && bcCheck < 4096) {
	    						xbpe = "0100";
	    						address = toHex(bcCheck);
	    					} else {
	    						status_flag = 1;
	    						error << "Specify a better base.  Base addressing too big, consider extended addressing. Found at address: " << a[0] << endl;
	    						address = "0";
	    					}
    					} else {

    						// get address at which NOBASE was declared.
							string nobaseaddr = SYMTAB["NOBASE"].address;

							if (toDec(nobaseaddr) < toDec(a[0])) {
								// if NOBASE specified before program line, then BASE relative addressing cannot be used.
								status_flag = 1;
								address = "0";
								error << "Specify base addressing, PC relative addressing insufficient.  Found at address: " << a[0] << endl;
							} else {
								// check if address is in bounds of base addressing.
								if (bcCheck >= 0 && bcCheck < 4096) {
		    						xbpe = "0100";
		    						address = toHex(bcCheck);
		    					} else {
		    						status_flag = 1;
		    						error << "Specify a better base.  Base addressing too big, consider extended addressing. Found at address: " << a[0] << endl;
		    						address = "0";
		    					}
							}
    					}    					
    				} else {
    					// show error as BASE is not specified and PC relative addressing is too big.
    					status_flag = 1;
    					error << "No BASE specified, not possible to do PC relative or Base relative addressing at address: " << a[0] << endl;
    					address = "0";
    				}
    			} 

    			// set 'x' field = 1 if index addressing is used.
    			if (a[4] == "X") {
    				xbpe = "1" + xbpe.substr(1,4);
    			}

    			// write opcode to text record.
    			int oplength = opcode.length();

    			for (int i = 2-oplength; i > 0; i--) {
    				trecord += "0";
    			} 
    			for (int i = 0; i < oplength; i++) {
    				trecord += opcode[i];
    			}

    			// write xbpe field to text record.
    			string binar =  toHex(BintoDec(xbpe));
    			trecord += binar;

    			// specify address field length according to instruction format.
    			int addrLength = 3;

    			if (flag_34 == 1) {
    				addrLength = 5;
    			}

    			// check if address specified in program line is too big to fit in address field of text record.
    			int m = address.length();

    			if (m > addrLength) {
    				status_flag = 1;
    				address = "000";
    				error << "Too big address specified.  Consider using extended addressing.  Found at address: " << a[0] << endl;
    			}

    			// write address in text record.
    			for (int i = addrLength-m; i > 0; i--) {
    				trecord += "0";
    			}
    			for (int i = 0; i < m && i < addrLength; i++) {
    				trecord += address[i];
    			}

    			// increase size of text record according to the instruction format.
    			if (flag_34 == 1) {
    				t_length += 4;
    			} else {
    				t_length += 3;
    			}

    			// specify the modification records if extended addressing but not immediate addressing is not used.
    			if (flag_34 == 1 && immaddr == 0) {

    				mod << "M^";

    				string reladdr = toHex(toDec(a[0]) - toDec(startaddress) + 1);

    				int l = reladdr.length();

    				for (int i = 5-l; i >0; i--) {
    					mod << "0";
    				}
    				for (int i =0; i < l && i <5; i++) {
    					mod << reladdr[i];
    				}

    				mod<< "^05"<<endl;
    			} 
    		}
    	} else if (a[2] == "BYTE") {
    		// BYTE is specified in the program line.
    		string constant;

    		// get constant specified in the program line.
    		if (a[3][0] == 'C') {
    			constant = "";
    			string constintermediate = a[3].substr(2, -1);
    			for (int i = 0; i < constintermediate.length()-1; i++) {
    				constant += to_string(int(constintermediate[i]));
    			}
    		} else if (a[3][0] == 'X') {
    			string constintermediate = a[3].substr(2, a[3].length() - 1);
    			for (int i = 0; i < constintermediate.length()-1; i++) {
    				constant += constintermediate[i];
    			}
    		} else {
    			constant = a[3];
    		}

    		checkTrecordLength(trecord, t_length, a, constant.length());

    		trecord += "^";

    		// write text record.
    		for (int i =0; i < constant.length(); i++) {
    			trecord += constant[i];
    		}

    		// increment text record length.
    		t_length += (constant.length()/2);
    	} else if (a[2] == "WORD") {
    		// WORD is specified in the program line.

    		checkTrecordLength(trecord, t_length, a, 6);

    		trecord += "^";

    		// extract constant specified in program line.
    		string constant = toHex(atoi(a[3].c_str()));
    		int constlength = constant.length();

    		// if the length of constant is bigger that 3 bytes, show error.
    		if (constlength > 6) {
    			status_flag = 1;
    			error << "Constant too big for 3 bytes at address: " << a[0] << endl;
    			trecord += "000000";
    			continue;
    		}

    		// write text record.
    		for (int i = 6 - constlength; i > 0; i--) {
    			trecord += "0";
    		}
    		for (int i = 0; i < constlength; i++) {
    			trecord += constant[i];
    		}

    		// increment text record length.
    		t_length += 3;
    	} 
    }

    // check if errors occured.  If so, notify the user.
    if (status_flag == 1) {
        cout<< "Errors encountered!  Have a look at the error file to know more!" <<endl;
    } else {
    	cout << "Object Code created successfully!!" << endl;
    }

    // close all open files.
    err.close();
	intermediate.close();
    objcode.close();
    error.close();
    modintake.close();
}
