/*

SIN Toolchain
Stack.cpp
Copyright 2019 Riley Lannon

Contains the implementations of the stack instruction routines for the SIN VM.

*/

#include "SINVM.h"


void SINVM::push_stack(uint16_t reg_to_push) {
	// push the current value in "reg_to_push" (A or B) onto the stack, decrementing the SP (because the stack grows downwards)

	// first, make sure the stack hasn't hit its bottom -- it must be at least 2 above the stack bottom (wordsize)
	if (this->SP < _STACK_BOTTOM + 2) {
		this->send_signal(SINSIGSTKFLT);
	}
	else {
		for (size_t i = (this->_WORDSIZE / 8); i > 0; i--) {
			uint8_t val = (reg_to_push >> ((i - 1) * 8)) & 0xFF;
			this->memory[SP] = val;
			this->SP--;
		}
	}

	return;
}

uint16_t SINVM::pop_stack() {
	/*
	Pops the most recently pushed value off the stack; this means we must:
		1) increment the SP (as the current value pointed to by SP is the one to which we will write next; also, the stack grows downwards so we want to increase the address if we pop something off)
		2) dereference; this means this area of memory is the next to be written to if we push something onto the stack
	*/

	// first, make sure we aren't going to have an underflow
	if (this->SP >= _STACK) {
		this->send_signal(SINSIGSTKFLT);
		return 0;
	}
	else {
		/*
		For each byte in a word, we must:
			1) increment the stack pointer by one byte (increments BEFORE reading, as the SP points to the next AVAILABLE byte)
			2) get the data, shifted over according to what byte number we are on
				Remember: we are reading the data back in little-endian byte order, not big, so we shift BEFORE we add
		*/
		uint16_t stack_data = 0;
		for (size_t i = 0; i < (this->_WORDSIZE / 8); i++) {
			this->SP++;
			stack_data += (this->memory[SP] << (i * 8));
		}

		// finally, return the stack data
		return stack_data;
	}
}
