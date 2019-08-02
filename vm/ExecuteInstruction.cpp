/*

SIN Toolchain
ExecuteInstruction.cpp
Copyright 2019 Riley Lannon

Contains the implementation of the execute_instruction function for the SIN VM; because it is such a large function, it is easier to have it in a separate file

*/

#include "SINVM.h"

void SINVM::execute_instruction(uint16_t opcode) {
	/*

	Execute a single instruction. Each instruction will increment or set the PC according to what it needs to do for that particular instruction.
	This function delegates the task of handling instruction execution to many other functions to make the code more maintainable and easier to understand.

	*/

	// use a switch statement; compiler may be able to optimize it more easily and it will be easier to read
	switch (opcode) {
		/*
		
		GENERAL PROCESSOR INSTRUCTIONS
		
		*/

		case NOOP:
			break;

		/*
		
		REGISTER INSTRUCTIONS
		
		*/

		// A register
		case LOADA:
			REG_A = this->execute_load();
			break;
		case STOREA:
			this->execute_store(REG_A);
			break;
		case TAB:
			this->REG_B = this->REG_A;
			break;
		case TAX:
			this->REG_X = this->REG_A;
			break;
		case TAY:
			this->REG_Y = this->REG_A;
			break;
		case TASP:
			this->SP = this->REG_A;
			break;
		case TASTATUS:
			this->STATUS = this->REG_A;
			break;
		case INCA:
			this->REG_A += 1;
			break;
		case DECA:
			this->REG_A -= 1;
			break;

		// B register
		case LOADB:
			REG_B = this->execute_load();
			break;
		case STOREB:
			this->execute_store(REG_B);
			break;
		case TBA:
			this->REG_A = this->REG_B;
			break;
		case TBX:
			this->REG_X = this->REG_B;
			break;
		case TBY:
			this->REG_Y = this->REG_B;
			break;
		case TBSP:
			this->SP = this->REG_B;
			break;
		case TBSTATUS:
			this->STATUS = this->REG_B;
			break;
		case INCB:
			this->REG_B += 1;
			break;
		case DECB:
			this->REG_B -= 1;
			break;

		// X register
		case LOADX:
			REG_X = this->execute_load();
			break;
		case STOREX:
			this->execute_store(REG_X);
			break;
		case TXA:
			this->REG_A = this->REG_X;
			break;
		case TXB:
			this->REG_B = this->REG_X;
			break;
		case TXY:
			this->REG_Y = this->REG_X;
			break;
		case TXSP:
			this->SP = this->REG_X;
			break;
		case INCX:
			this->REG_X += 1;
			break;
		case DECX:
			this->REG_X -= 1;
			break;

		// Y register
		case LOADY:
			REG_Y = this->execute_load();
			break;
		case STOREY:
			this->execute_store(REG_Y);
			break;
		case TYA:
			this->REG_A = this->REG_Y;
			break;
		case TYB:
			this->REG_B = this->REG_Y;
			break;
		case TYX:
			this->REG_X = this->REG_Y;
			break;
		case TYSP:
			this->SP = this->REG_Y;
			break;
		case INCY:
			this->REG_Y += 1;
			break;
		case DECY:
			this->REG_Y -= 1;
			break;
		
		/*
		
		ALU INSTRUCTIONS

		*/
		case LSR: case LSL: case ROR: case ROL:
		{
			this->execute_bitshift(opcode);		// todo: move bitshift instructions to ALU?
			break;
		}
		case ADDCA:
		{
			// get the addend
			uint16_t addend = this->execute_load();

			// call the alu.add(...) function using the value we just fetched
			this->alu.add(addend);
			break;
		}
		case ADDCB:
		{
			// todo: implement the ADDCB instruction
			break;
		}
		case SUBCA:
		{
			// in subtraction, REG_A is the minuend and the value supplied is the subtrahend
			uint16_t subtrahend = this->execute_load();

			// call the alu.sub function using the value we just fetched
			this->alu.sub(subtrahend);
			break;
		}
		case SUBCB:
		{
			// todo: implement the SUBCB instruction
			break;
		}
		case MULTA:
		{
			// Multiply A by some value; treat both integers as signed
			uint16_t multiplier = this->execute_load();

			// call alu.mult_signed using the multiplier value we just fetched
			this->alu.mult_signed(multiplier);
			break;
		}
		case DIVA:
		{
			// Signed division on A by some value; this uses _integer division_ where B will hold the remainder of the operation
			// fetch the right operand and call the ALU div_signed function using said operand as an argument
			uint16_t divisor = this->execute_load();

			// if the divisor is 0, send a SINSIGFPE to the processor
			if (divisor == 0) {
				this->PC -= 3;	// the PC will be on the last byte of the data, so it needs to be backed up 3 bytes to point to the opcode
				this->send_signal(SINSIGFPE);
			}
			else {
				this->alu.div_signed(divisor);
			}
			break;
		}
		case MULTUA:
		{
			// Unsigned multiplication
			// fetch the value and call ALU::mult_unsigned(...) using the value we fetched as our argument
			uint16_t multiplier = this->execute_load();
			this->alu.mult_unsigned(multiplier);
			break;
		}
		case DIVUA:
		{
			// Unsigned division; B will hold the remainder from the operation

			// fetch the right operand
			uint16_t divisor = this->execute_load();

			// if the operand is 0, send a SINSIGFPE to the processor
			if (divisor == 0) {
				this->set_status_flag('U');
				this->PC -= 3;	// like above, back up the PC 3 bytes to point to the opcode
				this->send_signal(SINSIGFPE);
			}
			else {
				// call the ALU's div_unsigned function using the value we just fetched as the parameter
				this->alu.div_unsigned(divisor);
			}
			break;
		}

		// Logical operations
		// todo: move logical operation instructions to ALU
		case ANDA:
		{
			uint16_t and_value = this->execute_load();

			REG_A = REG_A & and_value;
			break;
		}
		case ORA:
		{
			uint16_t or_value = this->execute_load();

			REG_A = REG_A | or_value;
			break;
		}
		case XORA:
		{
			uint16_t xor_value = this->execute_load();

			REG_A = REG_A ^ xor_value;
			break;
		}

		// Comparatives
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

		/*
		
		FPU INSTRUCTIONS
		todo: implement FPU instructions
		
		*/

		// 16-bit
		case FADDA:
		{
			uint16_t addend = this->execute_load();
			this->fpu.fadda(addend);
			break;
		}
		case FSUBA:
		{
			uint16_t subtrahend = this->execute_load();
			this->fpu.fsuba(subtrahend);
			break;
		}
		case FMULTA:
		{
			uint16_t multiplier = this->execute_load();
			this->fpu.fmulta(multiplier);
			break;
		}
		case FDIVA:
		{
			uint16_t divisor = this->execute_load();
			if (divisor == 0) {
				this->PC -= 3;
				this->send_signal(SINSIGFPE);
			}
			else {
				this->fpu.fdiva(divisor);
			}

			// make sure we didn't get an undefined result; if we did, then generate a floating point error
			if (this->is_flag_set('U')) {
				this->send_signal(SINSIGFPE);
			}

			break;
		}
		
		// 32-bit
		// todo: devise a method to load a 32-bit value
		case SFADDA:
			break;
		case SFSUBA:
			break;
		case SFMULTA:
			break;
		case SFDIVA:
			break;

		/*
		
		STACK INSTRUCTIONS
		
		*/

		case PHA:
			this->push_stack(REG_A);
			break;
		case PHB:
			this->push_stack(REG_B);
			break;
		case PLA:
			REG_A = this->pop_stack();
			break;
		case PLB:
			REG_B = this->pop_stack();
			break;
		case PRSA:
			this->push_call_stack(this->REG_A);
			break;
		case PRSB:
			this->push_call_stack(this->REG_B);
			break;
		case RSTA:
			this->REG_A = this->pop_call_stack();
			break;
		case RSTB:
			this->REG_B = this->pop_call_stack();
			break;
		case PRSR:
		{
			/*
			
			Preserve registers, pushed in the following order:
				- A
				- B
				- X
				- Y
				- SP
				- STATUS
			One word is used for each register, meaning we need 6 words total, or 12 bytes

			*/

			
			// create an array of uint8_t holding all of our data
			uint16_t to_push[6] = { this->REG_A, this->REG_B, this->REG_X,  this->REG_Y, this->SP, this->STATUS };

			// push the elements of the array to our stack
			for (size_t i = 0; i < 6; i++) {
				this->push_call_stack(to_push[i]);
			}

			break;
		}
		case RSTR:
		{
			/*

			Pull registers in the reverse order as we pushed them

			*/

			uint16_t* popped[6] = { &this->STATUS, &this->SP, &this->REG_Y, &this->REG_X, &this->REG_B, &this->REG_A };

			for (size_t i = 0; i < 6; i++) {
				*(popped[i]) = this->pop_call_stack();
			}

			break;
		}
		case TSPA:
			this->REG_A = this->SP;
			break;
		case TSPB:
			this->REG_B = this->SP;
			break;
		case TSPX:
			this->REG_X = this->SP;
			break;
		case TSPY:
			this->REG_Y = this->SP;
			break;
		case INCSP:
			// Make sure that incrementing the SP will not cause a stack fault
			if (this->SP <= (_STACK - (this->_WORDSIZE / 8))) {
				this->SP += (this->_WORDSIZE / 8);	// incrementing the stack pointer increments by a _word_, not a _byte_
			}
			else {
				this->send_signal(SINSIGSTKFLT);	// otherwise, send a stack fault signal
			}
			break;
		case DECSP:
			// Same procedure as INCSP, basically
			if (this->SP >= (_STACK_BOTTOM + (this->_WORDSIZE / 8))) {
				this->SP -= (this->_WORDSIZE / 8);
			}
			else {
				this->send_signal(SINSIGSTKFLT);
			}
			break;

		/*
		
		STATUS Register Intructions
		
		*/
		
		case CLC:
			this->clear_status_flag('C');
			break;
		case SEC:
			this->set_status_flag('C');
			break;
		case CLN:
			this->clear_status_flag('N');
			break;
		case SEN:
			this->set_status_flag('N');
			break;
		case CLF:
			this->clear_status_flag('F');
			break;
		case SEF:
			this->set_status_flag('F');
			break;
		case TSTATUSA:
			this->REG_A = this->STATUS;
			break;
		case TSTATUSB:
			this->REG_B = this->STATUS;
			break;

		/*
		
		Control Flow Instructions
		
		*/
		
		case JMP:
			this->execute_jmp();
			break;
		case BRNE: case BRZ:	// BRNE and BRZ both test for the Z flag; no sense in repeating code
			// if the comparison was unequal, the Z flag will be clear; if it's set, we do not branch
			if (this->is_flag_set('Z')) {
				// skip past the addressing mode and address if it isn't set
				// we need to skip past 3 bytes
				this->PC += 3;
			}
			else {
				// if it's clear, branch
				this->execute_jmp();
			}
			break;
		case BREQ:
			// if the comparison was equal, the Z flag will be set
			if (this->is_flag_set('Z')) {
				// if it's set, execute a jump
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
		case BRN:
			// branch on negative; if the N flag is set, branch
			if (this->is_flag_set('N')) {
				this->execute_jmp();
			}
			else {
				this->PC += 3;
			}
		case BRPL:
			// branch on plus; if the N flag is clear, branch
			if (this->is_flag_set('N')) {
				this->PC += 3;
			}
			else {
				this->execute_jmp();
			}
		case IRQ:
		{
			// todo: implement IRQ instruction
			break;
		}
		case RTI:
		{
			// todo: implement RTI instruction
			break;
		}
		case JSR:
		{
			// get the addressing mode
			this->PC++;
			uint8_t addressing_mode = this->memory[this->PC];

			// get the value
			this->PC++;
			uint16_t address_to_jump = this->get_data_of_wordsize();
			uint16_t return_address = this->PC;	// the current address is the last of the instruction, which is where we want to return

			if (this->CALL_SP > _CALL_STACK_BOTTOM) {
				this->push_call_stack(return_address);
				this->PC = address_to_jump - 1;	// jump to one byte before the next instruction, as the PC is incremented at the end of each cycle
			}
			else {
				this->PC -= 3;
				this->send_signal(SINSIGSTKFLT);
			}
			break;
		}
		case RTS:
		{
			uint16_t return_address = this->pop_call_stack();
			this->PC = return_address;	// we don't need to offset because the absolute address was pushed to the call stack
			break;
		}

		// System instructions
		case BRK:
			/*
			Temporary debugging instruction; will be deleted once the actual debugger is implemented
			*/
			std::cout << "A: $" << std::hex << this->REG_A << std::endl;
			std::cout << "B: $" << this->REG_B << std::endl;
			std::cout << "X: $" << this->REG_X << std::endl;
			std::cout << "Y: $" << this->REG_Y << std::endl;
			std::cout << "SP: $" << this->SP << std::endl;
			std::cout << "CALL: $" << this->CALL_SP << std::endl;
			std::cout << "STATUS: $" << this->STATUS << std::endl;
			std::cin.clear();
			std::cin.get();
			break;
		case SYSCALL:
			this->execute_syscall();	// call the execute_syscall function; this will handle everything for us
			break;
		case RESET:
			// if we get a RESET instruction, generate a SINSIGRESET signal
			this->send_signal(SINSIGRESET);
			break;
		case HALT:
			// if we get a HALT instruction, we want to set the H flag, which will stop the VM in its main loop
			this->set_status_flag('H');
			break;

		// if we encounter an unknown opcode, generate a SINSIGILL signal
		default:
		{
			this->send_signal(SINSIGILL);
			break;
		}
	}

	return;
}
