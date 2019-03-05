#pragma once

#include <vector>
#include <list>
#include <tuple>
#include <string>
#include <fstream>
#include <iostream>

#include "Assembler.h"
#include "SinObjectFile.h"	// to load a .SINC file
// #include "BinaryIO.h"	// included in Assembler.h, but commenting here to denote that functions from it are being used in this class
// #include "OpcodeConstants.h"	// included in Assembler.h, but commenting here to serve as a reminder that the constants are used in this class so we don't need to use the hex values whenever referencing an instruction
#include "VMMemoryMap.h"	// contains the constants that define where various blocks of memory begin and end in the VM
//#include "AddressingModeConstants.h"	// included in Assembler.h
#include "DynamicObject.h"	// for use in allocating objects on the heap
#include "Exceptions.h"	// for VMException

/*
	The virtual machine that will be responsible for interpreting SIN bytecode
*/

class SINVM
{

	// the VM's word size
	// TODO: use initializer list so that this can be a const member?
	uint8_t _WORDSIZE;

	// create objects for our program counter and stack pointer
	uint16_t PC;	// points to the memory address in the VM containing the next byte we want
	uint16_t SP;	// points to the next byte to be written in the stack
	uint16_t CALL_SP;	// the call stack pointer -- return addresses are not held on the regular stack; modified only by JSR and RTS

	// create objects for our registers
	uint16_t REG_A;
	uint16_t REG_B;
	uint16_t REG_X;
	uint16_t REG_Y;

	/*
	The status register has the following layout:
		7	6	5	4	3	2	1	0
		N	V	0	H	0	F	Z	C
	Flag meanings:
		N: Negative
		V: Overflow
		H: HALT instruction executed
		F: Floating-point
		Z: Zero
		C: Carry
	Notes:
		- The Z flag is also used for equality; compare instructions will set the zero flag if the operands are equal
	*/

	uint8_t STATUS;	// our byte to hold our status information

	// create an array to hold our program memory
	uint8_t memory[memory_size];
	size_t program_start_address;

	// create a list to hold our DynamicObjects
	std::list<DynamicObject> dynamic_objects;

	// check whether a memory address is legal
	static const bool address_is_valid(size_t address);

	// read a value in memory
	uint16_t get_data_of_wordsize();
	std::vector<uint8_t> get_properly_ordered_bytes(int value);

	// execute a single instruction
	void execute_instruction(int opcode);

	// instruction-specific load/store functions
	uint16_t execute_load();
	void execute_store(uint16_t reg_to_store);

	// generic load/store functions
	int get_data_from_memory(uint16_t address, bool is_short = false);
	void store_in_memory(uint16_t address, uint16_t new_value, bool is_short = false);

	void execute_bitshift(int opcode);

	void execute_comparison(int reg_to_compare);
	void execute_jmp();

	// stack functions; ALWAYS use register A
	void push_stack(int reg_to_push);
	int pop_stack();

	// syscall utility
	void free_heap_memory();
	void allocate_heap_memory();	// if the "in_bytes" flag is set, we use bytes, not words, for storage

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
	~SINVM();
};

