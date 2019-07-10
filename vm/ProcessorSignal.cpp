/*

SIN Toolchain
ProcessorSignal.cpp
Copyright 2019 Riley Lannon

Contains the implementation of the VM function to handle processor signals

*/

#include "SINVM.h"

void SINVM::send_signal(uint8_t sig) {
	/*

	Accepts a processor signal and performs the appropriate actions for it. Whenever a signal is generated, the following routine happens:
		1) If the signal is SINSIGSEGV or SINSIGKILL, the processor cannot recover; it prints the appropriate message and aborts
		2) Otherwise, the processor will push the PC to the call stack such that the next instruction to be executed is the instruction that generated the signal
		3) The VM looks to its signal vector at 0xF000 and sees if there is a routine to handle the generated signal
		4) The VM will execute that function if possible, or abort if there is none

	*/

	// two signals may not be trapped
	if (sig == SINSIGKILL) {
		// abort
		this->set_status_flag('H');
		throw VMException("SINSIGKILL generated; aborting execution", this->PC, this->STATUS);
	}
	else if (sig == SINSIGSEGV) {
		this->set_status_flag('H');
		throw VMException("Segmentation violation; aborting execution", this->PC, this->STATUS);
	}
	// the RESET signal will always do the same thing
	else if (sig == SINSIGRESET) {
		/*
		The RESET signal will essentially cause the processor to go back to its initial state without reloading any memory
			1) clear the status register
			2) reset the program counter (to _PRG_BOTTOM - 1, it will increment at the end of the cycle)
			3) reset the stack pointers
			4) clear our dynamic objects vector
		*/
		this->STATUS = 0;
		this->PC = _PRG_BOTTOM - 1;
		this->SP = _STACK;
		this->CALL_SP = _CALL_STACK;
		this->dynamic_objects.clear();
	}
	// the rest can be trapped
	else {
		bool was_caught = false;
		uint16_t vector_data = 0;
		size_t vector_address = 0;
		std::string sig_name = "";

		// first, see if there is data at the memory location we want for the signal generated
		if (sig == SINSIGFPE) {
			vector_address = _SINSIGFPE_VECTOR;
			sig_name = "SINSIGFPE";
		}
		else if (sig == SINSIGSYS) {
			vector_address = _SINSIGSYS_VECTOR;
			sig_name = "SINSIGSYS";
		}
		else if (sig == SINSIGILL) {
			vector_address = _SINSIGILL_VECTOR;
			sig_name = "SINSIGILL";
		}
		else if (sig == SINSIGSTKFLT) {
			vector_address = _SINSIGSTKFLT_VECTOR;
			sig_name = "SINSIGSTKFLT";
		}
		else {
			throw std::runtime_error("Unrecognized signal number " + std::to_string((size_t)sig));
		}

		// get the data at the vector and see if we caught it (if caught, the memory at the vector will not be 0)
		vector_data = (this->memory[vector_address] << 8) + (this->memory[vector_address + 1]);
		was_caught = vector_data != 0;

		// if the signal was caught, jump to its handler
		if (was_caught) {
			// write the return address -- the current address - 1 -- to the call stack
			uint16_t return_address = this->PC - 1;
			for (size_t i = 0; i < (this->_WORDSIZE / 8); i++) {
				uint8_t memory_byte = return_address >> (i * 8);
				this->memory[CALL_SP] = memory_byte;
				this->CALL_SP--;
			}

			// set the PC equal to vector_data - 1 to perform the jump
			this->PC = vector_data - 1;
		}
		// otherwise, throw a generic error, returning the signal name, the PC value, and the STATUS register
		else {
			this->set_status_flag('H');
			throw VMException(sig_name + " signal generated; aborting execution", this->PC, this->STATUS);
		}
	}
}