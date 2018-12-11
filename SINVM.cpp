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
		// For these, we will use our load function to get the data we want in addition to the A register; we won't put it in the a register, but we will use the function because it will give it to us no problem
		case ADDCA:
			int addend;
			this->execute_load(&addend);

			// add the fetched data to A
			REG_A += addend;
			break;
		case SUBCA:
			// in subtraction, REG_A is the minuend and the value supplied is the subtrahend
			int subtrahend;
			this->execute_load(&subtrahend);

			if (subtrahend > REG_A) {
				this->set_status_flag('N');	// set the N flag if the result is negative, which will be the case if subtrahend > REG_A
			}
			else if (subtrahend == REG_A) {
				this->set_status_flag('Z');	// set the Z flag if the two are equal, as the result will be 0
			}

			REG_A -= subtrahend;
			break;
		case ANDA:
			int and_value;
			this->execute_load(&and_value);

			REG_A = REG_A & and_value;
			break;
		case ORA:
			int or_value;
			this->execute_load(&or_value);

			REG_A = REG_A | or_value;
			break;
		case XORA:
			int xor_value;
			this->execute_load(&xor_value);

			REG_A = REG_A ^ xor_value;
			break;

		// TODO: bit shifting

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
			this->push_stack();
			break;
		case PLA:
			this->pop_stack();
			break;

		// Standard input/output
		case INPUTB:
			// get the contents of the standard input and store in REG_B
			std::cin >> REG_B;
			// clear cin's bits so the next input will be ok
			std::cin.clear();
			std::cin.ignore();
			break;
		case OUTPUTB:
			std::cout << REG_B << std::endl;
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
	if (((0 <= addressing_mode) && (addressing_mode < 3)) || (addressing_mode == 5) || (addressing_mode == 6)) {	// if the addressing mode is gt/eq 0 and lt 3, or =5, or =6
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
	Our indirect indexed modes are a little more complicated; they get the value at the address and use that as a memory location to access, finally indexing with the Y register. Essentially, this acts as a pointer.
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
	unsigned int memory_address = this->get_data_of_wordsize();

	// validate the memory location
	// TODO: decide whether writing to an invalid location will wrap around or simply throw an exception
	if (!address_is_valid(memory_address)) {
		// as long as it is out of range, continue subtracting the max memory size until it is valid
		while (memory_address > this->memory_size) {
			memory_address -= memory_size;
		}
	}

	// act according to the addressing mode
	if (((0 <= addressing_mode) && (addressing_mode < 3)) || (addressing_mode == 5) || (addressing_mode == 6)) {	// if addressing mode is gt/eq 0 and lt 3, or =5, or =6
		// add the appropriate register if it is an indexed addressing mode
		if (addressing_mode == 1) {
			memory_address += this->REG_X;
		}
		else if (addressing_mode == 2) {
			memory_address += this->REG_Y;
		}

		// make the assignment and return
		for (int i = this->_WORDSIZE / 8; i > 0; i--) {
			this->memory[memory_address + i] = reg_to_store >> ((i - 1) * 8);
		}
		return;
	}
	else if (addressing_mode == 3) {
		// we cannot use immediate addressing with a store instruction, so throw an exception
		throw std::exception("Invalid addressing mode for store instruction.");
	}
}


void SINVM::execute_comparison(int reg_to_compare) {
	int to_compare;
	this->execute_load(&to_compare);

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
	uint8_t addressing_mode = *this->PC;

	// get the memory address to which we want to jump
	this->PC++;
	int memory_address = this->get_data_of_wordsize();
	// make sure it's a jump within the program, so add the offset of the program start
	memory_address += (int)program_start_address;

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


// Stack functions -- these always use register A, so no point in having parameters
void SINVM::push_stack() {
	// push the current value in A onto the stack, decrementing the SP (because the stack grows downwards)

	// first, make sure the stack hasn't hit its bottom
	if (this->SP < 0x10) {
		throw std::exception("**** ERROR: Stack overflow.");
	}

	// TODO: we must do this as many times as is necessary to fill a word on the VM (2 times for 16 bit, 4 for 32)
	for (int i = (this->_WORDSIZE / 8); i > 0; i--) {
		this->memory[SP] = REG_A >> ((i - 1) * 8);
		this->SP--;
	}
	std::cout << std::endl;
	return;
}

void SINVM::pop_stack() {
	// pop the most recently pushed value off the stack; this means we must increment the SP (as the current value pointed to by SP is the one to which we will write next; also, the stack grows downwards so we want to increase the address if we pop something off), and then dereference; this means this area of memory is the next to be written to if we push something onto the stack

	// first, make sure we aren't going beyond the bounds of the stack
	if (this->SP > 0x1f) {
		throw std::exception("**** ERROR: Stack underflow.");
	}

	// for each byte in a word, dereference the stack pointer and add it to our stack_data variable, shifting over 8 bits afterward
	int stack_data = 0;
	for (int i = 1; i < (this->_WORDSIZE / 8); i++) {
		stack_data += this->memory[SP];
		stack_data = stack_data << 8;
		this->SP++;
	}
	// because we have added 1 too few bytes, we must add the dereferenced pointer to stack_data and increment the stack pointer again
	stack_data += this->memory[SP];
	this->SP++;

	// finally, set A equal to stack_data
	this->REG_A = stack_data;

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
	std::cout << "\t" << "Registers:" << "\n\t\tA: $" << std::hex << this->REG_A << std::endl;
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
	// a vector to hold our program data
	std::vector<uint8_t> program_data;

	// first, read the magic number
	char header[4];
	char * buffer = &header[0];

	file.read(buffer, 4);

	// if our magic number is valid
	if (header[0, 1, 2, 3] == * "s", "i", "n", "C") {
		// must have the correct version
		uint8_t version = readU8(file);

		if (version == 1) {
			// get the word size
			uint8_t word_size = readU8(file);
			// get the program size
			uint16_t program_size = readU16(file);

			// use program_size to read the proper number of bytes
			int i = 0;
			while (!file.eof() && i < program_size) {
				program_data.push_back(readU8(file));
				i++;
			}

			// return word size and the program data vector
			return std::make_tuple(word_size, program_data);
		}
		// cannot handle any other versions right now because they don't exist yet
		else {
			throw std::exception("Other .sinc file versions not supported at this time.");
		}
	}
	else {
		throw std::exception("Invalid magic number in file header.");
	}
}


SINVM::SINVM(std::istream& file)
{
	// first, load the data from our .sinc file and copy it in appropriately
	std::tuple<uint8_t, std::vector<uint8_t>> sinc_data = this->load_sinc_file(file);

	// set the data as is appropriate for the
	this->_WORDSIZE = std::get<0>(sinc_data);
	std::vector<uint8_t> instructions = std::get<1>(sinc_data);

	// copy our instructions into memory so that the program is at the end of memory
	size_t program_size = instructions.size();
	this->program_start_address = memory_size - program_size;	// the start addres for the program should be the total size - the program size

	std::vector<uint8_t>::iterator instruction_iter = instructions.begin();
	size_t memory_index = this->program_start_address;
	while ((instruction_iter != instructions.end()) && memory_index < memory_size) {
		this->memory[memory_index] = *instruction_iter;
		memory_index++;
		instruction_iter++;
	}

	// initialize the stack to location $1f; it grows downwards
	this->SP = 0x1f;

	// always initialize our status register so that all flags are not set
	this->STATUS = 0;

	// initialize the program counter to start at the top of the program
	if (instructions.size() != 0) {
		this->PC = &this->memory[this->program_start_address];	// make sure that we don't read memory that doesn't exist if our program is empty
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
	this->program_start_address = memory_size - program_size;	// the start addres for the program should be the total size - the program size

	std::vector<uint8_t>::iterator instruction_iter = instructions.begin();
	size_t memory_index = this->program_start_address;
	while ((instruction_iter != instructions.end()) && memory_index < memory_size) {
		this->memory[memory_index] = *instruction_iter;
		memory_index++;
		instruction_iter++;
	}

	// initialize the stack to location $1f; it grows downwards
	this->SP = 0x1f;

	// always initialize our status register so that all flags are not set
	this->STATUS = 0;

	// initialize the program counter to start at the top of the program
	if (instructions.size() != 0) {
		this->PC = &this->memory[this->program_start_address];	// make sure that we don't read memory that doesn't exist if our program is empty
	}
	else {
		// throw an exception; the VM cannot execute an empty program
		throw std::exception("Cannot execute an empty program; program size must be > 0");
	}
}

SINVM::~SINVM()
{
}
