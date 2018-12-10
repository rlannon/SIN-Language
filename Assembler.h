#pragma once

#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <list>
#include <tuple>

/*

This class will take a SIN Assembly file (.sina or .sinasm) and output a .sinc file. The functions that actually create the assembled file will be private, but the function to convert a .sina or .sinasm file to a .sinc file will be public.

The point of the class is to create a file that the SINVM can use; the SIN Compiler will create the SINVM Assembly file, which will go through the SIN Assembler to create a SIN Bytecode / Compiled SIN file (.sinc) which can be used by the SINVM.

This file also includes all the opcodes and their mnemonics; this is so we can refer to the opcode by its mnemonic in the code for readability


General guide to the assembly:
	
	All instructions follow one of the following formats:
		MNEMONIC,
		MNEMONIC VALUE,		(VALUE can be prefixed to indicate format and addressing mode)
		.LABELNAME:,
		MACRO = VALUE
	
	When decoded, the instructions will take up at least 2 bytes -- one for the instruction opcode, one for the addressing mode, and optional bytes for data. The _WORDSIZE variable will determine how many bytes are allocated for values, and defaults to 16 bits if it is not specified. _WORDSIZE can be 16, 32, or 64 bits.

	The following addressing modes are available
		Absolute	-	e.g., LOADA $1234		-	Gets the value at the address specified
		Indexed		-	e.g., LOADA $1234, X	-	Gets the value at the address specified, indexed with x (so here, if x were $02, it would get the value at the address of $1236)
		Immediate	-	e.g., LOADA #$1234		-	The literal value written; no memory access
		Relative	-	
		Indirect	-	e.g., LOADA ($1234, Y)	-	

*/

const int num_instructions = 32;

const int HALT = 0xFF;	// halt
const int NOOP = 0x00;	// no operation

// register A
const int LOADA = 0x01;	// load a with value
const int STOREA = 0x02;	// store a at address

// register B
const int LOADB = 0x03;
const int STOREB = 0x04;

// register X
const int LOADX = 0x05;	// load x with value
const int STOREX = 0x06;	// store x at address

// register Y
const int LOADY = 0x07;	// load y with value
const int STOREY = 0x08;	// store y at address

// STATUS register
const int CLC = 0x09;	// clear carry bit
const int SEC = 0x0A;	// set carry bit

// ALU-related instructions
const int ADDCA = 0x10;	// add register A (with carry) to some value, storing the result in A
const int SUBCA = 0x11;	// subtract some value (with carry) from register A ...
const int ANDA = 0x12;	// logical AND some value with A ...
const int ORA = 0x13;	// logical OR some value with A ...
const int XORA = 0x14;	// logical XOR with A ...
const int LSR = 0x15;	// logical shift right on some memory (or A)
const int LSL = 0x16;	// logical shift left on some memory (or A)
const int ROR = 0x17;	// rotate right on some memory (or A)
const int ROL = 0x18;	// rotate left on some memory (or A)

// Comparatives
const int CMPA = 0x20;	// compare registers by value
const int CMPB = 0x21;
const int CMPX = 0x22;
const int CMPY = 0x23;

// branch / control flow logic
const int JMP = 0x24;	// unconditional jump to supplied address
const int BRNE = 0x25;	// branch on not equal
const int BREQ = 0x26;	// branch on equal
const int BRGT = 0x27;	// branch on greater
const int BRLT = 0x28;	// branch on less
const int BRZ = 0x29;	// branch on zero

// note: we are not defining memory locations for the various registers as they won't be considered to be part of the processor's address space in this VM's architecture

class Assembler
{
	// allow SINVM access to these functions
	friend class SINVM;

	// tell the assembler our wordsize
	uint8_t _WORDSIZE;
	const uint8_t _MEM_WORDSIZE = 16;	// wordsize of our memory block; for now, set it to 16 bits no matter what (and 32 or 64 bit values will have to be divided)

	// Initialize the file object to store our mnemonics
	std::ifstream* asm_file;

	// utility variables and functions
	static bool is_whitespace(char ch);	// tests whether we are in whitespace
	static bool is_not_newline(char ch);	// tests to see whether we have hit a line break yet
	static bool is_comment(char ch);	// tests whether we are at a comment
	bool end_of_file();	// test for EOF by peeking and checking for EOF so we don't have to do this every time
	void skip();	// skip ahead in the istream by one character
	void read_while(bool(*predicate)(char));	// read as long as "predicate" is true; this mirrors "read_while()" in "Lexer"

	// track what byte of memory we are on in the program so we know what addresses to store for labels
	int current_byte;

	// test for various types (label, macro, etc)
	static bool is_label(std::string candidate);	// tests whether the string is a label
	static bool is_mnemonic(std::string candidate);	// tests whether the string is an opcode mnemonic
	static int get_integer_value(std::string value);	// converts the read value into an integer (converts numbers with prefixes)

	// we need a symbol table to keep track of addresses when macros are used; we will use a list of tuples
	std::list<std::tuple<std::string, int>> symbol_table;

	// we also need a function to construct the symbol table in the first pass of our code
	std::list<std::tuple<std::string, int>> construct_symbol_table();

	// look into our symbol table to get the value of the symbol requested
	int get_value_of(std::string symbol);

	// get the opcode of whatever mnemonic we requested, not accounting for its address mode
	static int get_opcode(std::string mnemonic);
	static uint8_t get_address_mode(std::string value, std::string offset="");

	// TODO: write more assembler-related functions as needed

	// get the instructions of a SINASM file and returns them in a vector<int>
	std::vector<uint8_t> assemble();
public:
	void create_sinc_file(std::string output_file_name);	// takes a SINASM file as input and creates a .sinc file to be used by the SINVM
	void create_txt_file(std::string output_file_name);	// disassembles the code, but outputs to a .txt file instead of a .sinc file

	Assembler(std::ifstream* asm_file, uint8_t _WORDSIZE=16);
	~Assembler();
};

