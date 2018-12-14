#pragma once

#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <list>
#include <tuple>
#include <algorithm>	// std::remove_if
#include <regex>

#include "BinaryIO.h"	// all the functions used to write data to binary files (for SIN bytecode/compiled-SIN (.sinc) files)

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

	The following addressing modes are available
		Absolute	-	e.g., LOADA $1234		-	Gets the value at the address specified
		Indexed		-	e.g., LOADA $1234, X	-	Gets the value at the address specified, indexed with x (so here, if x were $02, it would get the value at the address of $1236)
		Immediate	-	e.g., LOADA #$1234		-	The literal value written; no memory access; 16-bit value
		8-bit		-	e.g., LOADA $12			-	Like absolute, but only addresses a single byte
		Indirect	-	e.g., LOADA ($1234, Y)	-	
		Register	-	e.g., LSR A				-	Can only be used with the A register and the bitshift instructions; operates on the given register

		// TODO: add support for 8-bit addressing (so only write to the single byte specified; not 2 bytes)

*/

// to maintain the .sinc file standard
const uint8_t sinc_version = 1;

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
// increment/decrement registers
const int INCA = 0x19;	// increment A
const int DECA = 0x1A;	// decrement A
const int INCX = 0x1B;	// can only do this to A, X, Y (not B)
const int DECX = 0x1C;
const int INCY = 0x1D;
const int DECY = 0x1E;

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
const int JSR = 0x3A;	// jump to subroutine
const int RTS = 0x3B;	// return from subroutine

// register transfers
// we can transfer register values to A and the value in A to any register, but not other combinations
const int TBA = 0x2A;	// transfer B to A
const int TXA = 0x2B;
const int TYA = 0x2C;
const int TSPA = 0x2D;	// transfer stack pointer to A
const int TAB = 0x2E;	// transfer A to B
const int TAX = 0x2F;
const int TAY = 0x30;
const int TASP = 0x31;	// transfer A to stack pointer

// the stack
const int PHA = 0x32;	// push A onto the stack
const int PLA = 0x33;	// pop a value off the stack and store in A

// SYSCALL -- handles all interaction with the host machine
const int SYSCALL = 0x36;

// TODO: implement db instruction

// DB instruction -- so that we can set a memory array programatically (particularly useful for strings)
const int DB = 0x40;


// some constants for opcode comparisons (used for maintainability)
const size_t num_instructions = 50;
const std::string instructions_list[num_instructions] = { "HALT", "NOOP", "LOADA", "STOREA", "LOADB", "STOREB", "LOADX", "STOREX", "LOADY", "STOREY", "CLC", "SEC", "ADDCA", "SUBCA", "ANDA", "ORA", "XORA", "LSR", "LSL", "ROR", "ROL", "INCA", "DECA", "INCX", "DECX", "INCY", "DECY", "CMPA", "CMPB", "CMPX", "CMPY", "JMP", "BRNE", "BREQ", "BRGT", "BRLT", "BRZ", "TBA", "TXA", "TYA", "TSPA", "TAB", "TAX", "TAY", "TASP", "PHA", "PLA", "SYSCALL", "DB" };
const int opcodes[num_instructions] = { HALT, NOOP, LOADA, STOREA, LOADB, STOREB, LOADX, STOREX, LOADY, STOREY, CLC, SEC, ADDCA, SUBCA, ANDA, ORA, XORA, LSR, LSL, ROR, ROL, INCA, DECA, INCX, DECX, INCY, DECY, CMPA, CMPB, CMPX, CMPY, JMP, BRNE, BREQ, BRGT, BRLT, BRZ, TBA, TXA, TYA, TSPA, TAB, TAX, TAY, TASP, PHA, PLA, SYSCALL, DB };

// opcodes which do not need values to follow them (and, actually, for which proceeding values are forbidden)
const size_t num_standalone_opcodes = 20;
const int standalone_opcodes[num_standalone_opcodes] = { HALT, NOOP, CLC, SEC, INCA, DECA, INCX, DECX, INCY, DECY, TBA, TXA, TYA, TSPA, TAB, TAX, TAY, TASP, PHA, PLA };

const bool is_standalone(int opcode);	// tells us whether an opcode needs a value to follow
const bool is_bitshift(int opcode);	// tests whether the opcode is a bitshift instruction


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

	// track what byte of memory we are on in the program so we know what addresses to store for labels
	int current_byte;

	// test for various types (label, macro, etc)
	static bool is_label(std::string candidate);	// tests whether the string is a label
	static bool is_mnemonic(std::string candidate);	// tests whether the string is an opcode mnemonic
	static int get_integer_value(std::string value);	// converts the read value into an integer (converts numbers with prefixes)

	static bool can_use_immediate_addressing(int opcode);	// determines whether the opcode is a store instruction (STOREA, STOREB, etc.)

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

	Assembler(std::ifstream* asm_file, uint8_t _WORDSIZE=16);
	~Assembler();
};

