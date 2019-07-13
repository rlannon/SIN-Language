/*

SIN Toolchain
SINVM.cpp
Copyright 2019 Riley Lannon

Contains the implementation of various functions related to the VM.

*/


#include "SINVM.h"


const bool SINVM::address_is_valid(size_t address, bool privileged) {
	/*
	
	Checks to see whether the address may be accessed by the program.
	If we set the privileged flag, we may write to any area of memory except the word at 0x0000
	Otherwise, we may not write to:
		- the call stack
		- program memory
	Generally, we will not be checking for valid addresses with the privileged flag set, but there are instances where we may want to

	*/

	bool valid = (address >= 0x0002) && (address < _MEMORY_MAX);
	
	if (privileged) {
		return valid;
	}
	else {
		return (
			valid &&
			(address < _CALL_STACK_BOTTOM) &&	// make sure the address isn't within the call stack
			(address > _CALL_STACK) &&
			(address < _PRG_BOTTOM)		// make sure the address isn't within program memory
			);
	}
}

std::vector<uint8_t> SINVM::get_properly_ordered_bytes(uint16_t value) {
	std::vector<uint8_t> ordered_bytes;
	for (uint8_t i = (this->_WORDSIZE / 8); i > 0; i--) {
		ordered_bytes.push_back(value >> ((i - 1) * 8));
	}
	return ordered_bytes;
}

void SINVM::execute_bitshift(uint16_t opcode)
{

	// TODO: correct for wordsize -- currently, highest bit is always 7; should be highest bit in the wordsize

	// LOGICAL SHIFTS: 0 always shifted in; bit shifted out goes into carry
	// ROTATIONS: carry bit shifted in; bit shifted out goes into carry

	// check our addressing mode
	this->PC++;
	uint8_t addressing_mode = this->memory[this->PC];

	if (addressing_mode != addressingmode::reg_a) {
		uint16_t value_at_address;
		uint8_t high_byte_address;
		bool carry_set_before_bitshift = false;

		// increment PC so we can use get_data_of_wordsize()
		this->PC++;
		uint16_t address = this->get_data_of_wordsize();

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
			throw VMException("Cannot use that addressing mode with bitshifting instructions.", this->PC);
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


void SINVM::execute_comparison(uint16_t reg_to_compare) {
	// fetch the data for the comparison
	uint16_t to_compare = this->execute_load();

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
	uint16_t memory_address = this->get_data_of_wordsize();

	// check our addressing mode to see how we need to handle the data we just received
	if (addressing_mode == addressingmode::absolute) {
		// if absolute, set it to memory address - 1 so when it gets incremented at the end of the instruction, it's at the proper address
		this->PC = memory_address - 1;
		return;
	}
	// indexed
	else if (addressing_mode == addressingmode::x_index) {
		memory_address += REG_X;
		this->PC = memory_address - 1;
	}
	else if (addressing_mode == addressingmode::y_index) {
		memory_address += REG_Y;
		this->PC = memory_address - 1;
	}
	// indexed indirect modes -- first, add the register value, get the value stored at that address, then use that fetched value as the memory location
	else if (addressing_mode == addressingmode::indexed_indirect_x) {
		size_t address_of_value = memory_address + REG_X;
		
		memory_address = this->get_data_from_memory(address_of_value);
		this->PC = memory_address - 1;
	}
	else if (addressing_mode == addressingmode::indexed_indirect_y) {
		size_t address_of_value = memory_address + REG_Y;

		memory_address = this->get_data_from_memory(address_of_value);
		this->PC = memory_address - 1;
	}
	// indirect indexed modes -- first, get the value stored at memory_address, then add the register index
	else if (addressing_mode == addressingmode::indirect_indexed_x) {
		/*
		
		First, we update the value of memory_address -- e.g., if memory_address was $1234:
			if $1234, $1235 had the value $23, $C0, we would get the value at $1234 -- $23C0
			then, we set memory_address to that value -- 23C0
			then, we add the value at the X or Y register
		
		*/
		
		memory_address = this->get_data_from_memory(memory_address);
		memory_address += REG_X;

		this->PC = memory_address - 1;
	}
	else if (addressing_mode == addressingmode::indirect_indexed_y) {
		memory_address = this->get_data_from_memory(memory_address);
		memory_address += REG_Y;

		this->PC = memory_address - 1;
	}
	// invalid addressing modes will generate a SINSIGILL signal
	else {
		this->PC -= 3;	// we already read the data; back up 3 bytes
		this->send_signal(SINSIGILL);	// illegal to use the supplied addressing mode with the current instruction
	}

	return;
}


// STATUS register operations
void SINVM::set_status_flag(char flag) {
	// sets the flag equal to 'flag' in the status register

	if (flag == 'N') {
		this->STATUS |= StatusConstants::negative;
	}
	else if (flag == 'V') {
		this->STATUS |= StatusConstants::overflow;
	}
	else if (flag == 'U') {
		this->STATUS |= StatusConstants::undefined;
	}
	else if (flag == 'H') {
		this->STATUS |= StatusConstants::halt;
	}
	else if (flag == 'I') {
		this->STATUS |= StatusConstants::interrupt;
	}
	else if (flag == 'F') {
		this->STATUS |= StatusConstants::floating_point;
	}
	else if (flag == 'Z') {
		this->STATUS |= StatusConstants::zero;
	}
	else if (flag == 'C') {
		this->STATUS |= StatusConstants::carry;
	}
	else {
		throw std::runtime_error("Invalid STATUS flag selection");	// throw an exception if we call the function with an invalid flag
	}

	return;
}

void SINVM::clear_status_flag(char flag) {
	// sets the flag equal to 'flag' in the status register

	if (flag == 'N') {
		this->STATUS = this->STATUS & (255 - StatusConstants::negative);
	}
	else if (flag == 'V') {
		this->STATUS = this->STATUS & (255 - StatusConstants::overflow);
	}
	else if (flag == 'U') {
		this->STATUS = this->STATUS & (255 - StatusConstants::undefined);
	}
	else if (flag == 'H') {
		this->STATUS = this->STATUS & (255 - StatusConstants::halt);
	}
	else if (flag == 'I') {
		this->STATUS = this->STATUS & (255 - StatusConstants::interrupt);
	}
	else if (flag == 'F') {
		this->STATUS = this->STATUS & (255 - StatusConstants::floating_point);
	}
	else if (flag == 'Z') {
		this->STATUS = this->STATUS & (255 - StatusConstants::zero);
	}
	else if (flag == 'C') {
		this->STATUS = this->STATUS & (255 - StatusConstants::carry);
	}
	else {
		throw std::runtime_error("Invalid STATUS flag selection");	// throw an exception if we call the function with an invalid flag
	}

	return;
}

uint8_t SINVM::get_processor_status() {
	return this->STATUS;
}

bool SINVM::is_flag_set(char flag) {
	// TODO: add new flags

	// return the status of a single flag in the status register
	// do this by performing an AND operation on that bit; if set, we will get true (the only bit that can be set is the bit in question)
	if (flag == 'N') {
		return this->STATUS & StatusConstants::negative; // will only return true if N is set; if any other bit is set, it will be AND'd against a 0
	}
	else if (flag == 'V') {
		return this->STATUS & StatusConstants::overflow;
	}
	else if (flag == 'U') {
		return this->STATUS & StatusConstants::undefined;
	}
	else if (flag == 'H') {
		return this->STATUS & StatusConstants::halt;
	}
	else if (flag == 'I') {
		return this->STATUS & StatusConstants::interrupt;
	}
	else if (flag == 'F') {
		return this->STATUS & StatusConstants::floating_point;
	}
	else if (flag == 'Z') {
		return this->STATUS & StatusConstants::zero;
	}
	else if (flag == 'C') {
		return this->STATUS & StatusConstants::carry;
	}
	else {
		throw std::runtime_error("Invalid STATUS flag selection");	// throw an exception if we call the function with an invalid flag
	}
}


void SINVM::run_program() {
	// as long as the HALT flag is not set
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
	std::cout << "\t\tSP: $" << std::hex << this->SP << std::endl;
	std::cout << "\t\tSTATUS: $" << std::hex << (uint16_t)this->STATUS << std::endl << std::endl;

	std::cout << "Memory: " << std::endl;
	for (size_t i = 0; i < 0xFF; i++) {
		// display the first two pages of memory
		std::cout << "\t$000" << i << ": $" << std::hex << (uint16_t)this->memory[i] << "\t\t$0" << 0x100 + i << ": $" << (uint16_t)this->memory[256 + i] << std::endl;
	}

	std::cout << "\nStack: " << std::endl;
	for (size_t i = 0xff; i > 0x00; i--) {
		// display the top page of the stack
		if (i > 0x0f) {
			std::cout << "\t$23" << i << ": $" << std::hex << (int)this->memory[0x2300 + i] << std::endl;
		}
		else {
			std::cout << "\t$230" << i << ": $" << std::hex << (int)this->memory[0x2300 + i] << std::endl;
		}
	}

	std::cout << std::endl;
}



SINVM::SINVM(std::istream& file)
{
	// TODO: redo the SINVM constructor...

	// initialize our ALU and FPU
	this->alu = ALU(&this->REG_A, &this->REG_B, &this->STATUS);
	this->fpu = FPU(&this->REG_A, &this->REG_B, &this->STATUS);

	// initialize some memory addresses
	for (size_t i = 0; i < 8; i++) {
		this->memory[_SIG_VECTOR + i] = 0;	// initialize all signal vector data to 0 to start
	}

	// get the wordsize, make sure it is compatible with this VM
	uint8_t file_wordsize = BinaryIO::readU8(file);
	if (file_wordsize != _WORDSIZE) {
		throw VMException("Incompatible word sizes; the VM uses a " + std::to_string(_WORDSIZE) + "-bit wordsize; file to execute uses a " + std::to_string(file_wordsize) + "-bit word.");
	}

	size_t prg_size = (size_t)BinaryIO::readU32(file);
	std::vector<uint8_t> prg_data;

	for (size_t i = 0; i < prg_size; i++) {
		uint8_t next_byte = BinaryIO::readU8(file);
		prg_data.push_back(next_byte);
	}

	// if the size of the program is greater than 0xF000 - 0x2600, it's too big
	if (prg_data.size() > (_PRG_TOP - _PRG_BOTTOM)) {
		throw VMException("Program too large for conventional memory map!");

		// TODO: remap memory if the program is too large instead of exiting?

	}

	// copy the program data into memory
	std::vector<uint8_t>::iterator instruction_iter = prg_data.begin();
	size_t memory_index = _PRG_BOTTOM;
	while ((instruction_iter != prg_data.end())) {
		this->memory[memory_index] = *instruction_iter;
		memory_index++;
		instruction_iter++;
	}

	// initialize our list of dynamic objects as an empty list
	this->dynamic_objects = {};

	// initialize the stack to location to our stack's upper limit; it grows downwards
	this->SP = _STACK;
	this->CALL_SP = _CALL_STACK;

	// always initialize our status register so that all flags are not set
	this->STATUS = 0;

	// Set the word at 0x00 to 0 so that null pointers will not ever be valid
	this->memory[0] = 0;
	this->memory[1] = 0;

	// initialize the program counter to start at the top of the program
	if (prg_data.size() != 0) {
		this->PC = _PRG_BOTTOM;	// make sure that we don't read memory that doesn't exist if our program is empty
	}
	else {
		// throw an exception; the VM cannot execute an empty program
		throw VMException("Cannot execute an empty program; program size must be > 0");
	}
}

SINVM::~SINVM()
{
}
