#pragma once

#include <string>	// we have string constants

/*

OPCODECONSTANTS.H

This file is simply used to contain all of the constants for our opcodes. It is much easier to house them here, in their own header file, rather than in Assembler.h, where they used to live.

*/

// the number of instructions in our machine language
const size_t num_instructions = 50;

// general instructions
const int HALT = 0xFF;	// halt
const int NOOP = 0x00;	// no operation
						
// register A
const int LOADA = 0x01;	// load a with value
const int STOREA = 0x02;	// store a at address

// register B
const int LOADB = 0x03;
const int STOREB = 0x04;

// register X
const int LOADX = 0x05;	// load x with value
const int STOREX = 0x06;	// store x at address

// register Y
const int LOADY = 0x07;	// load y with value
const int STOREY = 0x08;	// store y at address

// STATUS register
const int CLC = 0x09;	// clear carry bit
const int SEC = 0x0A;	// set carry bit

// ALU-related instructions
const int ADDCA = 0x10;	// add register A (with carry) to some value, storing the result in A
const int SUBCA = 0x11;	// subtract some value (with carry) from register A ...
const int ANDA = 0x12;	// logical AND some value with A ...
const int ORA = 0x13;	// logical OR some value with A ...
const int XORA = 0x14;	// logical XOR with A ...
const int LSR = 0x15;	// logical shift right on some memory (or A)
const int LSL = 0x16;	// logical shift left on some memory (or A)
const int ROR = 0x17;	// rotate right on some memory (or A)
const int ROL = 0x18;	// rotate left on some memory (or A)
						// increment/decrement registers
const int INCA = 0x19;	// increment A
const int DECA = 0x1A;	// decrement A
const int INCX = 0x1B;	// can only do this to A, X, Y (not B)
const int DECX = 0x1C;
const int INCY = 0x1D;
const int DECY = 0x1E;

// Comparatives
const int CMPA = 0x20;	// compare registers by value
const int CMPB = 0x21;
const int CMPX = 0x22;
const int CMPY = 0x23;

// branch / control flow logic
const int JMP = 0x24;	// unconditional jump to supplied address
const int BRNE = 0x25;	// branch on not equal
const int BREQ = 0x26;	// branch on equal
const int BRGT = 0x27;	// branch on greater
const int BRLT = 0x28;	// branch on less
const int BRZ = 0x29;	// branch on zero
const int JSR = 0x3A;	// jump to subroutine
const int RTS = 0x3B;	// return from subroutine

// register transfers
// we can transfer register values to A and the value in A to any register, but not other combinations
const int TBA = 0x2A;	// transfer B to A
const int TXA = 0x2B;
const int TYA = 0x2C;
const int TSPA = 0x2D;	// transfer stack pointer to A
const int TAB = 0x2E;	// transfer A to B
const int TAX = 0x2F;
const int TAY = 0x30;
const int TASP = 0x31;	// transfer A to stack pointer

// the stack
const int PHA = 0x32;	// push A onto the stack
const int PLA = 0x33;	// pop a value off the stack and store in A

// SYSCALL -- handles all interaction with the host machine
const int SYSCALL = 0x36;

// TODO: implement db instruction

// DB instruction -- so that we can set a memory array programatically (particularly useful for strings)
const int DB = 0x40;

// some constants for opcode comparisons (used for maintainability)
const std::string instructions_list[num_instructions] = { "HALT", "NOOP", "LOADA", "STOREA", "LOADB", "STOREB", "LOADX", "STOREX", "LOADY", "STOREY", "CLC", "SEC", "ADDCA", "SUBCA", "ANDA", "ORA", "XORA", "LSR", "LSL", "ROR", "ROL", "INCA", "DECA", "INCX", "DECX", "INCY", "DECY", "CMPA", "CMPB", "CMPX", "CMPY", "JMP", "BRNE", "BREQ", "BRGT", "BRLT", "BRZ", "TBA", "TXA", "TYA", "TSPA", "TAB", "TAX", "TAY", "TASP", "PHA", "PLA", "SYSCALL", "DB" };
const int opcodes[num_instructions] = { HALT, NOOP, LOADA, STOREA, LOADB, STOREB, LOADX, STOREX, LOADY, STOREY, CLC, SEC, ADDCA, SUBCA, ANDA, ORA, XORA, LSR, LSL, ROR, ROL, INCA, DECA, INCX, DECX, INCY, DECY, CMPA, CMPB, CMPX, CMPY, JMP, BRNE, BREQ, BRGT, BRLT, BRZ, TBA, TXA, TYA, TSPA, TAB, TAX, TAY, TASP, PHA, PLA, SYSCALL, DB };