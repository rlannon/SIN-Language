#pragma once

#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <list>
#include <tuple>
#include <algorithm>	// std::remove_if
#include <regex>
#include <sstream>	// to convert an integer into its hex equivalent string (e.g., convert decimal 10 into the string 'A' (10 in dexadecimal)

#include "SinObjectFile.h"
//#include "BinaryIO.h"	// all the functions used to write data to binary files (for SIN bytecode/compiled-SIN (.sinc) files) -- included in "LoadSINC.h"
#include "OpcodeConstants.h"	// so we can reference opcodes by name rather than using hex values every time
#include "AddressingModeConstants.h"	// so we can reference addressing modes by name
#include "Exceptions.h"
#include "LinkerSymbols.h"

/*

This class will take a SIN Assembly file (.sina or .sinasm) and output a .sinc file. The functions that actually create the assembled file will be private, but the function to convert a .sina or .sinasm file to a .sinc file will be public.

The point of the class is to create a file that the SINVM can use; the SIN Compiler will create the SINVM Assembly file, which will go through the SIN Assembler to create a SIN Bytecode / Compiled SIN file (.sinc) which can be used by the SINVM.

This file also includes all the opcodes and their mnemonics; this is so we can refer to the opcode by its mnemonic in the code for readability


Quick guide to the assembly (see Doc/sinasm for more information):
	
	All instructions follow one of the following formats:
		MNEMONIC,
		MNEMONIC VALUE,		(VALUE can be prefixed to indicate format and addressing mode)
		.LABELNAME:,
		MACRO = VALUE
	
	When decoded, the instructions will take up at least 2 bytes -- one for the instruction opcode, one for the addressing mode, and optional bytes for data. The _WORDSIZE variable will determine how many bytes are allocated for values, and defaults to 16 bits if it is not specified. _WORDSIZE can be 16, 32, or 64 bits.

	For the available addressing modes, see "AddressingModeConstants.h"

*/

// to test whether instructions are of particular "classes"
const bool is_standalone(int opcode);	// tells us whether an opcode needs a value to follow
const bool is_bitshift(int opcode);	// tests whether the opcode is a bitshift instruction

// we also have some assembler directives that are NOT opcodes, but rather things for the assembler to do when it comes across them
const std::string assembler_directives[3] = { "DB", "RS", "INCLUDE" };


// Note: we are not defining memory locations for the various registers as they won't be considered to be part of the processor's address space in this VM's architecture


class Assembler
{
	// allow SINVM access to these functions
	friend class SINVM;
	friend class SinObjectFile;

	// tell the assembler our wordsize
	uint8_t _WORDSIZE;
	const uint8_t _MEM_WORDSIZE = 16;	// wordsize of our memory block; for now, set it to 16 bits no matter what (and 32 or 64 bit values will have to be divided)

	// Initialize the file object to store our mnemonics
	std::istream* asm_file;

	// utility variables and functions
	int line_counter;	// to track the line number we are on
	// Note: the line counter /sort of/ works; it can tell you about where you are, but it's not always exactly right
	// Also, not all errors are currently detected by the assembler before runtime

	static bool is_whitespace(char ch);	// tests whether we are in whitespace
	static bool is_not_newline(char ch);	// tests to see whether we have hit a line break yet
	static bool is_comment(char ch);	// tests whether we are at a comment
	static bool is_empty_string(std::string str);	// tests to see if a string is ""

	void getline(std::istream& file, std::string* target);	// gets a line from the file and saves it into 'target'
	bool end_of_file();	// test for EOF by peeking and checking for EOF so we don't have to do this every time
	void skip();	// skip ahead in the istream by one character
	void read_while(bool(*predicate)(char));	// read as long as "predicate" is true; this mirrors "read_while()" in "Lexer"

	// get the line data as a series of separate strings, omitting comments
	std::vector<std::string> get_line_data(std::string line);

	// track what byte of memory we are on in the program so we know what addresses to store for labels
	size_t current_byte;

	// track the current scope we are in for relative labels
	std::string current_scope;

	// test for various types (label, macro, etc)
	static bool is_label(std::string candidate);	// tests whether the string is a label
	static bool is_mnemonic(std::string candidate);	// tests whether the string is an opcode mnemonic
	static bool is_opcode(int candidate);	// tests whether the integer is a valid opcode
	static int get_integer_value(std::string value);	// converts the read value into an integer (converts numbers with prefixes)

	static bool can_use_immediate_addressing(int opcode);	// determines whether the opcode is a store instruction (STOREA, STOREB, etc.)

	// we need a symbol table to keep track of addresses when macros are used; we will use a list of tuples
	// tuple is (symbol_name, symbol_value, class)
	// class of "D" = defined, "U" = undefined, "R" = space reserved, "C" = constant
	std::list<AssemblerSymbol> symbol_table;

	// we also need a list for our "_DATA" section
	// this is simply a bytearray, and will convert string numbers where it can
	std::list<DataSymbol> data_table;

	// we also need a relocation table for all addresses used in control flow instructions
	std::list<RelocationSymbol> relocation_table;

	// we also need a function to construct the symbol table in the first pass of our code
	void construct_symbol_table();

	// look into our symbol table to get the value of the symbol requested
	int get_value_of(std::string symbol);

	// get the opcode/mnemonic
	static int get_opcode(std::string mnemonic);
	static std::string get_mnemonic(int opcode);
	static uint8_t get_addressing_mode(std::string value, std::string offset="");

	// TODO: write more assembler-related functions as needed

	// we will need to return a list of file names that we need to be linked
	std::vector<std::string> obj_files_to_link;

	// get the instructions of a SINASM file and returns them in a vector<int>
	std::vector<uint8_t> assemble();
public:
	// take a SINASM file as input and creates a .sinc file to be used by the SINVM
	void create_sinc_file(std::string output_file_name);

	// get the list of object files for the linker
	std::vector<std::string> get_obj_files_to_link();

	// take a .sinc file and return a .sina file containing the disassembled file
	void disassemble(std::istream& sinc_file, std::string output_file_name);

	// class constructor/destructor
	Assembler(std::istream& asm_file, uint8_t _WORDSIZE=16);
	~Assembler();
};

