#pragma once

#include <vector>
#include <list>
#include <tuple>
#include <string>
#include <fstream>
#include <iostream>

#include "Assembler.h"
// #include "FundamentalDataTypes.h"	// included in Assembler.h, but commenting here to denote that functions from it are being used in this class

/*
	The virtual machine that will be responsible for interpreting SIN bytecode
*/

class SINVM
{
	// declare how much memory the virtual machine has
	static const size_t memory_size = 256;

	// the VM's word size
	uint8_t _WORDSIZE;

	// create objects for our program counter and stack pointer
	uint8_t* PC;
	size_t SP;	// why of different types? should be consistent...

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
	void execute_load(int* reg_target);
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

	// file I/O
	std::tuple<uint8_t, std::vector<uint8_t>> load_sinc_file(std::istream& file);	// load our file (if there is one) and populate our instructions list
public:
	// entry function for the VM -- execute a program
	void run_program();

	void _debug_values();	// for debug -- print values to screen

	// constructor/destructor
	SINVM(std::istream& file);	// if we have a .sinc file we want to load
	SINVM(Assembler& assembler);
	~SINVM();
};

