#include "SINVM.h"


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
		data_to_load = data_to_load + *this->PC;
		data_to_load = data_to_load << 8;
		this->PC++;
	}
	// and put the data in on the last time -- this is to prevent incrementing PC too much or bitshifting too far
	data_to_load = data_to_load + *this->PC;

	return data_to_load;
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
			this->execute_load(&REG_A);
			break;
		case STOREA:
			this->execute_store(REG_A);
			break;

		case LOADB:
			this->execute_load(&REG_B);
			break;
		case STOREB:
			this->execute_store(REG_B);
			break;

		case LOADX:
			this->execute_load(&REG_X);
			break;
		case STOREX:
			this->execute_store(REG_X);
			break;
	
		case LOADY:
			this->execute_load(&REG_Y);
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
		case ADDCA:

			// TODO: ALU instructions, ADDCA

			break;

		// TODO: execute more instructions here

		case JMP:
			this->execute_jmp();
			break;

		// if we encounter an unknown opcode
		default:
			throw std::exception("Unknown opcode; halting execution.");
			break;
	}

	return;
}


void SINVM::execute_load(int* reg_target) {
	/*
	
	Execute a LOAD_ instruction. This function takes a pointer to the register we want to load and executes the load instruction accordingly, storing the ultimate fetched result in the register we passed into the function.

	The function first reads the addressing mode, uses get_data_of_wordsize() to get the data after the addressing mode data (could be interpreted as an immediate value or memory address). It then checks the addressing mode to see how it needs to interpret the data it just fetched (data_to_load), and acts accordingly (e.g., if it's absolute, it gets the value at that memory address, then stores it in the register; if it is indexed, it does it almost the same, but adds the address value first, etc.).

	This function also makes use of 'validate_address()' to ensure all the addresses are within range.
	
	*/


	// the next data we have is the addressing mode
	this->PC++;
	uint8_t addressing_mode = *this->PC;

	this->PC++;

	// load the appropriate number of bytes according to our wordsize
	unsigned int data_to_load = this->get_data_of_wordsize();

	// check our addressing mode and decide how to interpret our data
	if ((0 <= addressing_mode) && (addressing_mode < 3)) {
		// if we have absolute or x/y indexed-addressing, we will be reading from memory
		int data_in_memory = 0;

		// however, we need to make sure that if it is indexed, we add the register values to data_to_load, which contains the memory address, so we actually perform the indexing
		if (addressing_mode == 1) {
			// x indexed -- add REG_X to data_to_load
			data_to_load += REG_X;
		}
		else if (addressing_mode == 2) {
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
		// our starting address is data_to_load
		for (int i = 0; i < (this->_WORDSIZE / 8); i++) {
			data_in_memory = data_in_memory + this->memory[data_to_load + i];	// get the value at the address of data_to_load indexed with i
			data_in_memory = data_in_memory << 8;
		}
		data_in_memory = data_in_memory >> 8;	// we went one too far in the loop, so move the data back to its proper place

		// load our target register with the data we fetched
		*reg_target = data_in_memory;

		// we are done
		return;
	}
	else if (addressing_mode == 3) {
		// if we are using immediate addressing, data_to_load is the data we want to load
		// simply assign it to our target register and return
		*reg_target = data_to_load;
		return;
	}
	/*
	Our indirect indexed modes are a little more complicated; they get the value at the address and use that as a memory location to access, finally indexing with the Y register.
	*/

	// TODO: implement indirect indexed addressing

	else if (addressing_mode == 5) {
		// indirect indexed addressing with the X register

	}
	else if (addressing_mode == 6) {
		// indirect indexed addressing with the Y register
	}
}


void SINVM::execute_store(int reg_to_store) {
	// our next byte is the addressing mode
	this->PC++;
	uint8_t addressing_mode = *this->PC;

	this->PC++;

	// next, get the memory location
	int memory_address = this->get_data_of_wordsize();

	// act according to the addressing mode
	if (addressing_mode == 0) {
		// make the assignment and return
		int to_assign = reg_to_store;
		for (int i = this->_WORDSIZE / 8; i > 0; i--) {
			this->memory[memory_address + i] = to_assign >> ((i - 1) * 8);
		}
		return;
	}
	else if (addressing_mode == 1) {
		// the memory address to overwrite
	}
	else if (addressing_mode == 3) {
		// we cannot use immediate addressing with a store instruction, so throw an exception
		throw std::exception("Invalid addressing mode for store instruction.");
	}
}


void SINVM::execute_jmp() {
	/*
	
	Execute a JMP instruction. The VM first increments the PC to get the addressing mode, then increments it again to get the memory address
	
	*/
	
	this->PC++;
	uint8_t addressing_mode = *this->PC;

	// get the memory address to which we want to jump
	this->PC++;
	int memory_address = this->get_data_of_wordsize();

	// check our addressing mode to see how we need to handle the data we just received
	if (addressing_mode == 0) {
		// if absolute, set it to memory address - 1 so when it gets incremented at the end of the instruction, it's at the proper address
		this->PC = &this->memory[memory_address - 1];
		return;
	}
	else if (addressing_mode == 1) {
		memory_address += REG_X;
		this->PC = &this->memory[memory_address - 1];
	}
	else if (addressing_mode == 2) {
		memory_address += REG_Y;
		this->PC = &this->memory[memory_address - 1];
	}

	// TODO: Indirect indexed ?

	else {
		throw std::exception("Invalid addressing mode for JMP instruction.");
	}

	return;
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
		this->execute_instruction(*this->PC);

		// advance the program counter to point to the next instruction
		this->PC++;
	}

	return;
}



void SINVM::_debug_values() {
	std::cout << "SINVM Values:" << std::endl;
	std::cout << "\t" << "Regsters:" << "\n\t\tA: $" << std::hex << this->REG_A << std::endl;
	std::cout << "\t\tX: $" << std::hex << this->REG_X << std::endl;
	std::cout << "\t\tY: $" << std::hex << this->REG_Y << std::endl;
	std::cout << "\t\tSTATUS: $" << std::hex << (int)this->STATUS << std::endl << std::endl;

	std::cout << "Memory: " << std::endl;
	for (int i = 0; i < 10; i++) {
		std::cout << "\t$000" << i << ": $" << std::hex << (int)this->memory[i] << std::endl;
	}

	std::cout << std::endl;
}


std::tuple<uint8_t, std::vector<uint8_t>> SINVM::load_sinc_file(std::istream& file) {

	// TODO: establish .sinc file format
	// TODO: write .sinc loading algorithm

	// this is just so the code compiles
	std::vector<uint8_t> machine_code{ 0, 0 };
	return std::make_tuple(16, machine_code);
}


SINVM::SINVM(std::istream& file, bool is_disassembled)
{
	// first, load the data from our .sinc file and copy it in appropriately
	std::tuple<uint8_t, std::vector<uint8_t>> sinc_data = this->load_sinc_file(file);
	this->_WORDSIZE = std::get<0>(sinc_data);
	std::vector<uint8_t> instructions = std::get<1>(sinc_data);

	// copy our instructions into memory so that the program is at the end of memory
	size_t program_size = instructions.size();
	size_t start_address = memory_size - program_size;	// the start addres for the program should be the total size - the program size

	std::vector<uint8_t>::iterator instruction_iter = instructions.begin();
	size_t memory_index = start_address;
	while ((instruction_iter != instructions.end()) && memory_index < memory_size) {
		this->memory[memory_index] = *instruction_iter;
		memory_index++;
		instruction_iter++;
	}

	// always initialize our status register so that all flags are not set
	this->STATUS = 0;

	// initialize the program counter to start at the top of the program
	if (instructions.size() != 0) {
		this->PC = &this->memory[start_address];	// make sure that we don't read memory that doesn't exist if our program is empty
	}
	else {
		// throw an exception; the VM cannot execute an empty program
		throw std::exception("Cannot execute an empty program; program size must be > 0");
	}
}

SINVM::SINVM(Assembler& assembler) {
	// first, load a vector with our instructions from the .sinc file
	this->_WORDSIZE = assembler._WORDSIZE;
	std::vector<uint8_t> instructions = assembler.assemble();

	// copy our instructions into memory so that the program is at the end of memory
	size_t program_size = instructions.size();
	size_t start_address = memory_size - program_size;	// the start addres for the program should be the total size - the program size

	std::vector<uint8_t>::iterator instruction_iter = instructions.begin();
	size_t memory_index = start_address;
	while ((instruction_iter != instructions.end()) && memory_index < memory_size) {
		this->memory[memory_index] = *instruction_iter;
		memory_index++;
		instruction_iter++;
	}

	// always initialize our status register so that all flags are not set
	this->STATUS = 0;

	// initialize the program counter to start at the top of the program
	if (instructions.size() != 0) {
		this->PC = &this->memory[start_address];	// make sure that we don't read memory that doesn't exist if our program is empty
	}
	else {
		// throw an exception; the VM cannot execute an empty program
		throw std::exception("Cannot execute an empty program; program size must be > 0");
	}
}

SINVM::~SINVM()
{
}
