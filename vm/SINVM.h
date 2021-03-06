/*

SIN Toolchain
SINVM.h
Copyright 2019 Riley Lannon

The virtual machine that is responsible for executing SIN bytecode. While this VM is mostly self-sufficient, it is not completely made from the ground up.
It still requires explicit host interaction through the SYSCALL instruction for standard and file I/O, and will use native code on the host machine to implement various libraries that require the OS API, in a similar manner as the JVM.

*/

#pragma once

#include <vector>
#include <list>
#include <tuple>
#include <string>
#include <fstream>
#include <iostream>

#include "../assemble/Assembler.h"
#include "../util/SinObjectFile.h"	// to load a .SINC file
#include "../util/VMMemoryMap.h"	// contains the constants that define where various blocks of memory begin and end in the VM
#include "DynamicObject.h"	// for use in allocating objects on the heap
#include "../util/Exceptions.h"	// for VMException
#include "StatusConstants.h"
#include "ALU.h"
#include "FPU.h"
#include "../util/Signals.h"


class SINVM
{

	// the VM's word size
	const uint8_t _WORDSIZE = 16;
	uint16_t _DB_START;

	// the VM will contain an ALU instance
	ALU alu;	// todo: allocate alu and fpu on heap?
	FPU fpu;

	// create objects for our program counter and stack pointer
	uint16_t PC;	// points to the memory address in the VM containing the next byte we want
	uint16_t SP;	// points to the next byte to be written in the stack
	uint16_t CALL_SP;	// the call stack pointer -- return addresses are not held on the regular stack; modified only by JSR and RTS

	// create objects for our registers
	uint16_t REG_A;
	uint16_t REG_B;
	uint16_t REG_X;
	uint16_t REG_Y;

	uint16_t STATUS;	// a register to our status information

	// create an array to hold our program memory
	uint8_t memory[memory_size];

	// create a list to hold our DynamicObjects
	std::list<DynamicObject> dynamic_objects;

	// send a processor signal
	void send_signal(uint8_t sig);

	// check whether a memory address is legal
	static const bool address_is_valid(size_t address, bool privileged = false);

	// read a value in memory
	uint16_t get_data_of_wordsize();
	std::vector<uint8_t> get_properly_ordered_bytes(uint16_t value);

	// execute a single instruction
	void execute_instruction(uint16_t opcode);

	// instruction-specific load/store functions
	uint16_t execute_load();
	void execute_store(uint16_t reg_to_store);

	// generic load/store functions
	uint16_t get_data_from_memory(uint16_t address, bool is_short = false);
	void store_in_memory(uint16_t address, uint16_t new_value, bool is_short = false);

	void execute_bitshift(uint16_t opcode);

	void execute_comparison(uint16_t reg_to_compare);
	void execute_jmp();

	void execute_syscall();

	// stack functions
	void push_stack(uint16_t reg_to_push);
	uint16_t pop_stack();

	void push_call_stack(uint16_t to_push);
	uint16_t pop_call_stack();

	// syscall utility
	void free_heap_memory();
	void allocate_heap_memory();
	void reallocate_heap_memory(bool error_if_not_found = true);

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

