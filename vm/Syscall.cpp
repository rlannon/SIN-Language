/*

SIN Toolchain
Syscall.cpp
Copyright 2019 Riley Lannon

This includes the implementation of the syscall instruction, including functionality for some of the syscall operations.

Note the SYSCALL instruction is akin to an INT instruction in Unix; it is like an interrupt request, but it triggers the VM to perform some interaction with the host system. This allows the VM to be very separated from the host, and only interact with it in very specific and well-defined ways.

*/

#include "SINVM.h"

void SINVM::execute_syscall() {
	// get the addressing mode
	this->PC++;
	uint8_t addressing_mode = this->memory[this->PC];

	// increment the PC to point at the data and get the syscall number
	this->PC++;
	uint16_t syscall_number = this->get_data_of_wordsize();

	// TODO: implement more syscalls and split them into their own functions

	// If our syscall is for stdin or stdout
	if (syscall_number == 0x13) {
		// get user input
		// first, the program will look in the B register for the address where it should put the data
		unsigned int start_address = REG_B;

		// all input comes in as a string, but we want to save it as a series of bytes
		std::string input;
		std::getline(std::cin, input);
		input.push_back('\0');	// add a null terminator

		// input will always return a series of ASCII-encoded bytes

		// make a vector of uint8_t to hold our input data, always encoded as ASCII text
		std::vector<uint8_t> input_bytes;

		// iterate over the string and store the characters in 'input_bytes'
		for (std::string::iterator string_character = input.begin(); string_character != input.end(); string_character++) {
			input_bytes.push_back(*string_character);
		}

		// the length of the input buffer is the max - min + 1, as we start at 0x00 and end at 0xFF
		size_t buffer_length = _STRING_BUFFER_MAX - _STRING_BUFFER_START + 1;

		// check to make sure the input data won't overflow the buffer
		if (input_bytes.size() <= buffer_length) {
			// store those bytes in memory
			// use size() here because we want to index
			for (int i = 0; i < input_bytes.size(); i++) {
				this->memory[start_address + i] = input_bytes[i];
			}

			// finally, store the length of input_bytes (in bytes) in register A
			this->REG_A = input_bytes.size();
		}
		// if the data is longer than the input buffer, only copy in as many bytes as we have available in the buffer, truncating the input data; if we don't do this, the data could overflow into the stack
		else {
			for (int i = 0; i < buffer_length; i++) {
				this->memory[start_address + i] = input_bytes[i];
			}

			// store the length of the memory buffer in register A
			this->REG_A = buffer_length;
		}
	}
	else if (syscall_number == 0x14) {
		// If we want to print something to the screen, we must specify the address where it starts
		// It will print a number of bytes from memory as specified by REG_A, formatted as ASCII
		// The system will use the ASCII value corresponding to the hex value stored in memory and print that

		size_t num_bytes = REG_A;
		// the current address from which we are reading data
		unsigned int current_address = REG_B;
		// the current character we are on; start at 'start_address'
		uint8_t current_char = this->memory[current_address];

		std::string output_string;

		for (size_t i = 0; i < num_bytes; i++) {
			output_string.push_back(current_char);	// add the character to our output string
			current_address++;	// increment the address by one byte
			current_char = this->memory[current_address];	// get the next character
		}

		// print the string
		std::cout << output_string << std::endl;
	}
	else if (syscall_number == 0x15) {
		// Read out the number of bytes stored in A, starting at the address stored in B, and print them (as raw hex values) to the standard output
		int num_bytes = REG_A;	// number of bytes is in A
		int start_address = REG_B;	// start address is in B

		for (int i = 0; i < num_bytes; i++) {
			// note -- must cast to int before output -- otherwise, it will print the character, not the hex value
			std::cout << "$" << std::hex << (int)this->memory[start_address + i] << std::endl;
		}
	}
	else if (syscall_number == 0x20) {
		this->free_heap_memory();
	}
	else if (syscall_number == 0x21) {
		this->allocate_heap_memory();
	}
	else if (syscall_number == 0x22) {
		this->reallocate_heap_memory();	// reallocates heap memory, returning NULL if the object isn't found
	}
	else if (syscall_number == 0x23) {
		this->reallocate_heap_memory(false);	// reallocates heap memory, creating a new object if one isn't found
	}
	// if it is not a valid syscall number, throw an error
	else {
		throw VMException("Unknown syscall number; halting execution.", this->PC);
	}
}
