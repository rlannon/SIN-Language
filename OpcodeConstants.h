#pragma once

#include <string>	// we have string constants
#include <cinttypes>	// we need uint8_t

/*

OPCODECONSTANTS.H

This file is simply used to contain all of the constants for our opcodes. It is much easier to house them here, in their own header file, rather than in Assembler.h, where they used to live.

This file also contains lists of the opcodes so that the opcode can be easily retrieved from the mnemonic, and vice versa.

TODO: remap opcodes

*/


// the number of instructions in our machine language
const size_t num_instructions = 63;


// general instructions
const uint8_t HALT = 0xFF;	// halt
const uint8_t NOOP = 0x00;	// no operation


// register A
const uint8_t LOADA = 0x01;	// load a with value
const uint8_t STOREA = 0x02;	// store a at address


// register B
const uint8_t LOADB = 0x03;
const uint8_t STOREB = 0x04;


// register X
const uint8_t LOADX = 0x05;	// load x with value
const uint8_t STOREX = 0x06;	// store x at address


// register Y
const uint8_t LOADY = 0x07;	// load y with value
const uint8_t STOREY = 0x08;	// store y at address


// STATUS register
const uint8_t CLC = 0x09;	// clear carry bit
const uint8_t SEC = 0x0A;	// set carry bit
const uint8_t CLN = 0x4A;	// clear the negative bit
const uint8_t SEN = 0x4B;	// set the negative bit
//const uint8_t CLF = 0x4C;	// clear the float bit
//const uint8_t SEF = 0x4D;	// set the float bit


// ALU-related instructions
// mathematical operations
const uint8_t ADDCA = 0x10;	// add register A (with carry) to some value, storing the result in A
const uint8_t SUBCA = 0x11;	// subtract some value (with carry) from register A ...
const uint8_t MULTA = 0x60;	// signed multiplication on register A with some value
const uint8_t DIVA = 0x61;	// signed divisionregister A by some value
const uint8_t MULTUA = 0x62;	// unsigned multiplication on register A with some value
const uint8_t DIVUA = 0x63;	// unsigned division on register A by some value

// logical operations
const uint8_t ANDA = 0x12;	// logical AND some value with A ...
const uint8_t ORA = 0x13;	// logical OR some value with A ...
const uint8_t XORA = 0x14;	// logical XOR with A ...

// bitshift operations
const uint8_t LSR = 0x15;	// logical shift right on some memory (or A)
const uint8_t LSL = 0x16;	// logical shift left on some memory (or A)
const uint8_t ROR = 0x17;	// rotate right on some memory (or A)
const uint8_t ROL = 0x18;	// rotate left on some memory (or A)


// increment/decrement registers
const uint8_t INCA = 0x19;	// increment A
const uint8_t DECA = 0x1A;	// decrement A
const uint8_t INCX = 0x1B;	// can only do this to A, X, Y (not B)
const uint8_t DECX = 0x1C;
const uint8_t INCY = 0x1D;
const uint8_t DECY = 0x1E;

const uint8_t INCB = 0x1F;	// we can increment B, but not decrement it

const uint8_t INCSP = 0x40;	// increment / decrement the stack pointer
const uint8_t DECSP = 0x41;


// Comparatives
const uint8_t CMPA = 0x20;	// compare registers by value
const uint8_t CMPB = 0x21;
const uint8_t CMPX = 0x22;
const uint8_t CMPY = 0x23;


// branch / control flow logic
const uint8_t JMP = 0x24;	// unconditional jump to supplied address
const uint8_t BRNE = 0x25;	// branch on not equal
const uint8_t BREQ = 0x26;	// branch on equal
const uint8_t BRGT = 0x27;	// branch on greater
const uint8_t BRLT = 0x28;	// branch on less
const uint8_t BRZ = 0x29;	// branch on zero
const uint8_t JSR = 0x3A;	// jump to subroutine
const uint8_t RTS = 0x3B;	// return from subroutine


// register transfers
// we can transfer register values to A and the value in A to any register, but not other combinations
const uint8_t TBA = 0x2A;	// transfer B to A
const uint8_t TXA = 0x2B;
const uint8_t TYA = 0x2C;
const uint8_t TSPA = 0x2D;	// transfer stack pointer to A
const uint8_t TSTATUSA = 0x51;	// transfer STATUS to A

const uint8_t TAB = 0x2E;	// transfer A to B
const uint8_t TAX = 0x2F;
const uint8_t TAY = 0x30;
const uint8_t TASP = 0x31;	// transfer A to stack pointer
const uint8_t TASTATUS = 0x50;	// transfer A to STATUS register


// the stack
const uint8_t PHA = 0x32;	// push A onto the stack
const uint8_t PLA = 0x33;	// pop a value off the stack and store in A
const uint8_t PHB = 0x34;	// push B onto the stack
const uint8_t PLB = 0x35;	// pop a value off the stack and store in B


// SYSCALL -- handles all interaction with the host machine
const uint8_t SYSCALL = 0x36;


// some constants for opcode comparisons (used for maintainability)
const std::string instructions_list[num_instructions] = { "HALT", "NOOP", "LOADA", "STOREA", "LOADB", "STOREB", "LOADX", "STOREX", "LOADY", "STOREY", "CLC", "SEC", "CLN", "SEN", "ADDCA", "SUBCA", "MULTA", "MULTUA", "DIVA", "DIVUA", "ANDA", "ORA", "XORA", "LSR", "LSL", "ROR", "ROL", "INCA", "DECA", "INCX", "DECX", "INCY", "DECY", "INCB", "INCSP", "DECSP", "CMPA", "CMPB", "CMPX", "CMPY", "JMP", "BRNE", "BREQ", "BRGT", "BRLT", "BRZ", "JSR", "RTS", "TBA", "TXA", "TYA", "TSPA", "TSTATUSA", "TAB", "TAX", "TAY", "TASP", "TASTATUS", "PHA", "PLA", "PHB", "PLB", "SYSCALL" };
const uint8_t opcodes[num_instructions] = { HALT, NOOP, LOADA, STOREA, LOADB, STOREB, LOADX, STOREX, LOADY, STOREY, CLC, SEC, CLN, SEN, ADDCA, SUBCA, MULTA, MULTUA, DIVA, DIVUA, ANDA, ORA, XORA, LSR, LSL, ROR, ROL, INCA, DECA, INCX, DECX, INCY, DECY, INCB, INCSP, DECSP, CMPA, CMPB, CMPX, CMPY, JMP, BRNE, BREQ, BRGT, BRLT, BRZ, JSR, RTS, TBA, TXA, TYA, TSPA, TSTATUSA, TAB, TAX, TAY, TASP, TASTATUS, PHA, PLA, PHB, PLB, SYSCALL };

// opcodes which do not need values to follow them (and, actually, for which proceeding values are forbidden)
const size_t num_standalone_opcodes = 30;
const uint8_t standalone_opcodes[num_standalone_opcodes] = { HALT, NOOP, CLC, SEC, CLN, SEN, INCA, DECA, INCX, DECX, INCY, DECY, INCB, INCSP, DECSP, RTS, TBA, TXA, TYA, TSPA, TSTATUSA, TAB, TAX, TAY, TASP, TASTATUS, PHA, PLA, PHB, PLB };
