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
	case RESET:
		// if we get a RESET instruction, generate a SINSIGRESET signal
		this->send_signal(SINSIGRESET);
		break;
	case HALT:
		// if we get a HALT instruction, we want to set the H flag, which will stop the VM in its main loop
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
	case CLN:
		this->clear_status_flag('N');
		break;
	case SEN:
		this->set_status_flag('N');
		break;

		// ALU instructions
		// For these, we will use our load function to get the right operand, which will be the function parameter for the ALU functions
	case ADDCA:
	{
		// get the addend
		uint16_t addend = this->execute_load();

		// call the alu.add(...) function using the value we just fetched
		this->alu.add(addend);
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
			this->PC -= 3;	// like above, back up the PC 3 bytes to point to the opcode
			this->send_signal(SINSIGFPE);
		}
		else {
			// call the ALU's div_unsigned function using the value we just fetched as the parameter
			this->alu.div_unsigned(divisor);
		}
		break;
	}
	// todo: update logical operations to use the ALU
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
	case INCB:
		this->REG_B += 1;
		break;
		// Note that INCSP and DECSP modify by one /word/, not one byte (it is unlike the other inc/dec instructions in this way)
	case INCSP:
		// increment by one word
		if (this->SP < (uint16_t)_STACK) {
			this->SP += (this->_WORDSIZE / 8);
		}
		else {
			// the PC doesn't need to be adjusted, as INCSP is a single-byte instruction
			this->send_signal(SINSIGSTKFLT);
		}
		break;
	case DECSP:
		// decrement by one word
		if (this->SP > (uint16_t)_STACK_BOTTOM) {
			this->SP -= (this->_WORDSIZE / 8);
		}
		else {
			// DECSP is also a single byte, so the PC doesn't need to be adjusted
			this->send_signal(SINSIGSTKFLT);
		}
		break;

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

		// Branch and control flow logic
	case JMP:
		this->execute_jmp();
		break;
	case BRNE:
		// if the comparison was unequal, the Z flag will be clear
		if (!this->is_flag_set('Z')) {
			// if it's clear, execute a jump
			this->execute_jmp();
		}
		else {
			// skip past the addressing mode and address if it isn't set
			// we need to skip past 3 bytes
			this->PC += 3;
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
		uint16_t address_to_jump = this->get_data_of_wordsize();

		// the return address should be one byte before the next instruction, as the PC is incremented at the _end_ of each cycle
		uint16_t return_address = this->PC;

		// if the CALL_SP is greater than _CALL_STACK_BOTTOM, we have not written past the end of it
		if (this->CALL_SP >= _CALL_STACK_BOTTOM) {
			// memory size is 16 bits but...do this anyways
			for (size_t i = 0; i < (this->_WORDSIZE / 8); i++) {
				uint8_t memory_byte = return_address >> (i * 8);
				this->memory[CALL_SP] = memory_byte;
				this->CALL_SP--;
			}
		}
		else {
			// first, back up the PC to the instruction's start address
			this->PC -= 3;

			// send the processor signal
			this->send_signal(SINSIGSTKFLT);
		}

		this->PC = address_to_jump - 1;

		break;
	}
	case RTS:
	{
		uint16_t return_address = 0;
		if (this->CALL_SP <= _CALL_STACK) {
			for (size_t i = (this->_WORDSIZE / 8); i > 0; i--) {
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
		REG_A = (uint16_t)SP;	// SP holds the address to which the next element in the stack will go, and is incremented every time something is pushed, and decremented every time something is popped
		break;
	case TSTATUSA:
		REG_A = (uint16_t)STATUS;
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
		SP = (uint16_t)REG_A;
		break;
	case TASTATUS:
		STATUS = (uint8_t)REG_A;
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
		this->execute_syscall();	// call the execute_syscall function; this will handle everything for us
		break;
	}

	// if we encounter an unknown opcode, generate a SINSIGILL signal
	default:
	{
		this->send_signal(SINSIGILL);
		break;
	}
	}

	return;
}
