#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
using namespace std;

//struct for holding LA record
struct LA_output {
	string token;   //category
	string lexeme;  //actual
};

//functions to write to file
template <typename T>
void writetofile(ofstream& ofs, T& x, string token, LA_output & val) {
	val.lexeme = x;
	val.token = token;
	ofs << setw(10) << x << "\t\t\t" << token << endl;
}
template <typename T>
void writetofile(T* x, string token, LA_output & val) {
	val.lexeme = x;
	val.token = token;
	writetofile(*x, token);
}
///////////////////////////////////////////////////////////////////////////////////
void m_id_key(ofstream& ofs, char *&p, string * keyword, LA_output & val) {
	//since starting state is 1 and entering this function requires a letter input
	int state = 2;
	bool found_keyword = false;
	string word = "";
	/*
	A finite state machine for identifiers and keywords;
	will continue checking character inputs as long as
	they are valid inputs (letter, digit, $).
	Accepting states 2, 3, 5
	*/

	int id_FSM[6][4]{
		//state   L   D   $
		1,        2,  6,  6,
		2,        3,  4,  5,
		3,        3,  4,  5,
		4,        3,  4,  5,
		5,        6,  6,  6,
		6,        6,  6,  6

	};



	//while valid input
	while (((isalpha(*p)) || (isdigit(*p))) || (*p == '$')) {
		if (isalpha(*p))
			state = id_FSM[state - 1][1];
		else if (isdigit(*p))
			state = id_FSM[state - 1][2];
		else
			state = id_FSM[state - 1][3];
		word += *p;
		++p;
	}

	//if accepting state
	if (((state == 2) || (state == 3)) || (state == 5)) {
		//check if keyword
		for (int i = 0; i < 13; i++) {
			if (word == keyword[i]) {
				found_keyword = true;
				writetofile(ofs, word, "keyword", val);
			}
		}
		//otherwise its an identifier
		if (!found_keyword)
			writetofile(ofs, word, "identifier", val);
	}
	//not accepting state
	else
		writetofile(ofs, word, "unknown", val);
}

//determines whether the input characters are integers or real numbers
void m_int_real(ofstream& ofs, char*&p, LA_output & val) {
	int state = 2; //set to 2 since input char was a digit
				   /*
				   combination of integer and real number FSM
				   states 2 is accepting states for integer numbers
				   state 4 is the accepting state for real numbers
				   state 5 is an error state.
				   */
	int int_real_FSM[5][3]{
		//state     D   .
		1,          2,  5,
		2,          2,  3,
		3,          4,  5,
		4,          4,  5,
		5,          5,  5,
	};

	string word = "";
	//while valid input
	while (isdigit(*p) || (*p) == '.') {
		if (isdigit(*p))
			state = int_real_FSM[state - 1][1];
		else
			state = int_real_FSM[state - 1][2];
		word += *p;
		++p;
	}

	if (state == 2)
		writetofile(ofs, word, "integer", val);
	else if (state == 4)
		writetofile(ofs, word, "real", val);
	else
		writetofile(ofs, word, "unknown", val);
}

//determines if the character is a separator
bool is_sep(ofstream& ofs, char *&p, char * sep, LA_output & val) {
	for (int i = 0; i < 10; i++) {
		if (*p == sep[i]) {
			if (*p == '%') {
				//if next char is also %
				if (*(++p) == '%') {
					writetofile(ofs, "%%", "separator", val);
					++p;
					return true;
				}
				else {
					//decrement to original p for error message
					--p;
					char tmp = p[0];
					writetofile(ofs, tmp, "unknown", val);
					++p;
					return true;
				}
			}
			else {
				char tmp = p[0];
				writetofile(ofs, tmp, "separator", val);
				++p;
				return true;
			}
		}
	}
	return false;
}
//determines if the current character and next char are operators
bool is_op(ofstream& ofs, char *&p, char * ope, LA_output & val) {
	string word_ope = "";
	for (int i = 0; i < 8; i++) {
		if (*p == ope[i]) {
			word_ope += *p;
			++p;
			//second loop to check if next char is part of operator
			for (int k = 0; k < 8; k++) {
				if (*p == ope[k]) {
					word_ope += *p;
					++p;
					break; //if operator found leave the loop
				}
			}
			writetofile(ofs, word_ope, "operator", val);
			return true;
		}
	}
	return false;
}
//checks for comments
bool is_comment(char *&p, fstream & fin, string & x) {
	if (*p == '!') {
		++p;
		while (*p != '!')
		{
			if (p != &x[x.length()])
				++p;
			else
			{
				getline(fin, x);
				p = &x[0];
			}
		}
		++p;
		return true;
	}
	return false;
}

//the lexer function considers all valid inputs
LA_output lexer(char *& p, ofstream & outfile, fstream & fin, string * keywords,
	string & x, char * operators, char * separators) {
	LA_output ex;
	if (isalpha(*p))m_id_key(outfile, p, keywords, ex);
	else if (isdigit(*p))m_int_real(outfile, p, ex);
	else if (is_op(outfile, p, operators, ex)) {}
	else if (is_sep(outfile, p, separators, ex)) {}
	else if ((((*p) == 32) || (*p == '\t')) || ((*p == '\n') || (int)(*p) == 13)) { ++p; }
	else if (is_comment(p, fin, x)) {}
	else {
		char tmp = p[0];
		writetofile(outfile, tmp, "unknown", ex);
		++p;
	}
	return ex;
}

int main() {

	//valid keywords, separators, and operators
	string keywords[13] = { "int","boolean","real","function","return","true",
		"false","if","else","endif","while","get","put" };
	char separators[10] = { '(',')',':',',','{','}', ']', '[', ';', '%' };
	char operators[8] = { '<','>','=','^','+','-','*','/' };

	//read file
	fstream fin;

	//candidate lexeme
	char * lex_candid;

	//holds each input file line
	string line_code;
	string file_name;

	//structure containing lexer function results
	LA_output record;

	//prompt user to enter file name
	cout << "What is the name of the txt file you want to test:  " << endl;
	cin >> file_name;
	fin.open(file_name);

	//while unable to open file prompt user to re-enter file name
	while (!(fin.is_open())) {
		cout << "Unable to open file; please re-enter file name" << endl;
		cin >> file_name;
		fin.open(file_name);
	}

	//creating and formatting output file
	ofstream outfile("output.txt");
	outfile << setw(10) << "token" << "\t\t\t" << "lexeme" << endl;
	outfile << "--------------------------------------------------------------\n";

	//while there are remaining code lines
	while (getline(fin, line_code)) {
		//point to the first character of line
		lex_candid = &line_code[0];
		while (lex_candid != &line_code[line_code.length()]) {
			//calling lexer and storing results
			record = lexer(lex_candid, outfile, fin, keywords, line_code, operators, separators);
		}
	}

	fin.close();
	outfile.close();
	return 0;
}