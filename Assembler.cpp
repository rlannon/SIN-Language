#include "Assembler.h"


const bool is_standalone(int opcode) {
	// returns true if the instruction is to be used without a value following
	int i = 0;
	bool found = false;
	while (!found && (i < num_standalone_opcodes)) {
		if (opcode == standalone_opcodes[i]) {
			found = true;
		}
		else {
			i++;
		}
	}

	return found;
}

const bool is_bitshift(int opcode) {
	// returns true if the instruction is a bitshift instruction
	return (opcode == LSL || opcode == LSR || opcode == ROL || opcode == ROR);
}

// Our various utility functions

bool Assembler::is_whitespace(char ch) {
	// if the character is a space, newline, or tab, return true
	return (ch == ' ' || ch == '\n' || ch == '\t');
}

bool Assembler::is_not_newline(char ch) {
	// if the character is a newline character, return FALSE, otherwise, return TRUE
	return (ch != '\n');
}

bool Assembler::is_comment(char ch) {
	// if the character is a semicolon, it's a comment
	return ch == ';';
}

bool Assembler::is_empty_string(std::string str)
{
	// returns true if a string is empty; this is used as a predicate function when removing empty strings from 'string_delimited' later
	return (str == "");
}

void Assembler::getline(std::istream& file, std::string* target) {
	// uses std::getline, but increments line_counter
	std::getline(file, *target);
	line_counter++;
}

bool Assembler::end_of_file() {
	// peek at the next character; if it triggers EOF, then return true; else, return false
	char peek = this->asm_file->peek();
	if (peek == EOF) {
		return this->asm_file->eof();
	}
	else {
		return false;
	}
}

void Assembler::skip() {
	// skip one character
	char ch = this->asm_file->get();
	if (ch == '\n') {
		line_counter += 1;
	}
	return;
}

void Assembler::read_while(bool(*predicate)(char)) {
	// continue to read while a certain predicate function returns true

	if (this->end_of_file()) {
		std::cout << "Note: end of file reached." << std::endl;
		return;
	}
	else {
		while (!this->end_of_file() && predicate(this->asm_file->peek())) {	// continue reading while predicate is true and we haven't hit EOF
			this->skip();	// we will only skip if the peeked character (in end_of_file() ) is not EOF
		}
		return;
	}
}



// Our type testing functions -- these are all static

bool Assembler::is_label(std::string candidate) {
	// if the candidate is a label, return true
	return (candidate[0] == '.');
}

bool Assembler::is_mnemonic(std::string candidate) {
	// if the candidate is an opcode mnemonic, return true

	int index = 0;
	bool found = false;

	while (!found && (index < num_instructions)) {
		// if our candidate string is in the instructions list (use regex so we can ignore case)
		if (std::regex_match(instructions_list[index], std::regex(candidate, std::regex_constants::icase))) {
			found = true;
		}
		else {
			index++;
		}
	}

	return found;
}

int Assembler::get_integer_value(std::string value) {
	// if the string begins with a prefix to denote its base (i.e. $ or %), convert it
	// else, just use std::stoi to convert the integer

	// Note: the immediate value prefix (#) may be passed into the function, but should be handled BEFORE passing the value; this function will not return the addressing mode desired
	
	// validate we didn't get an empty string
	if (value.size() != 0) {
		// next, check to see if we have the '#' prefix; if we do, delete it
		if (value[0] == '#') {
			value = value.substr(1);	// value should now equal the slice from position 1 on
		}

		// next, check to see if we have a prefix (if it's not a digit)
		if (!isdigit(value[0])) {
			if (value[0] == '$') {
				value = value.substr(1);	// get the digits only (substring starting at value[1])
				return std::stoi(value, nullptr, 16);	// return the string interpreted as a base 16 number
			}
			else if (value[0] == '%') {
				value = value.substr(1);	// get the digits only (substring starting at value[1])
				return std::stoi(value, nullptr, 2);	// return the string interpreted as a base 2 number
			}
			// if it's not $ or %, it's not a valid operator; throw an exception
			else {
				std::string err_msg = ("The character '", value[0], "' is not a valid value operator. Options are $ (hex) or % (binary).");
				throw std::exception(err_msg.c_str());
			}
		}
		// if it is a digit, then just use stoi and return the value
		else {
			return std::stoi(value);
		}
	}
	else {
		throw std::exception("Cannot get the value of an empty string.");
	}
}


bool Assembler::can_use_immediate_addressing(int opcode) {
	// determine what CAN'T use immediate addressing
	if (opcode == STOREA || opcode == STOREB || opcode == STOREX || opcode == STOREY) {
		return false;
	}
	else if (is_standalone(opcode)) {
		return false;
	}
	else if (opcode == JMP || opcode == BRNE || opcode == BREQ || opcode == BRGT || opcode == BRLT) {
		return false;
	}
	else {
		// if it's not one of the above, it can be used with the immediate addressing mode
		return true;
	}
}


// TODO: write more error-catching functions


/************************************************************************************************************************
****************************************			ASSEMBLER FUNCTIONS			*****************************************
************************************************************************************************************************/


// Do an initial pass of the code so we can construct the symbol table
std::list<std::tuple<std::string, int>> Assembler::construct_symbol_table() {
	// Construct the symbol table in the first pass of our code

	std::list<std::tuple<std::string, int>> labels;

	// continue reading from the file as long as we have not reached the end
	while (!this->end_of_file()) {
		// continue reading through comments and whitespace
		if (this->is_comment(this->asm_file->peek())) {
			this->read_while(&this->is_not_newline);
		}
		this->read_while(&this->is_whitespace);

		std::string line_data;

		// get the first part of our line
		*this->asm_file >> line_data;

		// check its type
		if (is_mnemonic(line_data)) {
			if (is_standalone(this->get_opcode(line_data))) {
				this->current_byte += 1;
				// it's a standalone instruction, so we don't need the rest of the line
				continue;
			}
			else {
				this->current_byte += 2;	// increment the current byte by 2; one for the mnemonic itself and one for the addressing mode
			}

			// if the next character is a new line or EOF, then go back to the top of the loop; we don't have a value
			if (this->asm_file->peek() == '\n' || this->asm_file->peek() == EOF) {
				continue;
			}
			// otherwise, we must have a value
			else {
				// skip any spaces
				this->read_while(&this->is_whitespace);

				// if we have a value, skip ahead by the wordsize in bytes
				int offset = this->_WORDSIZE / 8;	// _WORDSIZE / 8 is the number of bytes to offset
				this->current_byte += offset;

				// go to the next line
				this->read_while(&this->is_not_newline);	// read until peek() returns a newline character
				this->skip();	// eat the newline character
				continue;
			}
		}
		else if (is_label(line_data)) {
			// note: labels don't actually occupy any space in memory; they are a way for us to reference memory addresses for the assembler
			// as such, we don't need to increment current_byte when we hit one; we push it to our symbol table and continue, as "current_byte" points to the first instruction /after/ the label

			// verify there is a semicolon at the end of the label and remove it before we push it back
			if (line_data[line_data.size() - 1] == ':') {
				line_data = line_data.substr(0, line_data.size() - 1);	// removes the last character in "line_data"
			}
			// throw an exception; if a label has a colon, the second pass will ignore it; if it doesn't, the second pass will try and replace it with a memory address
			else {
				throw std::exception(("Missing colon in label creation (line " + std::to_string(this->line_counter) + ")").c_str());
			}

			// push back the line_data as the symbol's name and "current_byte" as the current byte in memory; this will serve as the memory address
			labels.push_back(std::make_tuple(line_data, this->current_byte));
			
			// skip to the next line
			this->read_while(&this->is_not_newline);	// read until peek() returns a newline character
			this->asm_file->get();	// eat the newline character
			continue;
		}
		// if it's not a label and its not a mnemonic, but it isalpha(), then it must be a macro
		else if (isalpha(line_data[0])) {
			// macros at the beginning of a line are used by the assembler itself, and will not go into the final .sinc file
			// as such, ignore it; skip the rest of the line
			this->read_while(&this->is_not_newline);	// read until peek() returns a newline character
			this->asm_file->get();	// eat the newline character
			continue;
		}
	}

	return labels;
}

// Look into the symbol table and get the value stored there for the symbol we want
int Assembler::get_value_of(std::string symbol) {
	
	std::list<std::tuple<std::string, int>>::iterator symbol_table_iter = this->symbol_table.begin();
	bool found = false;

	while (!found && (symbol_table_iter != this->symbol_table.end())) {
		// if the names are the same, mark that we have found it
		if (std::get<0>(*symbol_table_iter) == symbol) {
			found = true;
		}
		// if we have not yet found it, increment the iterator
		else {
			symbol_table_iter++;
		}
	}

	if (found) {
		return std::get<1>(*symbol_table_iter);
	}
	else {
		throw std::exception(("Cannot find '" + symbol + "' in symbol table (line " + std::to_string(this->line_counter) + ")").c_str());
	}
}

// Get the opcode of some mnemonic without addressing more information
// note this is a static function
int Assembler::get_opcode(std::string mnemonic) {
	// some utility variables -- an index and a variable to tell us whether we have found the right instruction
	bool found = false;
	int index = 0;

	// iterate through the list as long as we a) haven't found the right opcode and b) haven't gone past the end
	while ((!found) && (index < num_instructions)) {
		// if our instruction is in the list
		// it's a small enough list that we can iterate over it without concern
		// use regex so we can ignore the case
		if (std::regex_match(instructions_list[index], std::regex(mnemonic, std::regex_constants::icase))) {
			// set found
			found = true;
		}
		else {
			// only increment the index if we haven't found it
			index++;
		}
	}

	// if we found the opcode during our search, return it
	if (found) {
		return opcodes[index];
	}
	// otherwise, throw an exception; the instruction they want was not recognized
	else {
		throw std::exception(("Unrecognized instruction '" + mnemonic + "'").c_str());
	}
}


uint8_t Assembler::get_address_mode(std::string value, std::string offset) {
	// given a vector of strings, this function will return the correct addressing mode to use for the instruction
	// note: does not return indirect indexed addressing; only handles two strings at maximum; in order to get the indirect indexed mode, parse the address mode within the parens and add 4 to the result

	/*

	OUR ADDRESSING MODES:

	$00	-	Absolute	(e.g., 'LOADA $1234')
	$01	-	X-indexed	(e.g., 'LOADA $1234, X')
	$02	-	Y-indexed	(e.g., 'LOADA $1234, Y')
	$03	-	Immediate	(e.g., 'LOADA #$1234')
	??	$04	-	8-bit		(e.g., 'STOREA $12')
	$05	-	Indirect indexed with X	(e.g., 'LOADA ($1234, X)')
	$06	-	Indirect indexed with Y	(e.g., 'LOADA ($1234, Y)')
	$07	-	Register	(e.g., 'LSR A')	-	Can only be used with bitshift operations

	*/

	// immediate addressing
	if (value[0] == '#') {
		return 3;
	}
	// otherwise, we are accessing memory
	else {
		// if offset is an empty string, then we have absolute addressing
		if (offset == "") {
			return 0;
		}
		// if we have data in 'offset', check to see if there is a comma after 'value'
		else if (value[value.size() - 1] == ',') {
			// if so, check to see what the first character of 'offset' is
			if (offset[0] == 'X') {
				// we have X-indexed addressing
				return 1;
			}
			else if (offset[0] == 'Y') {
				// we have Y-indexed addressing
				return 2;
			}
			// if it's not X or Y, it's not proper; throw exception
			else {
				throw std::exception("Must use register X or Y when using indirect addressing modes.");
			}
		}
	}
}



// Assemble the code

std::vector<uint8_t> Assembler::assemble()
{
	/*

	SINVM ASSEMBLER

	This assembler works in two passes; on the first pass, it goes line by line through the code and finds all labels, making note of their addresses in the symbol table. On the second pass, it actually converts the assembly syntax into SINVM Machine Code, or a .sinc file, and all label references will be looked up in the symbol table and replaced with the values stored there.
	Note macros are all parsed on the second pass; labels must be parsed in an initial pass so we can discern their location for whent they are forward-referenced.

	*/


	// Pass 1

	this->symbol_table = this->construct_symbol_table();	// get the symbol table by doing one pass over the file
	// reset our byte pointer and istream position pointer
	this->asm_file->clear();
	this->asm_file->seekg(0, std::ios::beg);
	this->current_byte = 0;
	this->line_counter = 0;

	// Pass 2

	// create objects to hold our opcodes and line data
	std::vector<uint8_t> program_data;

	// continue reading as long as we haven't hit the end of the file
	while (!this->end_of_file()) {
		std::string line;	// contains the whole line
		std::string opcode_or_symbol;	// first part of data in a line; can be an opcode, a label, or a macro
		std::string value;	// our value; whatever comes after the opcode/symbol
		const std::string delimiter = " ";	// our delimiter is a single space

		// read through any whitespace
		this->read_while(&this->is_whitespace);	// read through any whitespace

		this->getline(*this->asm_file, &line);	// get the whole line

		// split the line into parts, if we can
		std::vector<std::string> string_delimited;	// to hold the parts of our string
		size_t position = 0;
		while ((position = line.find(delimiter)) != std::string::npos) {
			string_delimited.push_back(line.substr(0, line.find(delimiter)));	// push the string onto the vector
			line.erase(0, position + delimiter.length());	// erase the string from the beginning up to the next token
		}
		string_delimited.push_back(line);

		// First, remove empty strings using std::remove_if (from <algorithm>)
		string_delimited.erase(std::remove_if(string_delimited.begin(), string_delimited.end(), is_empty_string), string_delimited.end());

		// Next, remove all comments by finding semicolons in the vector and deleting everything after them
		// find ';' in the vector
		std::vector<std::string>::iterator it;
		it = std::find(string_delimited.begin(), string_delimited.end(), ";");
		// remove all elements from there on
		string_delimited.erase(it, string_delimited.end());

		if (string_delimited.size() > 0) {

			// our opcode/symbol should be the 0th item in the vector we created
			opcode_or_symbol = string_delimited[0];

			// get the first part of the line's data, storing it in line data
			// this can contain a label, a mnemonic, or a macro
			//*this->asm_file >> opcode_or_symbol;

			// first, test to see whether we have a mnemonic, a label, or something else
			if (is_mnemonic(opcode_or_symbol)) {
				// first, get the opcode of our mnemonic
				int opcode = get_opcode(opcode_or_symbol);

				// if the next character is not a comment and it's not the end of the vector
				if ((string_delimited.size() > 1) && (!is_comment(string_delimited[1][0]))) {
					// first, push back our opcode
					program_data.push_back(opcode);

					uint8_t addressing_mode;	// our addressing mode is only 1 byte

					// the second value should be our value
					value = string_delimited[1];

					// first, check to see if we have parens -- if so, we will adjust accordingly
					if (value[0] == '(') {
						if (string_delimited.size() >= 3) {
							addressing_mode = get_address_mode(value.substr(1), string_delimited[2]);	// get the substring of 'value' starting at position 1 (ignoring paren)

							// just to make our lives easier, erase the paren right now
							value.erase(0);
						}
						else {
							// there MUST be another string in string_delimited if we have indirect addressing
							throw std::exception(("Indirect addressing requires a second string; however, one was not found (line " + std::to_string(this->line_counter) + ")").c_str());
						}

						// if the addressing mode is 1 or 2, add 4 to it to get to the indirect/indexed
						if (addressing_mode == 2 || addressing_mode == 3) {
							addressing_mode += 4;
						}
						// if neither, it's improper syntax -- cannot use parens with any other mode
						else {
							throw std::exception(("Unrecognized addressing mode (line " + std::to_string(this->line_counter) + ")").c_str());
						}

						// now that we have the addressing mode, push it to program data
						program_data.push_back(addressing_mode);

						// now, get the integer value of the address
						int address = get_integer_value(value);

						// now, turn it into a series of big-endian bytes
						for (int i = this->_WORDSIZE / 8; i > 0; i--) {
							uint8_t byte = address << ((i - 1) * 8);
							program_data.push_back(byte);
						}
					}
					// if the value is a label
					else if (value[0] == '.') {
						// addressing mode must be absolute
						addressing_mode = 0;

						// push_back the addressing mode
						program_data.push_back(addressing_mode);
						
						// get the value at 'value' (a label)
						int label_value = get_value_of(value);

						// turn it into a series of big-endian bytes
						for (int i = this->_WORDSIZE / 8; i > 0; i--) {
							uint8_t byte = label_value << ((i - 1) * 8);
							program_data.push_back(byte);
						}
					}
					// if the value is 'A'
					else if (value == "A" || value == "a") {
						// check to make sure the opcode is a bitshift instruction
						if (is_bitshift(opcode)) {
							// addressing mode for register operations is 0x07
							addressing_mode = 0x07;
							program_data.push_back(addressing_mode);
						}
						// if it's not a bitshift instruction, throw an exception
						else {
							throw std::exception(("Cannot use 'A' as an operand unless with a bitshift instruction (line " + std::to_string(this->line_counter) + ")").c_str());
						}
					}
					// otherwise, carry on normally
					else {
						// if a comment is not separated from the semicolon by a space, it won't get caught by the deletion function and it will screw with the code; as such, we must do a few extra checks
						// check to see if the last character in the second operand is a comma; if so, and there is no third operand or the third operand begins with a semicolon, throw an exception
						if (string_delimited[1][string_delimited[1].size() - 1] == ',') {
							if ((string_delimited.size() < 3) || (string_delimited[2][0] == ';')) {
								throw std::exception(("Expected register for index but found nothing (line " + std::to_string(this->line_counter) + ")").c_str());
							}
						}

						// further, if we made it this far without an exception, make sure that if we find a third string that the first character is NOT a semicolon; if so, the program should behave as if there were only two strings
						if (string_delimited.size() >= 3 && (string_delimited[2][0] != ';')) {
							addressing_mode = get_address_mode(value, string_delimited[2]);
						}
						else {
							addressing_mode = get_address_mode(value);
						}

						if (addressing_mode == 3 && !can_use_immediate_addressing(opcode)) {
							throw std::exception(("Cannot use this addressing mode on an instruction of this type (line " + std::to_string(this->line_counter) + ")").c_str());
						}

						// push_back the addressing mode and turn 'value' into a series of big_endian bytes
						program_data.push_back(addressing_mode);

						// get the integer value of the string that contains our value (might have flags like $ or %)
						int converted_value = get_integer_value(value);

						// Get the bytes in big-endian format -- we first get the highest byte, then the second-highest...
						for (int i = this->_WORDSIZE / 8; i > 0; i--) {
							uint8_t byte = converted_value >> ((i - 1) * 8);
							program_data.push_back(byte);
						}
					}
				}
				// we can push back the opcode without data bytes for a few instructions
				else if (is_standalone(opcode)) {
					program_data.push_back(opcode);
				}
				// otherwise, we must throw an exception
				else {
					throw std::exception(("Expected a value following instruction mnemonic (line " + std::to_string(this->line_counter) + ")").c_str());
				}
			}
			// if we have a label to start the line
			else if (is_label(opcode_or_symbol)) {
				// make sure it ends with a colon and move on to the next line
				if (opcode_or_symbol[opcode_or_symbol.size() - 1] == ':') {
					continue;
				}
				else {
					throw std::exception(("A label marker must end with a colon (line " + std::to_string(this->line_counter) + ")").c_str());
				}
			}
			else if (isalpha(opcode_or_symbol[0])) {
				// if we have a macro
				if (string_delimited.size() >= 3) {
					if (string_delimited[2] == "=") {
						// we have a macro

						// TODO: enable macros

					}
					else {
						throw std::exception(("Leading macros must be followed by an equals sign. (line " + std::to_string(this->line_counter) + ")").c_str());
					}
				}
				else {
					throw std::exception(("Non-opcode identifiers must be labels or macros (line " + std::to_string(this->line_counter) + ")").c_str());
				}
			}
			else if (is_comment(opcode_or_symbol[0])) {
				// don't do anything
				continue;
			}
			else {
				throw std::exception(("Unknown symbol in file (line " + std::to_string(this->line_counter) + ")").c_str());
			}
		}
		// we have an empty string
		else {
			// do nothing
		}

		// any last code for the loop should go here

	}

	// return our machine code
	return program_data;
}

void Assembler::create_sinc_file(std::string output_file_name)
{
	/*
	SINC FILE TYPE SPECIFICATION:
		A .sinc file is a compiled SIN or SIN bytecode file. It is a binary file containing a small header followed by the program data itself as a series of integers. The size of said integers is specified in the header of the file. The integers can be 16, 32, or 64 bits, depending on the machine, but it will default to the 16 bits if not specified. You can specify the word size in settings when compiling from .sin to .sina files.
		The word size in the file mostly serves as a check to ensure that the numbers are not too large for the machine running the code.

	FILE HEADER:
		The file header contains a few bytes of information:
			uint8_t magic_number[4]	=	sinC
			uint8_t version	=	(current version = 1)
			uint8_t word_size (contains the number corresponding to the number of bits in the integers represented)
			uint16_t program_size	(serves as a checksum for loading)
	BODY:
		The body of the program is simply a list of integers, corresponding to the integers produced by the assemble() function.

	*/
	
	// create a binary file of the specified name (with the sinc extension)
	std::ofstream sinc_file(output_file_name + ".sinc", std::ios::out | std::ios::binary);

	// create a vector<int> to hold the binary program data; it will be initialized to our assembled file
	std::vector<uint8_t> program_data = this->assemble();


	/************************************************************
	********************	FILE HEADER		*********************
	************************************************************/


	// Write the magic number to the file header
	char * header = ("sinC");
	sinc_file.write(header, 4);

	// write the version information
	writeU8(sinc_file, sinc_version);

	// write the word size
	writeU8(sinc_file, this->_WORDSIZE);

	// write the size of the program
	writeU16(sinc_file, program_data.size());


	/************************************************************
	********************	FILE CONTENTS	*********************
	************************************************************/

	// write each byte of data in sequentually by using a vector iterator
	for (std::vector<uint8_t>::iterator data_iter = program_data.begin(); data_iter != program_data.end(); data_iter++) {
		// write the value to the file
		writeU8(sinc_file, *data_iter);
	}

	sinc_file.close();
}



Assembler::Assembler(std::ifstream* asm_file, uint8_t _WORDSIZE)
{
	// set our line counter to 0
	this->line_counter = 0;

	// set our ASM file to be equal to the file we pass into the constructor
	this->asm_file = asm_file;

	// initialize the current byte to 0
	this->current_byte = 0;

	// set the wordsize
	this->_WORDSIZE = _WORDSIZE;

	// ensure it was actually a valid size
	if (_WORDSIZE == 32) {
		this->_WORDSIZE = 32;
	}
	else if (_WORDSIZE == 64) {
		this->_WORDSIZE = 64;
	}
	else if (_WORDSIZE != 16) {
		throw std::exception(("Cannot initialize machine word size to a value of " + std::to_string(_WORDSIZE) + "; must be 16, 32, or 64").c_str());
	}
}


Assembler::~Assembler()
{
}
