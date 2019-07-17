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
	if (this->SP > _STACK_BOTTOM) {
		uint8_t bytes[2] = { reg_to_push & 0xFF, (reg_to_push >> 8) };

		for (size_t i = 0; i < (this->_WORDSIZE / 8); i++) {
			this->memory[SP] = bytes[i];
			this->SP--;
		}
	}
	else {
		this->send_signal(SINSIGSTKFLT);
	}

	return;
}

uint16_t SINVM::pop_stack() {
	// first, make sure we aren't going to have an underflow
	if (this->SP < _STACK) {
		uint8_t stack_data[2];
		for (size_t i = 0; i < (this->_WORDSIZE / 8); i++) {
			this->SP++;
			stack_data[i] = this->memory[this->SP];
		}

		// finally, return the stack data
		uint16_t popped_value = (stack_data[0] << 8) | stack_data[1];
		return popped_value;
	}
	else {
		this->send_signal(SINSIGSTKFLT);
	}
}

void SINVM::push_call_stack(uint16_t to_push)
{
	// pushes a value onto the call stack

	// the call stack pointer has to be greater than the lowest address in the call stack
	if (this->CALL_SP > _CALL_STACK_BOTTOM) {
		for (size_t i = 0; i < (this->_WORDSIZE / 8); i++) {
			uint8_t val = to_push >> (i * 8);
			this->memory[this->CALL_SP] = val;
			this->CALL_SP--;
		}
	}
	else {
		this->send_signal(SINSIGSTKFLT);
	}
	return;
}

uint16_t SINVM::pop_call_stack()
{
	if (this->CALL_SP < _CALL_STACK) {
		uint8_t bytes[2] = { 0, 0 };
		
		for (size_t i = 0; i < (this->_WORDSIZE / 8); i++) {
			this->CALL_SP++;
			bytes[i] = this->memory[this->CALL_SP];
		}

		uint16_t to_return = (bytes[0] << 8) | bytes[1];
		return to_return;
	}
	else {
		this->send_signal(SINSIGSTKFLT);
		return static_cast<uint16_t>(SINSIGSTKFLT);
	}
}
