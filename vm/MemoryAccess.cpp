/*

SIN Toolchain
MemoryAccess.cpp
Copyright 2019 Riley Lannon

Contains the implementations to various memory access functions in the SIN VM such as load, store, read, and write functions.

*/

#include "SINVM.h"

uint16_t SINVM::get_data_of_wordsize() {
	/*

	Since we might have a variable wordsize, we need to be able to handle numbers of various word sizes. As such, have a function to do this for us (since we will be doing it a lot).
	The routine is simple: initialize a variable to 0, read in the first byte, add it to our variable, and shift the bits left by 8; then, increment the PC. Continue until we have one more byte left to read, at which point, stop looping. Then, dereference the PC and add it to our variable. We can then return the variable.
	It is done this way so that it
		a) doesn't shift the bits too far over, requiring us to shift them back after
		b) doesn't increment the PC one too many times, requiring us to decrement it after

	Note on usage:
		The function should be executed *with the PC pointing to the first byte we want to read*. It will end with the PC pointing to the next byte to read after we are done.

	*/

	// load the appropriate number of bytes according to our wordsize
	uint16_t data_to_load = 0;

	// loop one time too few
	for (size_t i = 1; i < (this->_WORDSIZE / 8); i++) {
		data_to_load = data_to_load + this->memory[this->PC];
		data_to_load = data_to_load << 8;
		this->PC++;
	}
	// and put the data in on the last time -- this is to prevent incrementing PC too much or bitshifting too far
	data_to_load = data_to_load + this->memory[this->PC];

	return data_to_load;
}

uint16_t SINVM::execute_load() {
	/*

	Execute a LOAD_ instruction. This function takes a pointer to the register we want to load and executes the load instruction accordingly, storing the ultimate fetched result in the register we passed into the function.

	The function first reads the addressing mode, uses get_data_of_wordsize() to get the data after the addressing mode data (could be interpreted as an immediate value or memory address). It then checks the addressing mode to see how it needs to interpret the data it just fetched (data_to_load), and acts accordingly (e.g., if it's absolute, it gets the value at that memory address, then stores it in the register; if it is indexed, it does it almost the same, but adds the address value first, etc.).

	This function also makes use of 'validate_address()' to ensure all the addresses are within range.

	*/


	// the next data we have is the addressing mode
	this->PC++;
	uint8_t addressing_mode = this->memory[this->PC];

	if (addressing_mode == addressingmode::reg_b) {
		return REG_B;
	}

	// the next data is the first byte of the data
	this->PC++;

	// load the appropriate number of bytes according to our wordsize
	uint16_t data_to_load = this->get_data_of_wordsize();

	/*

	If we are using short addressing, we will set a flag and subtract 0x10 (base for short addressing) from the addressing mode before proceeding. We will then pass our flag into the SINVM::get_data_from_memory(...) function; said function will operate appropriately for the short addressing mode

	*/

	bool is_short;
	if (addressing_mode >= addressingmode::absolute_short) {
		is_short = true;
		addressing_mode -= addressingmode::absolute_short;
	}
	else {
		is_short = false;
	}

	// check our addressing mode and decide how to interpret our data
	if ((addressing_mode == addressingmode::absolute) || (addressing_mode == addressingmode::x_index) || (addressing_mode == addressingmode::y_index)) {
		// if we have absolute or x/y indexed-addressing, we will be reading from memory
		uint16_t data_in_memory = 0;

		// however, we need to make sure that if it is indexed, we add the register values to data_to_load, which contains the memory address, so we actually perform the indexing
		if (addressing_mode == addressingmode::x_index) {
			// x indexed -- add REG_X to data_to_load
			data_to_load += REG_X;
		}
		else if (addressing_mode == addressingmode::y_index) {
			// y indexed
			data_to_load += REG_Y;
		}

		// fetch the data in memory; if the address is invalid, get_data_from_memory will generate a signal
		data_in_memory = this->get_data_from_memory(data_to_load, is_short);

		// load our target register with the data we fetched
		return data_in_memory;
	}
	else if (addressing_mode == addressingmode::immediate) {
		// if we are using immediate addressing, data_to_load is the data we want to load
		// simply assign it to our target register and return
		return data_to_load;
	}

	/*

	For these examples, let's say we have this memory map to work with:
			$0000	-	#$00
			$0000	-	#$00
			$0002	-	#$23
			$0003	-	#$C0
			...
			$23C0	-	#$AB
			$23C1	-	#$CD
			$23C2	-	#$12
			$23C3	-	#$34

	Indexed indirect modes go to some address, index it with X or Y, go to the address equal to that value, and fetch that. This acts as a pointer to a pointer, essentially. For example:
			loadx #$02
			loada ($00, x)
	will cause A to look at address $00, x, which contains the word $23C0; it then fetches the address at location $23C0, which is #$ABCD

	Our indirect indexed modes are like indexed indirect, but less insane; they get the value at the address and use that as a memory location to access, finally indexing with the X or Y registers. Essentially, this acts as a pointer. For example:
			loady #$02
			loada ($02), y
	will cause A to look at address $02, which contains the word $23C0; it then goes to that address and indexes it with the y register, so it points to address $23C2, which contains the word #$1234

	Both addressing modes can be used with the X and Y registers

	*/

	// indirect indexed
	else if (addressing_mode == addressingmode::indirect_indexed_x) {
		// indirect indexed addressing with the X register

		// get the data at the address indicated by data_to_load
		uint16_t data_in_memory = this->get_data_from_memory(data_to_load);	// get the whole word (as it's an address), so don't use short addressing
		// now go to that address + reg_x and return the data there
		return this->get_data_from_memory(data_in_memory + REG_X, is_short);	// we _may_ want short addressing here, so pass the is_short flag
	}
	else if (addressing_mode == addressingmode::indirect_indexed_y) {
		// indirect indexed addressing with the Y register
		uint16_t data_in_memory = this->get_data_from_memory(data_to_load);	// get the whole word (as it's an address), so don't use short addressing
		return this->get_data_from_memory(data_in_memory + REG_Y, is_short);	// we _may_ want short addressing here, so pass the is_short flag
	}

	// indexed indirect
	else if (addressing_mode == addressingmode::indexed_indirect_x) {
		// go to data_to_load + X, get the value there, go to that address, and return the value stored there
		uint16_t data_in_memory = this->get_data_from_memory(data_to_load + REG_X);	// get the whole word (as it's an address), so don't use short addressing
		return this->get_data_from_memory(data_in_memory, is_short);	// we _may_ want short addressing here, so pass the is_short flag
	}
	else if (addressing_mode == addressingmode::indexed_indirect_y) {
		uint16_t data_in_memory = this->get_data_from_memory(data_to_load + REG_Y);	// get the whole word (as it's an address), so don't use short addressing
		return this->get_data_from_memory(data_in_memory, is_short);	// we _may_ want short addressing here, so pass the is_short flag
	}
}


void SINVM::execute_store(uint16_t reg_to_store) {
	// our next byte is the addressing mode
	this->PC++;
	uint8_t addressing_mode = this->memory[this->PC];

	// increment the program counter
	this->PC++;

	// next, get the memory location
	uint16_t memory_address = this->get_data_of_wordsize();

	/*

	Now, check to see if we are using short addressing. If so, subtract 0x10 (base for short addressing) from the addressing mode, set the is_short flag, and proceed. We will then pass is_short into the actual memory store function

	*/

	bool is_short;
	if (addressing_mode >= addressingmode::absolute_short) {
		is_short = true;
		addressing_mode -= addressingmode::absolute_short;
	}
	else {
		is_short = false;
	}

	// act according to the addressing mode
	if ((addressing_mode != addressingmode::immediate) && (addressing_mode != addressingmode::reg_a) && (addressing_mode != addressingmode::reg_b)) {
		// add the appropriate register if it is an indexed addressing mode
		if ((addressing_mode == addressingmode::x_index) || (addressing_mode == addressingmode::indirect_indexed_x)) {
			// since we will index after, we can include indirect indexed here -- if it's indirect, we simply get that value before we index
			if (addressing_mode == addressingmode::indirect_indexed_x) {
				memory_address = this->get_data_from_memory(memory_address);
			}
			// index the memory address with X
			memory_address += this->REG_X;
		}
		else if ((addressing_mode == addressingmode::y_index) || (addressing_mode == addressingmode::indirect_indexed_y)) {
			if (addressing_mode == addressingmode::indirect_indexed_y) {
				memory_address = this->get_data_from_memory(memory_address);
			}
			memory_address += this->REG_Y;
		}
		else if (addressing_mode == addressingmode::indexed_indirect_x) {
			memory_address = this->get_data_from_memory(memory_address + REG_X);
		}
		else if (addressing_mode == addressingmode::indexed_indirect_y) {
			memory_address = this->get_data_from_memory(memory_address + REG_Y);
		}

		// if the memory address is not valid, store_in_memory will generate a signal
		this->store_in_memory(memory_address, reg_to_store, is_short);

		return;
	}
	else if (addressing_mode == addressingmode::immediate) {
		// back up the PC by three bytes as we have already read data
		this->PC -= 3;
		this->send_signal(SINSIGILL);	// illegal instruction; cannot use immediate addressing with a storeR instruction
	}
}


uint16_t SINVM::get_data_from_memory(uint16_t address, bool is_short) {
	// read value of _WORDSIZE in memory and return it in the form of an int
	// different from "execute_load()" in that it doesn't affect the program counter and does not take addressing mode into consideration

	if (address_is_valid(address)) {
		uint16_t data = 0;

		// if we are using the short addressing mode, get the individual byte
		if (is_short) {
			data = this->memory[address];
		}
		// otherwise, get the whole word
		else {
			size_t wordsize_bytes = this->_WORDSIZE / 8;

			size_t memory_index = 0;
			size_t bitshift_index = (wordsize_bytes - 1);

			while (memory_index < (wordsize_bytes - 1)) {
				data += this->memory[address + memory_index];
				data <<= (bitshift_index * 8);

				memory_index++;
				bitshift_index--;
			}
			data += this->memory[address + memory_index];
		}

		return data;
	}
	// if we have an invalid memory address, we have an access violation
	else {
		this->send_signal(SINSIGSEGV);
		return 0xFFFF;
	}
}

void SINVM::store_in_memory(uint16_t address, uint16_t new_value, bool is_short) {
	// stores value "new_value" in memory address starting at "high_byte_address"

	/*

	Make the assignment and return

	If we are using short addressing, we set the individual address to the new value and return

	Otherwise, if we are using the whole word:
	We can't do this quite the same way here as we did in the assembler; because our memory address needs to start low, and our bit shift needs to start high, we have to do a little trickery to get it right -- let's use 32-bit wordsize assigning to location $00 as an example:
	- First, we iterate from i=_WORDSIZE/8, so i=4
	- Now, we assign the value at address [memory_address + (wordsize_bytes - i)], so we assign to value memory_address, or $00
	- The value to assign is our value bitshifted by (i - 1) * 8; here, 24 bits; so, the high byte
	- On the next iteration, i = 3; so we assign to location [$00 + (4 - 3)], or $01, and we assign the value bitshifted by 16 bits
	- On the next iteration, i = 2; so we assign to location [$00 + (4 - 2)], or $02, and we assign the value bitshifted by 8 bits
	- On the final iteration, i = 1; so we assign to location [$00 + (4 - 1)], $03, and we assign the value bitshifted by (1 - 1); the value to assign is then reg_to_store >> 0, which is the low byte
	- That means we have the bytes ordered correctly in big-endian format

	*/

	// if we have a valid address, we are allowed to store the data in memory; otherwise, we have an access violation
	if (address_is_valid(address)) {
		if (is_short) {
			this->memory[address] = new_value;
		}
		else {
			size_t wordsize_bytes = this->_WORDSIZE / 8;
			for (size_t i = wordsize_bytes; i > 0; i--) {
				this->memory[address + (wordsize_bytes - i)] = new_value >> ((i - 1) * 8);
			}
		}
	}
	else {
		this->send_signal(SINSIGSEGV);
	}

	return;
}
