#include "SINVM.h"


// TODO: move error catching in syntax to the Assembler class; it doesn't really belong here


const bool SINVM::address_is_valid(size_t address) {
	// checks to see if the address we want to use is within 0 and memory_size

	return ((address >= 0) && (address < memory_size));
}


int SINVM::get_data_of_wordsize() {
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
	int data_to_load = 0;

	// loop one time too few
	for (int i = 1; i < this->_WORDSIZE / 8; i++) {
		data_to_load = data_to_load + this->memory[this->PC];
		data_to_load = data_to_load << 8;
		this->PC++;
	}
	// and put the data in on the last time -- this is to prevent incrementing PC too much or bitshifting too far
	data_to_load = data_to_load + this->memory[this->PC];

	return data_to_load;
}

std::vector<uint8_t> SINVM::get_properly_ordered_bytes(int value) {
	std::vector<uint8_t> ordered_bytes;
	for (int i = (this->_WORDSIZE / 8); i > 0; i--) {
		ordered_bytes.push_back(value >> ((i - 1) * 8));
	}
	return ordered_bytes;
}


void SINVM::execute_instruction(int opcode) {
	/* 
	
	Execute a single instruction. Each instruction will increment or set the PC according to what it needs to do for that particular instruction. This function delegates the task of handling instruction execution to many other functions to make the code more maintainable and easier to understand.

	*/

	// use a switch statement; compiler may be able to optimize it more easily and it will be easier to read
	switch (opcode) {
		case HALT:
			// if we get a HALT command, we want to set the H flag, which will stop the VM in its main loop
			this->set_status_flag('H');
			break;
		case NOOP:
			// do nothing
			break;

		// load/store registers
		// all cases use the execute_load() and execute_store() functions, just on different registers
		case LOADA:
			REG_A = this->execute_load();
			break;
		case STOREA:
			this->execute_store(REG_A);
			break;

		case LOADB:
			REG_B = this->execute_load();
			break;
		case STOREB:
			this->execute_store(REG_B);
			break;

		case LOADX:
			REG_X = this->execute_load();
			break;
		case STOREX:
			this->execute_store(REG_X);
			break;
	
		case LOADY:
			REG_Y = this->execute_load();
			break;
		case STOREY:
			this->execute_store(REG_Y);
			break;

		// carry flag
		case CLC:
			this->clear_status_flag('C');
			break;
		case SEC:
			this->set_status_flag('C');
			break;

		// ALU instructions
		// For these, we will use our load function to get the data we want in addition to the A register; we won't put it in the a register, but we will use the function because it will give it to us no problem
		case ADDCA:
		{
			int addend = this->execute_load();

			// add the fetched data to A
			REG_A += addend;
			break;
		}
		case SUBCA:
		{
			// in subtraction, REG_A is the minuend and the value supplied is the subtrahend
			int subtrahend = this->execute_load();

			if (subtrahend > REG_A) {
				this->set_status_flag('N');	// set the N flag if the result is negative, which will be the case if subtrahend > REG_A
			}
			else if (subtrahend == REG_A) {
				this->set_status_flag('Z');	// set the Z flag if the two are equal, as the result will be 0
			}

			REG_A -= subtrahend;
			break;
		}
		case ANDA:
		{
			int and_value = this->execute_load();

			REG_A = REG_A & and_value;
			break;
		}
		case ORA:
		{
			int or_value = this->execute_load();

			REG_A = REG_A | or_value;
			break;
		}
		case XORA:
		{
			int xor_value = this->execute_load();

			REG_A = REG_A ^ xor_value;
			break;
		}
		case LSR: case LSL: case ROR: case ROL:
		{
			this->execute_bitshift(opcode);
			break;
		}

		// Incrementing / decrementing registers
		case INCA:
			this->REG_A = this->REG_A + 1;
			break;
		case DECA:
			this->REG_A -= 1;
			break;
		case INCX:
			this->REG_X += 1;
			break;
		case DECX:
			this->REG_X -= 1;
			break;
		case INCY:
			this->REG_Y += 1;
			break;
		case DECY:
			this->REG_Y -= 1;
			break;
		// Note that INCSP and DECSP modify by one /word/, not one byte (like other increment instructions do)
		case INCSP:
			// increment by one word
			if (this->SP < (uint16_t)_STACK) {
				this->SP += (this->_WORDSIZE / 8);
			}
			else {
				throw std::runtime_error("**** Runtime error: Stack underflow!");
			}
			break;
		case DECSP:
			// decrement by one word
			if (this->SP > (uint16_t)_STACK_BOTTOM) {
				this->SP -= (this->_WORDSIZE / 8);
			}
			else {
				throw std::runtime_error("**** Runtime error: Stack overflow!");
			}
			break;

		// Comparatives

			// TODO: write comparatives

		case CMPA:
			this->execute_comparison(REG_A);
			break;
		case CMPB:
			this->execute_comparison(REG_B);
			break;
		case CMPX:
			this->execute_comparison(REG_X);
			break;
		case CMPY:
			this->execute_comparison(REG_Y);
			break;

		// Branch and control flow logic
		case JMP:
			this->execute_jmp();
			break;
		case BRNE:
			// if the comparison was unequal, the Z flag will be clear
			if (!this->is_flag_set('Z')) {
				// if it's set, execute a jump
				this->execute_jmp();
			}
			else {
				// skip past the addressnig mode and address if it isn't set
				// we need to skip past 3 bytes
				this->PC += 3;
			}
			break;
		case BREQ:
			// if the comparison was equal, the Z flag will be set
			if (this->is_flag_set('Z')) {
				this->execute_jmp();
			}
			else {
				// skip past the addressing mode and address if it isn't set
				this->PC += 3;
			}
			break;
		case BRGT:
			// the carry flag will be set if the value is greater than what we compared it to
			if (this->is_flag_set('C')) {
				this->execute_jmp();
			}
			else {
				// skip past data we don't need
				this->PC += 3;
			}
			break;
		case BRLT:
			// the carry flag will be clear if the value is less than what we compared it to
			if (!this->is_flag_set('C')) {
				this->execute_jmp();
			}
			else {
				this->PC += 3;
			}
			break;
		case BRZ:
			// branch on zero; if the Z flag is set, branch; else, continue

			// TODO: delete this? do we really need it? it's equal to branch if equal...

			if (this->is_flag_set('Z')) {
				this->execute_jmp();
			}
			else {
				this->PC += 3;
			}
			break;
		case JSR:
		{
			// get the addressing mode
			this->PC++;
			uint8_t addressing_mode = this->memory[this->PC];

			// get the value
			this->PC++;
			int address_to_jump = this->get_data_of_wordsize();

			int return_address = this->PC;

			// if the CALL_SP is greater than _CALL_STACK_BOTTOM, we have not written past the end of it
			if (this->CALL_SP >= _CALL_STACK_BOTTOM) {
				// memory size is 16 bits but...do this anyways
				for (int i = 0; i < (this->_WORDSIZE / 8);  i++) {
					uint8_t memory_byte = return_address >> (i * 8);
					this->memory[CALL_SP] = memory_byte;
					this->CALL_SP--;
				}
			}
			else {
				throw std::runtime_error("**** Runtime error: Stack overflow on call stack!");
			}

			this->PC = address_to_jump - 1;

			break;
		}
		case RTS:
		{
			int return_address = 0;
			if (this->CALL_SP <= _CALL_STACK) {
				for (int i = (this->_WORDSIZE / 8); i > 0; i--) {
					CALL_SP++;
					return_address += memory[CALL_SP];
					return_address = return_address << ((i - 1) * 8);
				}
			}
			this->PC = return_address;	// we don't need to offset because the absolute address was pushed to the call stack
			break;
		}

		// Register transfers
		case TBA:
			REG_A = REG_B;
			break;
		case TXA:
			REG_A = REG_X;
			break;
		case TYA:
			REG_A = REG_Y;
			break;
		case TSPA:
			REG_A = (int)SP;	// SP holds the address to which the next element in the stack will go, and is incremented every time something is pushed, and decremented every time something is popped
			break;
		case TAB:
			REG_B = REG_A;
			break;
		case TAX:
			REG_X = REG_A;
			break;
		case TAY:
			REG_Y = REG_A;
			break;
		case TASP:
			SP = (size_t)REG_A;
			break;

		// The stack
		case PHA:
			this->push_stack(REG_A);
			break;
		case PLA:
			REG_A = this->pop_stack();
			break;
		case PHB:
			this->push_stack(REG_B);
			break;
		case PLB:
			REG_B = this->pop_stack();
			break;

		// SYSCALL INSTRUCTION
		// Note the syscall instruction serves many purposes; see "Doc/syscall.txt" for more information
		case SYSCALL:
		{
			// get the addressing mode
			this->PC++;
			uint8_t addressing_mode = this->memory[this->PC];

			// increment the PC to point at the data and get the syscall number
			this->PC++;
			int syscall_number = this->get_data_of_wordsize();

			// TODO: execute system call accordingly

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
				// store those bytes in memory
				// use size() here because we want to index
				for (int i = 0; i < input_bytes.size(); i++) {
					this->memory[start_address + i] = input_bytes[i];
				}

				// finally, store the length of input_bytes (in bytes) in register A
				this->REG_A = input_bytes.size();
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

				// output the string we have created, enclosed in quotes
				std::cout << "\"" <<  output_string << "\"" << std::endl;
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
			// TODO: check for more syscall numbers...
			// if it is not a valid syscall number,
			else {
				throw std::runtime_error("Unknown syscall number; halting execution.");
			}

			break;
		}

		// if we encounter an unknown opcode
		default:
		{
			throw std::runtime_error("Unknown opcode; halting execution.");
			break;
		}
	}

	return;
}


int SINVM::execute_load() {
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

	this->PC++;

	// load the appropriate number of bytes according to our wordsize
	unsigned int data_to_load = this->get_data_of_wordsize();

	// check our addressing mode and decide how to interpret our data
	if ((addressing_mode == addressingmode::absolute) || (addressing_mode == addressingmode::x_index) || (addressing_mode == addressingmode::y_index)) {
		// if we have absolute or x/y indexed-addressing, we will be reading from memory
		int data_in_memory = 0;

		// however, we need to make sure that if it is indexed, we add the register values to data_to_load, which contains the memory address, so we actually perform the indexing
		if (addressing_mode == addressingmode::x_index) {
			// x indexed -- add REG_X to data_to_load
			data_to_load += REG_X;
		}
		else if (addressing_mode == addressingmode::y_index) {
			// y indexed
			data_to_load += REG_Y;
		}

		// check for a valid memory address
		if (!address_is_valid(data_to_load)) {
			// don't throw an exception if the address is out of range; subtract the memory size from data_to_load if it is over the limit until it is within range
			while (data_to_load > memory_size) {
				data_to_load -= memory_size;
			}
		}

		// now, data_to_load definitely contains the correct start address
		data_in_memory = this->get_data_from_memory(data_to_load);

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

	// TODO: implement indirect indexed and indexed indirect addressing

	// indirect indexed
	else if (addressing_mode == addressingmode::indirect_indexed_x) {
		// indirect indexed addressing with the X register
	}
	else if (addressing_mode == addressingmode::indirect_indexed_y) {
		// indirect indexed addressing with the Y register
	}

	// indexed indirect
	else if (addressing_mode == addressingmode::indexed_indirect_x) {

	}
	else if (addressing_mode == addressingmode::indexed_indirect_y) {

	}
}


void SINVM::execute_store(int reg_to_store) {
	// our next byte is the addressing mode
	this->PC++;
	uint8_t addressing_mode = this->memory[this->PC];

	this->PC++;

	// next, get the memory location
	unsigned int memory_address = this->get_data_of_wordsize();

	// validate the memory location
	// TODO: decide whether writing to an invalid location will wrap around or simply throw an exception
	if (!address_is_valid(memory_address)) {
		// as long as it is out of range, continue subtracting the max memory size until it is valid
		while (memory_address > memory_size) {
			memory_address -= memory_size;
		}
	}

	// act according to the addressing mode
	if ((addressing_mode != addressingmode::immediate) && (addressing_mode != addressingmode::reg_a) && (addressing_mode != addressingmode::reg_b)) {
		// add the appropriate register if it is an indexed addressing mode
		if (addressing_mode == addressingmode::x_index) {
			memory_address += this->REG_X;
		}
		else if (addressing_mode == addressingmode::y_index) {
			memory_address += this->REG_Y;
		}

		// TODO: add support for indirect addressing

		// use our store_in_memory function
		this->store_in_memory(memory_address, reg_to_store);

		return;
	}
	else if (addressing_mode == addressingmode::immediate) {
		// we cannot use immediate addressing with a store instruction, so throw an exception
		throw std::runtime_error("Invalid addressing mode for store instruction.");
	}
}

int SINVM::get_data_from_memory(int address) {
	// read value of _WORDSIZE in memory and return it in the form of an int
	// different from "execute_load()" in that it doesn't affect the program counter and does not take addressing mode into consideration

	int data = 0;
	int wordsize_bytes = this->_WORDSIZE / 8;

	for (int i = 0; i < (wordsize_bytes - 1); i++) {
		data += this->memory[address + i];
		data = data << (i * 8);
	}
	data += this->memory[address + (wordsize_bytes - 1)];

	return data;
}

void SINVM::store_in_memory(int address, int new_value) {
	// stores value "new_value" in memory address starting at "high_byte_address"
	
	/*
	Make the assignment and return
	We can't do this quite the same way here as we did in the assembler; because our memory address needs to start low, and our bit shift needs to start high, we have to do a little trickery to get it right -- let's use 32-bit wordsize assigning to location $00 as an example:
	- First, we iterate from i=_WORDSIZE/8, so i=4
	- Now, we assign the value at address [memory_address + (wordsize_bytes - i)], so we assign to value memory_address, or $00
	- The value to assign is our value bitshifted by (i - 1) * 8; here, 24 bits; so, the high byte
	- On the next iteration, i = 3; so we assign to location [$00 + (4 - 3)], or $01, and we assign the value bitshifted by 16 bits
	- On the next iteration, i = 2; so we assign to location [$00 + (4 - 2)], or $02, and we assign the value bitshifted by 8 bits
	- On the final iteration, i = 1; so we assign to location [$00 + (4 - 1)], $03, and we assign the value bitshifted by (1 - 1); the value to assign is then reg_to_store >> 0, which is the low byte
	- That means we have the bytes ordered correctly in big-endian format
	*/

	int wordsize_bytes = this->_WORDSIZE / 8;
	for (int i = wordsize_bytes; i > 0; i--) {
		this->memory[address + (wordsize_bytes - i)] = new_value >> ((i - 1) * 8);
	}

	return;
}

void SINVM::execute_bitshift(int opcode)
{

	// TODO: correct for wordsize -- currently, highest bit is always 7; should be highest bit in the wordsize

	// LOGICAL SHIFTS: 0 always shifted in; bit shifted out goes into carry
	// ROTATIONS: carry bit shifted in; bit shifted out goes into carry

	// check our addressing mode
	this->PC++;
	int addressing_mode = this->memory[this->PC];

	if (addressing_mode != addressingmode::reg_a) {
		int value_at_address;
		uint8_t high_byte_address;
		bool carry_set_before_bitshift = false;

		// increment PC so we can use get_data_of_wordsize()
		this->PC++;
		int address = this->get_data_of_wordsize();

		// if we have absolute addressing
		if (addressing_mode == addressingmode::absolute) {
			high_byte_address = address >> 8;
			value_at_address = this->get_data_from_memory(high_byte_address);

			// get the original status of carry so we can set bits in ROR/ROL instructions later
			carry_set_before_bitshift = this->is_flag_set('C');

			// shift the bit
			if (opcode == LSR || opcode == ROR) {
				value_at_address = value_at_address >> 1;
				
				if (opcode == ROR && carry_set_before_bitshift) {
					// ORing value_at_address with 128 will always set bit 7
					value_at_address = value_at_address | 128;
				}
				else if (opcode == ROR && !carry_set_before_bitshift) {
					// ANDing with 127 will leave every bit unchanged EXCEPT for bit 7, which will be cleared
					value_at_address = value_at_address & 127;
				}
			}
			else if (opcode == LSL || opcode == ROL) {
				value_at_address = value_at_address << 1;

				if (opcode == ROL && carry_set_before_bitshift) {
					// ORing value_at_address with 1 will always set bit 0
					value_at_address = value_at_address | 1;
				}
				else if (opcode == ROL && !carry_set_before_bitshift) {
					// ANDing with 254 will leave every bit unchanged EXCEPT for bit 0, which will be cleared
					value_at_address = value_at_address & 254;
				}
			}

			this->store_in_memory(high_byte_address, value_at_address);
		}
		// if we have x-indexed addressing
		else if (addressing_mode == addressingmode::x_index) {
			address += REG_X;
			high_byte_address = address >> 8;
			value_at_address = this->get_data_from_memory(high_byte_address);

			// get the original status of carry so we can set bits in ROR/ROL instructions later
			carry_set_before_bitshift = this->is_flag_set('C');

			// shift the bit
			if (opcode == LSR || opcode == ROR) {
				value_at_address = value_at_address >> 1;
			}
			else if (opcode == LSL || opcode == ROL) {
				value_at_address = value_at_address << 1;
			}
			this->store_in_memory(high_byte_address, value_at_address);
		}
		// if we have y-indexed addressing
		else if (addressing_mode == addressingmode::y_index) {
			address += REG_Y;
			high_byte_address = address >> 8;
			value_at_address = this->get_data_from_memory(high_byte_address);

			// get the original status of carry so we can set bits in ROR/ROL instructions later
			carry_set_before_bitshift = this->is_flag_set('C');

			// shift the bit
			if (opcode == LSR || opcode == ROR) {
				value_at_address = value_at_address >> 1;
			}
			else if (opcode == LSL || opcode == ROL) {
				value_at_address = value_at_address << 1;
			}

			this->store_in_memory(high_byte_address, value_at_address);
		}
		// if we have indirect addressing (x)
		else if (addressing_mode == addressingmode::indirect_indexed_x) {

			// TODO: enable indirect addressing

		}
		// if we have indirect addressing (y)
		else if (addressing_mode == addressingmode::indirect_indexed_y) {

			// TODO: enable indirect addressing

		}
		// if we have an invalid addressing mode
		// TODO: validate addressing mode in assembler for bitshifting instructions
		else {
			throw std::runtime_error("Cannot use that addressing mode with bitshifting instructions.");
		}

		if (opcode == LSR || opcode == ROR) {
			// if bit 0 was set
			if ((value_at_address & 1) == 1) {
				// set the carry flag
				this->set_status_flag('C');
			}
			// otherwise
			else {
				// clear the carry flag
				this->clear_status_flag('C');
			}
		}
		else if (opcode == LSL || opcode == ROL) {
			if ((value_at_address & 128) == 128) {
				this->set_status_flag('C');
			}
			else {
				this->clear_status_flag('C');
			}
		}
	}
	// if we have register addressing
	else {
		// so we know whether we need to set bit 7 or 0 after shifting
		bool carry_set_before_bitshift = this->is_flag_set('C');

		if (opcode == LSR || opcode == ROR) {
			// now, set carry according to the current bit 7 or 0
			if ((REG_A & 1) == 1) {
				this->set_status_flag('C');
			}
			else {
				this->clear_status_flag('C');
			}
			// shift the bit right
			REG_A = REG_A >> 1;

			if (opcode == ROR && carry_set_before_bitshift) {
				// ORing with 128 will always set bit 7
				REG_A = REG_A | 128;
			}
			else if (opcode == ROR && !carry_set_before_bitshift) {
				// ANDing with 127 will always clear bit 7
				REG_A = REG_A & 127;
			}
		}
		else if (opcode == LSL || opcode == ROL) {
			// now, set carry according to the current bit 7 or 0
			if ((REG_A & 1) == 1) {
				this->set_status_flag('C');
			}
			else {
				this->clear_status_flag('C');
			}
			// shift the bit left
			REG_A = REG_A << 1;

			if (opcode == ROL) {
				// ORing with 1 will always set bit 1
				REG_A = REG_A | 1;
			}
			else if (opcode == ROL && !carry_set_before_bitshift) {
				// ANDing with 254 will always clear bit 1
				REG_A = REG_A & 254;
			}
		}
	}
}


void SINVM::execute_comparison(int reg_to_compare) {
	int to_compare = this->execute_load();

	// if the values are equal, set the Z flag; if they are not, clear it
	if (reg_to_compare == to_compare) {
		this->set_status_flag('Z');
	}
	else {
		this->clear_status_flag('Z');
		// we may need to set other flags too
		if (reg_to_compare < to_compare) {
			// set the carry flag if greater than, clear if less than
			this->clear_status_flag('C');
		}
		else {	// if it's not equal, and it's not less, it's greater
			this->set_status_flag('C');
		}
	}
	return;
}


void SINVM::execute_jmp() {
	/*

	Execute a JMP instruction. The VM first increments the PC to get the addressing mode, then increments it again to get the memory address

	*/

	this->PC++;
	uint8_t addressing_mode = this->memory[this->PC];

	// get the memory address to which we want to jump
	this->PC++;
	int memory_address = this->get_data_of_wordsize();

	// check our addressing mode to see how we need to handle the data we just received
	if (addressing_mode == addressingmode::absolute) {
		// if absolute, set it to memory address - 1 so when it gets incremented at the end of the instruction, it's at the proper address
		this->PC = memory_address - 1;
		return;
	}
	else if (addressing_mode == addressingmode::x_index) {
		memory_address += REG_X;
		this->PC = memory_address - 1;
	}
	else if (addressing_mode == addressingmode::y_index) {
		memory_address += REG_Y;
		this->PC = memory_address - 1;
	}

	// TODO: Indirect indexed ?

	else {
		throw std::runtime_error("Invalid addressing mode for JMP instruction.");
	}

	return;
}


// Stack functions -- these always use register A, so no point in having parameters
void SINVM::push_stack(int reg_to_push) {
	// push the current value in "reg_to_pus" (A or B) onto the stack, decrementing the SP (because the stack grows downwards)

	// first, make sure the stack hasn't hit its bottom
	if (this->SP < _STACK_BOTTOM) {
		throw std::runtime_error("**** ERROR: Stack overflow.");
	}

	for (int i = (this->_WORDSIZE / 8); i > 0; i--) {
		this->memory[SP] = reg_to_push >> ((i - 1) * 8);
		this->SP--;
	}

	return;
}

int SINVM::pop_stack() {
	// pop the most recently pushed value off the stack; this means we must increment the SP (as the current value pointed to by SP is the one to which we will write next; also, the stack grows downwards so we want to increase the address if we pop something off), and then dereference; this means this area of memory is the next to be written to if we push something onto the stack

	// first, make sure we aren't going beyond the bounds of the stack
	if (this->SP > _STACK) {
		throw std::runtime_error("**** ERROR: Stack underflow.");
	}

	// for each byte in a word, we must increment the stack pointer by one byte (increments BEFORE reading, as the SP points to the next AVAILABLE byte), get the data, shifted over according to what byte number we are on--remember we are reading the data back in little-endian byte order, not big, so we shift BEFORE we add
	int stack_data = 0;
	for (int i = 0; i < (this->_WORDSIZE / 8); i++) {
		this->SP++;
		stack_data += (this->memory[SP] << (i * 8));
	}
	// because we have added 1 too few bytes, we must add the dereferenced pointer to stack_data and increment the stack pointer again
	/*stack_data += this->memory[SP];
	this->SP++;*/

	// finally, return the stack data
	return stack_data;
}



void SINVM::set_status_flag(char flag) {
	// sets the flag equal to 'flag' in the status register
	
	/*
	Reminder of the layout of the status register (bit number / flag):
		7	6	5	4	3	2	1	0
		N	V	0	H	0	0	Z	C
	
	Flag meanings:
		N: Negative
		V: Overflow
		H: HALT instruction executed
		Z: Zero
		C: Carry
	*/

	if (flag == 'N') {
		this->STATUS = 128;
	}
	else if (flag == 'V') {
		this->STATUS = 64;
	}
	else if (flag == 'H') {
		this->STATUS = 16;
	}
	else if (flag == 'Z') {
		this->STATUS = 2;
	}
	else if (flag == 'C') {
		this->STATUS = 1;
	}

	// this should never throw an exception

	return;
}

void SINVM::clear_status_flag(char flag) {
	// sets the flag equal to 'flag' in the status register

	/*
	Reminder of the layout of the status register (bit number / flag):
	7	6	5	4	3	2	1	0
	N	V	0	H	0	0	Z	C

	Flag meanings:
	N: Negative
	V: Overflow
	H: HALT instruction executed
	Z: Zero
	C: Carry
	*/

	if (flag == 'N') {
		this->STATUS = this->STATUS & 0b01111111;
	}
	else if (flag == 'V') {
		this->STATUS = this->STATUS & 0b10111111;
	}
	else if (flag == 'H') {
		this->STATUS = this->STATUS & 0b11101111;
	}
	else if (flag == 'Z') {
		this->STATUS = this->STATUS & 0b11111101;
	}
	else if (flag == 'C') {
		this->STATUS = this->STATUS & 0b11111110;
	}

	// this should never throw an exception

	return;
}

uint8_t SINVM::get_processor_status() {
	return this->STATUS;
}

bool SINVM::is_flag_set(char flag) {
	// return the status of a single flag in the status register
	// do this by performing an AND operation on that bit; if set, we will get true (the only bit that can be set is the bit in question)
	if (flag == 'N') {
		return this->STATUS & 128; // will only return true if N is set; if any other bit is set, it will be AND'd against a 0
	}
	else if (flag == 'V') {
		return this->STATUS & 64;
	}
	else if (flag == 'H') {
		return this->STATUS & 16;
	}
	else if (flag == 'Z') {
		return this->STATUS & 2;
	}
	else if (flag == 'C') {
		return this->STATUS & 1;
	}

	// this should never throw an exception
}


void SINVM::run_program() {
	// as long as the H flag (HALT) is not set (the VM has not received the HALT signal)
	while (!(this->is_flag_set('H'))) {
		// execute the instruction pointed to by the program counter
		this->execute_instruction(this->memory[this->PC]);

		// advance the program counter to point to the next instruction
		this->PC++;
	}

	return;
}



void SINVM::_debug_values() {
	std::cout << "SINVM Values:" << std::endl;
	std::cout << "\t" << "Registers:" << "\n\t\tA: $" << std::hex << this->REG_A << std::endl;
	std::cout << "\t\tB: $" << std::hex << this->REG_B << std::endl;
	std::cout << "\t\tX: $" << std::hex << this->REG_X << std::endl;
	std::cout << "\t\tY: $" << std::hex << this->REG_Y << std::endl;
	std::cout << "\t\tSTATUS: $" << std::hex << (int)this->STATUS << std::endl << std::endl;

	std::cout << "Memory: " << std::endl;
	for (int i = 0; i < 10; i++) {
		// display the first 10 addresses from each of the first two pages of memory
		std::cout << "\t$000" << i << ": $" << std::hex << (int)this->memory[i] << "\t\t$0" << 256 + i << ": $" << (int)this->memory[256 + i] << std::endl;
	}

	std::cout << std::endl;
}



SINVM::SINVM(std::istream& file)
{
	// TODO: redo the SINVM constructor...
	this->_WORDSIZE = BinaryIO::readU8(file);

	size_t prg_size = (size_t)BinaryIO::readU32(file);
	std::vector<uint8_t> prg_data;

	for (size_t i = 0; i < prg_size; i++) {
		uint8_t next_byte = BinaryIO::readU8(file);
		prg_data.push_back(next_byte);
	}

	// copy our instructions into memory so that the program is at the end of memory
	this->program_start_address = _PRG_BOTTOM;	// the start address is $2600

	// if the size of the program is greater than 0xF000 - 0x2600, it's too big
	if (prg_data.size() > (_PRG_TOP - _PRG_BOTTOM)) {
		throw std::runtime_error("Program too large for conventional memory map!");

		// remap memory instead?

	}

	// copy the program data into memory
	std::vector<uint8_t>::iterator instruction_iter = prg_data.begin();
	size_t memory_index = this->program_start_address;

	while ((instruction_iter != prg_data.end())) {
		this->memory[memory_index] = *instruction_iter;
		memory_index++;
		instruction_iter++;
	}

	// initialize the stack to location to our stack's upper limit; it grows downwards
	this->SP = _STACK;
	this->CALL_SP = _CALL_STACK;

	// always initialize our status register so that all flags are not set
	this->STATUS = 0;

	// initialize the program counter to start at the top of the program
	if (prg_data.size() != 0) {
		this->PC = this->program_start_address;	// make sure that we don't read memory that doesn't exist if our program is empty
	}
	else {
		// throw an exception; the VM cannot execute an empty program
		throw std::runtime_error("Cannot execute an empty program; program size must be > 0");
	}
}

SINVM::~SINVM()
{
}
