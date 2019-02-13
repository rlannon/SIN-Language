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

std::vector<std::string> Assembler::get_line_data(std::string line)
{
	/*
	
	Given a string, return the constituent parts of that string where the delimiter is a space

	*/
	
	const std::string delimiter = " ";

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

	return string_delimited;
}



// Our type testing functions -- these are all static

bool Assembler::is_label(std::string candidate) {
	// if the candidate is a label, return true
	return (candidate[candidate.length() - 1] == ':');
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

bool Assembler::is_opcode(int candidate)
{
	// if the candidate is an opcode mnemonic, return true

	int index = 0;
	bool found = false;

	while (!found && (index < num_instructions)) {
		// if our candidate string is in the instructions list (use regex so we can ignore case)
		if (candidate == opcodes[index]) {
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
				throw std::runtime_error(("The character '" + std::to_string(value[0]) + "' is not a valid value operator. Options are $ (hex) or % (binary).").c_str());
			}
		}
		// if it is a digit, then just use stoi and return the value
		else {
			return std::stoi(value);
		}
	}
	else {
		throw std::runtime_error("Cannot get the value of an empty string.");
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
void Assembler::construct_symbol_table() {
	// Construct the symbol and data tables in the first pass of our code

	// First, set our scope to global
	this->current_scope = "global";

	// continue reading from the file as long as we have not reached the end
	while (!this->end_of_file()) {
		// continue reading through comments and whitespace
		while (this->is_comment(this->asm_file->peek())) {
			this->read_while(&this->is_not_newline);
		}
		this->read_while(&this->is_whitespace);

		std::string line;
		this->getline(*this->asm_file, &line);

		if (line[0] == ';') {
			continue;
		}

		std::vector<std::string> line_data_vector = this->get_line_data(line);

		std::string line_data = line_data_vector[0];

		// check its type
		if (is_mnemonic(line_data)) {
			if (is_standalone(this->get_opcode(line_data))) {
				// increment by one; standalone opcodes are 1 byte in length
				this->current_byte += 1;

				// it's a standalone instruction, so we don't need the rest of the line
				continue;
			}
			else {
				this->current_byte += 2;	// increment the current byte by 2; one for the mnemonic itself and one for the addressing mode

				// check to see if we have a label; the first character in the data will be a dot
				if (line_data_vector[1][0] == '.') {
					std::string label_name = line_data_vector[1];
				}

				// next, skip ahead in our byte count by the wordsize (in bytes)
				int offset = this->_WORDSIZE / 8;	// _WORDSIZE / 8 is the number of bytes to offset
				this->current_byte += offset;

				continue;
			}
		}
		else if (is_label(line_data)) {
			// note: labels don't actually occupy any space in memory; they are a way for us to reference memory addresses for the assembler
			// as such, we don't need to increment current_byte when we hit one; we push it to our symbol table and continue, as "current_byte" points to the first instruction /after/ the label

			std::string label_name = line_data.substr(0, (line_data.size() - 1));

			// if the label begins with a dot, it is a sublabel
			if (label_name[0] == '.') {
				// the label in memory will be the scope + the sublabel
				label_name = this->current_scope + label_name;
			}
			// otherwise, if it is not a sublabel, then update the scope we are in
			else {
				this->current_scope = label_name;
			}

			// push back the line_data as the symbol's name and "current_byte" as the current byte in memory; this will serve as the memory address
			this->symbol_table.push_back(std::make_tuple(label_name, this->current_byte, "D"));	// name, value, and class
			
			// continue on to the next loop iteration
			continue;
		}
		// assembler directives begin with '@'
		else if (line_data[0] == '@') {
			// if we have a file to include
			if (line_data == "@include") {
				// get the file's name -- should be immediate after the directive
				std::string filename = line_data_vector[1];

				// get the file extension
				size_t ext_position = filename.find(".");
				std::string file_extension = filename.substr(ext_position);
				std::string filename_no_extension = filename.substr(0, ext_position);

				// if it is a .sinc file, just add it to the vector
				if (file_extension == ".sinc") {
					this->obj_files_to_link.push_back(filename);
				}
				// if it is a .sina file, we need to assemble it
				else if (file_extension == ".sina") {
					// open the file
					std::ifstream included_sina;
					included_sina.open(filename);
					if (included_sina.is_open()) {
						Assembler included_assembler(included_sina, this->_WORDSIZE);
						included_assembler.create_sinc_file(filename_no_extension);	// save it as the same filename, just with a different extension
						
						// iterate through the object files list of the included file and add them as dependencies to this file; this ensures all files get linked
						for (std::vector<std::string>::iterator it = included_assembler.obj_files_to_link.begin(); it != included_assembler.obj_files_to_link.end(); it++) {
							this->obj_files_to_link.push_back(*it);
						}

						// close the included file
						included_sina.close();
					}
					else {
						throw std::runtime_error("**** Cannot locate included file '" + filename + "'");
					}

				}
				// we can also include .bin files
				else if (file_extension == ".bin") {

					// TODO: add support for .bin files

				}
				// all other formats are unsupported at this time
				else {
					throw std::runtime_error("**** Format for included file '" + filename + "' is not supported by the assembler.");
				}
			}
			// if we have an assembler directive
			else if (line_data == "@rs") {
				/*
				Note:
				Syntax is:
				@rs _macro_name
				*/

				// the macro's name will come immediately after the directive
				std::string macro_name = line_data_vector[1];

				this->symbol_table.push_back(std::make_tuple(macro_name, 0, "R"));
			}
			else if (line_data == "@db") {
				/*
				Note:
				Syntax is:
				@db _macro_name (constant_value)
				*/

				// the macro's name will come immedaitely after the directive
				std::string macro_name = line_data_vector[1];

				std::string constant_data = "";
				for (std::vector<std::string>::iterator it = line_data_vector.begin() + 2; it != line_data_vector.end(); it++) {
					constant_data += *it;
					constant_data += " ";	// spaces were removed, as they are the delimiter
				}
				// we need to remove the parens now -- we also added one too many spaces, so remove them
				constant_data = constant_data.substr(1, constant_data.size() - 3);
				
				// add the constant to the data table
				// if it is a number, we will use stoi
				try {
					int number_base = 10;

					if (constant_data[0] == '#') {
						constant_data.erase(0, 1);
					}

					if (constant_data[0] == '$') {
						number_base = 16;
						constant_data.erase(0, 1);
					}
					else if (constant_data[0] == '%') {
						number_base = 2;
						constant_data.erase(0, 1);
					}

					int value = std::stoi(constant_data, nullptr, number_base);
					std::vector<uint8_t> data_array;
					for (size_t i = (this->_WORDSIZE / 8); i > 0; i--) {
						uint8_t to_add = value >> ((i - 1) * 8);
						data_array.push_back(to_add);
					}
					this->data_table.push_back(std::make_tuple(macro_name, data_array));
				}
				// if we can't use stoi, just push back the ASCII-encoded bytes
				catch (std::exception& e) {
					std::vector<uint8_t> data_array;
					for (std::string::iterator it = constant_data.begin(); it != constant_data.end(); it++) {
						data_array.push_back(*it);
					}
					this->data_table.push_back(std::make_tuple(macro_name, data_array));
				}

				// add it to the symbol table as well so we can reference it from other files; we will not give its address, as it will live in the _DATA section and so its address will be unknown to us until link time
				this->symbol_table.push_back(std::make_tuple(macro_name, 0, "C"));
			}
		}
		// if it's not a label, directive, or mnemonic, but it isalpha(), then it must be a macro
		else if (isalpha(line_data[0]) || (line_data[0] == '_')) {
			// macros skipped in pass 1
			continue;
		}
	}
}

// Look into the symbol table and get the value stored there for the symbol we want
int Assembler::get_value_of(std::string symbol) {
	
	std::list<std::tuple<std::string, int, std::string>>::iterator symbol_table_iter = this->symbol_table.begin();
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

	// if it's in the symbol table, return the value
	if (found) {
		// classes of C and R are technically defined but they still need to be added to the relocation table
		if ((std::get<2>(*symbol_table_iter) == "C") || (std::get<2>(*symbol_table_iter) == "R")) {
			this->relocation_table.push_back(std::make_tuple(std::get<0>(*symbol_table_iter), this->current_byte));
		}
		return std::get<1>(*symbol_table_iter);
	}
	// if it's NOT in the symbol table, add it with the class "U" and return 0
	// note it will be added to the relocation table before the value is ever retrieved
	else {
		this->symbol_table.push_back(std::make_tuple(symbol, 0x00, "U"));
		return 0;
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
		throw std::runtime_error("Unrecognized instruction '" + mnemonic + "'");
	}
}

// get the mnemonic of a given opcode
std::string Assembler::get_mnemonic(int opcode)
{
	// some utility variables -- an index and a variable to tell us whether we have found the right instruction
	bool found = false;
	int index = 0;

	// iterate through the list as long as we a) haven't found the right opcode and b) haven't gone past the end
	while ((!found) && (index < num_instructions)) {
		// if our instruction is in the list
		// it's a small enough list that we can iterate over it without concern
		if (opcode == opcodes[index]) {
			// set found
			found = true;
		}
		else {
			// only increment the index if we haven't found it
			index++;
		}
	}

	// if we found the mnemonic during our search, return it
	if (found) {
		return instructions_list[index];
	}
	// otherwise, throw an exception; the instruction they want was not recognized
	else {
		std::stringstream hex_number;
		hex_number << std::hex << opcode;	// format the decimal number as hex
		throw std::runtime_error("Unrecognized instruction opcode '$" + hex_number.str() + "'");
	}
}


uint8_t Assembler::get_addressing_mode(std::string value, std::string offset) {
	// given a vector of strings, this function will return the correct addressing mode to use for the instruction
	// note: does not return indirect indexed addressing; only handles two strings at maximum; in order to get the indirect indexed mode, parse the address mode within the parens and add 4 to the result

	// See "AddressingModeConstants" for more information about the addressing modes in SINASM

	// immediate addressing
	if (value[0] == '#') {
		return addressingmode::immediate;
	}
	// otherwise, we are accessing memory
	else {
		// if offset is an empty string, then we have absolute addressing
		if (offset == "") {
			return addressingmode::absolute;
		}
		// if we have data in 'offset', check to see if there is a comma after 'value'
		else if (value[value.size() - 1] == ',') {
			// if so, check to see what the first character of 'offset' is
			if (offset[0] == 'X' || offset[0] == 'x') {
				// we have X-indexed addressing
				return addressingmode::x_index;
			}
			else if (offset[0] == 'Y' || offset[0] == 'y') {
				// we have Y-indexed addressing
				return addressingmode::y_index;
			}
			// if it's not X or Y, it's not proper; throw exception
			else {
				throw std::runtime_error("Must use register X or Y when using indirect addressing modes.");
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

	// constructs the symbol table for the current object
	this->construct_symbol_table();


	// Pass 2

	// reset our byte pointer and istream position pointer
	this->asm_file->clear();
	this->asm_file->seekg(0, std::ios::beg);
	this->current_byte = 0;
	this->line_counter = 0;
	// reset our scope
	this->current_scope = "global";

	// create objects to hold our opcodes and line data
	std::vector<uint8_t> program_data;

	// continue reading as long as we haven't hit the end of the file
	while (!this->end_of_file()) {
		std::string line;	// contains the whole line
		std::string opcode_or_symbol;	// first part of data in a line; can be an opcode, a label, or a macro
		std::string value;	// our value; whatever comes after the opcode/symbol

		// read through any whitespace
		this->read_while(&this->is_whitespace);	// read through any whitespace

		this->getline(*this->asm_file, &line);	// get the whole line

		std::vector<std::string> string_delimited = this->get_line_data(line);

		if (string_delimited.size() > 0) {

			// our opcode/symbol should be the 0th item in the vector we created
			opcode_or_symbol = string_delimited[0];

			// first, test to see whether we have a mnemonic, a label, or something else
			if (is_mnemonic(opcode_or_symbol)) {
				// first, increment the current_byte by one; it now points to the byte AFTER the opcode
				this->current_byte++;
				// get the opcode of our mnemonic
				int opcode = get_opcode(opcode_or_symbol);

				// if the next character is not a comment and it's not the end of the vector
				if ((string_delimited.size() > 1) && (!is_comment(string_delimited[1][0]))) {
					// first, push back our opcode
					program_data.push_back(opcode);

					this->current_byte++;	// increment the current byte for the addressing mode
					uint8_t addressing_mode;	// our addressing mode is only 1 byte

					// the second value should be our value
					value = string_delimited[1];

					// first, check to see if we have parens -- if so, we will adjust accordingly
					if (value[0] == '(') {
						uint8_t to_add_to_addressing_mode;	// the number we will add to our addressing mode based on whether it's indexed indirect or indirect indexed
						if (string_delimited.size() >= 3) {
							/*
							
							At this point, we know we have one of the indirect modes -- however, we don't know which one yet.
							The way to determine is that indexed indirect addressing will leave strings like this:
								($00,
								x)
							whereas indirect indexed will look like this:
								($00),
								x
							As such, we just have to the last character of the second string; if it is a letter, we have indirect indexed; if not, but it is a paren, then we have indexed indirect
							
							This will only change the number we add to the addressing mode after we use get_addressing_mode()

							*/

							// get the value we need to add to the addressing mode (assuming we get x indexed or y indexed and not an error); this is +4 for indirect indexed ( ($00), y ) and +6 for indexed indirect ( ($00, y) )
							char last_value_char = string_delimited[2][string_delimited[2].length() - 1];	// get the last character of the second string in string_delimited
							if (last_value_char == ')') {
								// if the last character is ')', we have indexed indirect, so we need +6
								to_add_to_addressing_mode = 6;
							}
							else if (last_value_char == 'x' || last_value_char == 'X' || last_value_char == 'y' || last_value_char == 'Y') {
								// if the last character is a register name, we have indirect indexed, so we need +4
								to_add_to_addressing_mode = 4;
							}
							else {
								throw std::runtime_error("Invalid character in value expression (line " + std::to_string(line_counter) + ")");
							}

							addressing_mode = get_addressing_mode(value.substr(1), string_delimited[2]);	// get the substring of 'value' starting at position 1 (ignoring paren)

							// just to make our lives easier, erase the paren right now
							value.erase(0, 1);
						}
						else {
							// there MUST be another string in string_delimited if we have indirect addressing
							throw std::runtime_error("Indirect addressing requires a second string; however, one was not found (line " + std::to_string(this->line_counter) + ")");
						}

						// if the addressing mode is x/y indexed, add 4 to it to get to the indirect/indexed (they are 4 apart)
						if (addressing_mode == addressingmode::x_index || addressing_mode == addressingmode::y_index) {
							addressing_mode += to_add_to_addressing_mode;
						}
						// if neither, it's improper syntax -- cannot use parens with any other mode
						else {
							throw std::runtime_error("Unrecognized addressing mode (line " + std::to_string(this->line_counter) + ")");
						}

						// now that we have the addressing mode, push it to program data
						program_data.push_back(addressing_mode);

						// now, get the integer value of the address
						int address = get_integer_value(value);

						// increment the current byte according to _WORDSIZE
						this->current_byte += this->_WORDSIZE / 8;	// increment 1 per byte in the _WORDSIZE

						// now, turn the integer into a series of big-endian bytes
						for (int i = this->_WORDSIZE / 8; i > 0; i--) {
							uint8_t byte = address << ((i - 1) * 8);
							program_data.push_back(byte);
						}
					}
					// if the value is a label
					else if ((isalpha(value[0])) || (value[0] == '.') || (value[0] == '_') && !std::regex_match(value, std::regex("[ab]", std::regex_constants::icase))) {
						// first, make sure that the label does not end with a colon; throw an error if it does
						if (value[value.size() - 1] == ':') {
							throw std::runtime_error("Labels must not be followed by colons when referenced (line " + std::to_string(this->line_counter) + ")");
						}

						// next, check to see if it is a sublabel or not
						if (value[0] == '.') {
							value = this->current_scope + value;
						}

						// we must first note that we have a reference in the relocation table
						this->relocation_table.push_back(std::make_tuple(value, this->current_byte));

						// addressing mode must be absolute
						addressing_mode = addressingmode::absolute;

						// push_back the addressing mode
						program_data.push_back(addressing_mode);

						//// get the value at 'value' (a label)
						//int label_value = get_value_of(value);

						// increment the current byte according to _WORDSIZE
						this->current_byte += (this->_WORDSIZE / 8);	// increment 1 per byte in the _WORDSIZE

						// turn the value into a series of big-endian bytes
						for (int i = this->_WORDSIZE / 8; i > 0; i--) {
							//uint8_t byte = label_value << ((i - 1) * 8);
							//program_data.push_back(byte);
							program_data.push_back(0x00);	// push back 0s; this will be updated by the linker
						}
					}
					// if the value is 'A'
					else if ((value.size() == 1) && (value == "A" || value == "a")) {
						// check to make sure the opcode is a bitshift instruction
						if (is_bitshift(opcode)) {
							addressing_mode = addressingmode::reg_a;
							program_data.push_back(addressing_mode);
						}
						// if it's not a bitshift instruction, throw an exception
						else {
							throw std::runtime_error("Cannot use 'A' as an operand unless with a bitshift instruction (line " + std::to_string(this->line_counter) + ")");
						}
					}
					// if the value is 'B'
					else if ((value.size() == 1) && (value == "B" || value == "b")) {
						// make sure it is an addition or subtraction
						if ((opcode == ADDCA) || (opcode == SUBCA)) {
							addressing_mode = addressingmode::reg_b;
							program_data.push_back(addressing_mode);
						}
						// if it's not, throw an exception
						else {
							throw std::runtime_error("May only use 'B' as an operand with addition and subtraction instructions (line " + std::to_string(this->line_counter) + ")");
						}
					}
					// otherwise, carry on normally
					else {
						// if a comment is not separated from the semicolon by a space, it won't get caught by the deletion function and it will screw with the code; as such, we must do a few extra checks
						// check to see if the last character in the second operand is a comma; if so, and there is no third operand or the third operand begins with a semicolon, throw an exception
						if (string_delimited[1][string_delimited[1].size() - 1] == ',') {
							if ((string_delimited.size() < 3) || (string_delimited[2][0] == ';')) {
								throw std::runtime_error("Expected register for index but found nothing (line " + std::to_string(this->line_counter) + ")");
							}
						}

						// further, if we made it this far without an exception, make sure that if we find a third string that the first character is NOT a semicolon; if so, the program should behave as if there were only two strings
						if (string_delimited.size() >= 3 && (string_delimited[2][0] != ';')) {
							addressing_mode = get_addressing_mode(value, string_delimited[2]);
						}
						else {
							addressing_mode = get_addressing_mode(value);
						}

						if (addressing_mode == 3 && !can_use_immediate_addressing(opcode)) {
							throw std::runtime_error("Cannot use this addressing mode on an instruction of this type (line " + std::to_string(this->line_counter) + ")");
						}

						// push_back the addressing mode and turn 'value' into a series of big_endian bytes
						program_data.push_back(addressing_mode);

						// if 'value' starts with a pound sign, delete it
						if (value[0] == '#') {
							value = value.substr(1);
						}

						// declare the converted_value variable
						int converted_value = 0;

						// if it is a symbol or label, add an entry to the relocation table
						if (isalpha(value[0]) || (value[0] == '.') || (value[0] == '_')) {
							this->relocation_table.push_back(std::make_tuple(value, this->current_byte));
						}
						else {
							converted_value = get_integer_value(value);
						}

						// increment the current byte according to _WORDSIZE
						this->current_byte += this->_WORDSIZE / 8;	// increment 1 per byte in the _WORDSIZE

						// if "value" was a symbol
						if (isalpha(value[0]) || (value[0] == '.') || (value[0] == '_')) {
							// push back all 0x00 if it is a label; it will be resolved
							for (int i = (this->_WORDSIZE / 8); i > 0; i--) {
								program_data.push_back(0x00);
							}
						}
						// otherwise, it must be an actual value
						else {
							// Get the bytes in big-endian format -- we first get the highest byte, then the second-highest...
							for (int i = this->_WORDSIZE / 8; i > 0; i--) {
								// because it is an actual value, we can push back the bytes in it
								uint8_t byte = converted_value >> ((i - 1) * 8);
								program_data.push_back(byte);
							}
						}
					}
				}
				// we can push back the opcode without data bytes for a few instructions
				else if (is_standalone(opcode)) {
					program_data.push_back(opcode);
				}
				// otherwise, we must throw an exception
				else {
					throw std::runtime_error("Expected a value following instruction mnemonic (line " + std::to_string(this->line_counter) + ")");
				}
			}
			// if we have an assembler directive, skip it; these were assessed in the first pass
			else if (opcode_or_symbol[0] == '@') {
				// since we already have all of the line data, continue to the next loop iteration
				continue;
			}
			// if we have a label to start the line
			else if (is_label(opcode_or_symbol)) {
				// eliminate the colon at the end
				std::string label_name = opcode_or_symbol.substr(0, (opcode_or_symbol.size() - 1));

				// if it is not a sublabel, update the scope
				if (label_name[0] != '.') {
					this->current_scope = label_name;
				}
				else {
					// do nothing
				}
			}
			else if (isalpha(opcode_or_symbol[0])) {
				if (string_delimited.size() >= 3) {
					// if we have a macro
					if (string_delimited[1] == "=") {
						// we have a macro

						// macros can only be assigned one time; as such, if it is at the beginning of the line, then we have the initial assignment
						std::string macro_name = string_delimited[0];

						// to make it more readable, this is the string that has the macro value
						std::string macro_value_string = string_delimited[2];
						int macro_value = this->get_integer_value(macro_value_string);

						// if the macro name is already in the symbol table, update the value
						bool is_in_symbol_table = false;
						std::list<std::tuple<std::string, int, std::string>>::iterator iter = this->symbol_table.begin();

						while (iter != this->symbol_table.end() && !is_in_symbol_table) {
							if (macro_name == std::get<0>(*iter)) {
								is_in_symbol_table = true;
							}
							else {
								iter++;
							}
						}

						// if it's not in the symbol table, add it
						// also, add it to the relocation table (and its address)
						if (!is_in_symbol_table) {
							this->symbol_table.push_back(std::make_tuple(macro_name, macro_value, "M"));
						}
						// otherwise, update the reference
						else {
							*iter = std::make_tuple(macro_name, macro_value, "M");
						}
					}
					else {
						throw std::runtime_error("Leading macros must be followed by an equals sign. (line " + std::to_string(this->line_counter) + ")");
					}
				}
				else {
					throw std::runtime_error("Non-opcode identifiers must be labels, macros, or assembler directive instructions (line " + std::to_string(this->line_counter) + ")");
				}
			}
			else if (is_comment(opcode_or_symbol[0])) {
				// don't do anything
				continue;
			}
			else {
				throw std::runtime_error("Unknown symbol in file (line " + std::to_string(this->line_counter) + ")");
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



// disassemble a .sinc file and produce a .sina file of the same name
// TODO: update for new .sinc format; use labels / macros

void Assembler::disassemble(std::istream& sinc_file, std::string output_file_name) {
	// create an object for our object file
	SinObjectFile object_file;
	object_file.load_sinc_file(sinc_file);

	// load the file data appropriately
	this->_WORDSIZE = object_file.get_wordsize();
	std::vector<uint8_t> program_data = object_file.get_program_data();

	// create our output file
	std::ofstream sina_file;
	sina_file.open(output_file_name, std::ios::out);

	// iterate through our vector of bytes
	std::vector<uint8_t>::iterator program_iterator = program_data.begin();
	while (program_iterator != program_data.end()) {
		int opcode = *program_iterator;

		// if we have a valid opcode
		if (is_opcode(opcode)) {
			// if it's a standalone opcode, write it to the file, add a newline character, and continue
			if (is_standalone(*program_iterator)) {
				sina_file << get_mnemonic(*program_iterator) << std::endl;
				program_iterator++;
				continue;
			}
			else {
				// get the addressing mode
				program_iterator++;
				uint8_t addressing_mode = *program_iterator;

				program_iterator++;
				// get the number of bytes as is appropriate for the wordsize
				int value = 0;
				for (int i = this->_WORDSIZE / 8; i > 0; i--) {
					value += *program_iterator;	// add the value of the program iterator to 'value'
					value = value << ((i - 1) * 8);	// will shift by 0 the last time
					program_iterator++;	// increment the iterator
				}

				// write the opcode
				sina_file << get_mnemonic(opcode) << " ";

				// write the value, the format of which depends on the addressing mode
				if (addressing_mode == 0) {
					sina_file << "$" << std::hex << value << std::endl;
				}
				else if (addressing_mode == 1) {
					sina_file << "$" << std::hex << value << ", X" << std::endl;
				}
				else if (addressing_mode == 2) {
					sina_file << "$" << std::hex << value << ", Y" << std::endl;
				}
				else if (addressing_mode == 3) {
					sina_file << "#$" << std::hex << value << std::endl;
				}
				else if (addressing_mode == 4) {
					// TODO: add 8-bit mode ?
				}
				else if (addressing_mode == 5) {
					sina_file << "($" << std::hex << value << ", X)" << std::endl;
				}
				else if (addressing_mode == 6) {
					sina_file << "($" << std::hex << value << ", Y)" << std::endl;
				}
				else if (addressing_mode == 7) {
					sina_file << "A" << std::endl;
				}

				continue;
			}
		}
		else {
			std::stringstream hex_number;
			hex_number << std::hex << *program_iterator;	// format the decimal number as hex
			throw std::runtime_error("Unrecognized instruction opcode '$" + hex_number.str() + "'");
		}
	}

	// close the file and return to caller
	sina_file.close();
	return;
}



/************************************************************************************************************************
****************************************				OBJECT FILES			*****************************************
************************************************************************************************************************/



void Assembler::create_sinc_file(std::string output_file_name)
{
	/*
	
	This function writes an object file using the current assembler object and the SinObjectFile class. It is the entry point to the function, and all other assembly functions within end up being called by the object file class.

	*/

	// create an object to hold the data for our .sinc file, and pass it the assembler object we are in
	SinObjectFile object_file;
	AssemblerData asm_data(this->_WORDSIZE, this->assemble());
	
	// the tables don't get set in initialization because the file needs to be assembled first
	asm_data._symbol_table = this->symbol_table;
	asm_data._relocation_table = this->relocation_table;
	asm_data._data_table = this->data_table;

	object_file.write_sinc_file(output_file_name, asm_data);

	// return to caller
	return;
}

std::vector<std::string> Assembler::get_obj_files_to_link()
{
	return this->obj_files_to_link;
}



/************************************************************************************************************************
****************************************			CLASS CONSTRUCTOR			*****************************************
************************************************************************************************************************/



Assembler::Assembler(std::istream& asm_file, uint8_t _WORDSIZE) : asm_file(&asm_file), _WORDSIZE(_WORDSIZE)
{
	// set our line counter to 0
	this->line_counter = 0;

	// initialize the current byte to 0
	this->current_byte = 0;

	// ensure it was actually a valid size
	if (_WORDSIZE != 16 && _WORDSIZE != 32 && _WORDSIZE != 64) {
		throw std::runtime_error("Cannot initialize machine word size to a value of " + std::to_string(_WORDSIZE) + "; must be 16, 32, or 64");
	}
}


Assembler::~Assembler()
{
}
