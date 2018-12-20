#pragma once

#include <vector>
#include <list>
#include <tuple>
#include <string>
#include <fstream>
#include <iostream>

#include "Assembler.h"
#include "LoadSINC.h"	// to load a .SINC file
// #include "BinaryIO.h"	// included in Assembler.h, but commenting here to denote that functions from it are being used in this class
// #include "OpcodeConstants.h"	// included in Assembler.h, but commenting here to serve as a reminder that the constants are used in this class so we don't need to use the hex values whenever referencing an instruction

/*
	The virtual machine that will be responsible for interpreting SIN bytecode
*/

class SINVM
{
	// declare how much memory the virtual machine has
	static const size_t memory_size = 65536;	// 16k available to the VM

	// declare our start addresses for different sections of memory
	static const size_t _DATA = 0x0000;	// our "_DATA" section will always start at $00
	static const size_t _STACK = 0x1fff;	// our "STACK" will always start at $2fff and grow downwards
	static const size_t _STACK_BOTTOM = 0x1000;	// the bottom of the stack
	static const size_t _CALL_STACK = 0x2fff;
	static const size_t _CALL_STACK_BOTTOM = 0x2000;	// our call stack
	static const size_t _PRG_TOP = 0xff00;	// the limit for our program data -- if it hits $ff00, it's too large
	static const size_t _PRG_BOTTOM = 0x3000;	// our lowest possible memory address for the program
	static const size_t _ARG = 0xff00;	// ff00 - ffff -- one page -- available for command-line/environment arguments

	// the VM's word size
	uint8_t _WORDSIZE;

	// create objects for our program counter and stack pointer
	uint16_t PC;
	uint16_t SP;
	uint16_t CALL_SP;	// the call stack pointer -- return addresses are not held on the regular stack; modified only by JSR and RTS
	
	// TODO: reconcile types...PC must be more than 1 byte, but if we are dealing with bytes...
	// Combine opcode and addressing mode into one byte, and change from uint8_t as base memory unit to int ?
	// OR, use 2 uint8_ts (or a uint16_t) for the PC

	// create objects for our registers
	int REG_A;
	int REG_B;
	int REG_X;
	int REG_Y;

	/*
	The status register has the following layout:
		7	6	5	4	3	2	1	0
		N	V	0	H	0	E	Z	C
	Flag meanings:
		N: Negative
		V: Overflow
		H: HALT instruction executed
		Z: Zero
		C: Carry
	Notes:
		- The Z flag is also used for equality; compare instructions will set the zero flag if the operands are equal
	*/

	uint8_t STATUS;	// our byte to hold our status information

	// create an array to hold our program memory
	uint8_t memory[memory_size];
	size_t program_start_address;

	// check whether a memory address is legal
	static const bool address_is_valid(size_t address);

	// read a value in memory
	int get_data_of_wordsize();
	std::vector<uint8_t> get_properly_ordered_bytes(int value);

	// execute a single instruction
	void execute_instruction(int opcode);

	// instruction-specific load/store functions
	int execute_load();
	void execute_store(int reg_to_store);

	// generic load/store functions
	int get_data_from_memory(int address);
	void store_in_memory(int address, int new_value);

	void execute_bitshift(int opcode);

	void execute_comparison(int reg_to_compare);
	void execute_jmp();

	// stack functions; ALWAYS use register A
	void push_stack();
	void pop_stack();

	// status flag utility
	void set_status_flag(char flag); // set the status flag whose abbreviation is equal to the character 'flag'
	void clear_status_flag(char flag);	// clear the status flag whose abbreviation is equal to the character 'flag'
	uint8_t get_processor_status();	// return the status register
	bool is_flag_set(char flag);	// tells us if a specific flag is set
public:
	// entry function for the VM -- execute a program
	void run_program();

	void _debug_values();	// for debug -- print values to screen

	// constructor/destructor
	SINVM(std::istream& file);	// if we have a .sinc file we want to load
	SINVM(Assembler& assembler);
	~SINVM();
};

