/*

SIN Toolchain
OpcodeConstants.h
Copyright 2019 Riley Lannon

This file is simply used to contain all of the constants for our opcodes. It is much easier to house them here, in their own header file, rather than in Assembler.h, where they used to live.
This file also contains lists of the opcodes so that the opcode can be easily retrieved from the mnemonic, and vice versa.

Opcodes are organized as follows:
  - 0x00 to 0x0F  - General asm instructions
  - 0x10 to 0x4F  - Register instructions:
    - 0x1x  - A Register,
    - 0x2x  - B Register,
    - 0x3x  - X Register,
    - 0x4x  - Y Register; all opcodes correspond
  - 0x50 to 0x6F  - ALU instructions
  - 0x70 to 0x8F  - FPU instructions (mirrors ALU instructions)
  - 0x90 to 0x9F  - Stack instructions
  - 0xA0 to 0xAF  - STATUS instructions
  - 0xB0 to 0xBF  - Control Flow instructions
  - 0xC0 to 0xCF  - Reserved for extra ALU instructions
  - 0xD0 to 0xDF  - Reserved for extra FPU instructions
  - 0xE0 to 0xEF  - Reserved for extra miscellaneous instructions
  - 0xF0 to 0xFF  - Machine instructions

For register instructions, the scheme for opcode order is as follows:
  - x0 - load
  - x1 - store
  - x2 - transfer to A (avail B, X, Y)
  - x3 - transfer to B (avail A, X, Y)
  - x4 - transfer to X (avail A, B, Y)
  - x5 - transfer to Y (avail A, B, X)
  - x6 - transfer to SP (avail A, B, X, Y)
  - x7 - transfer to STATUS (avail A, B)
  - x8 - increment
  - x9 - decrement
Note that not all instructions are available on all registers; the places where those unavailable instructions shall remain blank until that space is needed for an instruction. Operands in places 0, 1, 8, and 9 are available on all registers, but 2-7 vary.

Instructions may be added as needed into the available pages

*/

#pragma once

#include <string>	// we have string constants
#include <cinttypes>	// we need uint8_t

// the number of instructions in our machine language
const size_t num_instructions = 87;

// General instructions
const uint8_t NOOP = 0x00;

// Register A
const uint8_t LOADA = 0x10;
const uint8_t STOREA = 0x11;
// 0x12 unused
const uint8_t TAB = 0x13;
const uint8_t TAX = 0x14;
const uint8_t TAY = 0x15;
const uint8_t TASP = 0x16;
const uint8_t TASTATUS = 0x17;
const uint8_t INCA = 0x18;
const uint8_t DECA = 0x19;

// Register B
const uint8_t LOADB = 0x20;
const uint8_t STOREB = 0x21;
const uint8_t TBA = 0x22;
// 0x23 unused
const uint8_t TBX = 0x24;
const uint8_t TBY = 0x25;
const uint8_t TBSP = 0x26;
const uint8_t TBSTATUS = 0x27;
const uint8_t INCB = 0x28;
const uint8_t DECB = 0x29;

// Register X
const uint8_t LOADX = 0x30;
const uint8_t STOREX = 0x31;
const uint8_t TXA = 0x32;
const uint8_t TXB = 0x33;
// 0x34 unused
const uint8_t TXY = 0x35;
const uint8_t TXSP = 0x36;
// 0x37 unused
const uint8_t INCX = 0x38;
const uint8_t DECX = 0x39;

// Register Y
const uint8_t LOADY = 0x40;
const uint8_t STOREY = 0x41;
const uint8_t TYA = 0x42;
const uint8_t TYB = 0x43;
const uint8_t TYX = 0x44;
// 0x45 unused
const uint8_t TYSP = 0x46;
// 0x47 unused
const uint8_t INCY = 0x48;
const uint8_t DECY = 0x49;

// ALU instructions
const uint8_t ROL = 0x50;
const uint8_t ROR = 0x51;
const uint8_t LSL = 0x52;
const uint8_t LSR = 0x53;
// 0x54 to 0x57 unused
const uint8_t INCM = 0x58;
const uint8_t DECM = 0x59;
const uint8_t ADDCA = 0x5A;
const uint8_t ADDCB = 0x5B;
const uint8_t MULTA = 0x5C;
const uint8_t MULTUA = 0x5D;
const uint8_t DIVA = 0x5E;
const uint8_t DIVUA = 0x5F;
const uint8_t ANDA = 0x60;
const uint8_t ORA = 0x61;
const uint8_t XORA = 0x62;
// 0x63 - 0x69 currently unused
const uint8_t CMPA = 0x6A;
const uint8_t CMPB = 0x6B;
const uint8_t CMPX = 0x6C;
const uint8_t CMPY = 0x6D;
// 0x6E, 0x6F currently unused

// todo: implement FPU 
/*

The FPU instructions are generally equivalent to their ALU counterparts with an F- prefix, though some differences exist.
The instructions should be laid out as follows:

  - 0x78  - FINC (accepts A and B as operands, as no FINCA or FINCB instructions exist)
  - 0x79  - FDEC (same as with FINCM)
  - 0x7A  - FADDA
  - 0x7B  - FADDB
  - 0x7C  - FMULTA
  - 0x7D  - FDIVA

Note that there is only one floating point multiplication and one floating point division number; floating point numbers in SIN are always signed. As such, there is no need for unsigned counterparts to these instructions.

*/

// Stack instructions
const uint8_t PHA = 0x90;
const uint8_t PHB = 0x91;
const uint8_t PLA = 0x92;
const uint8_t PLB = 0x93;
// 0x94 - 0x99 currently unused
const uint8_t TSPA = 0x9A;
const uint8_t TSPB = 0x9B;
const uint8_t TSPX = 0x9C;
const uint8_t TSPY = 0x9D;
const uint8_t INCSP = 0x9E;
const uint8_t DECSP = 0x9F;

// STATUS instructions
const uint8_t CLC = 0xA0;	// clear carry bit
const uint8_t SEC = 0xA1;	// set carry bit
const uint8_t CLN = 0xA2;	// clear the negative bit
const uint8_t SEN = 0xA3;	// set the negative bit
const uint8_t CLF = 0xA4;	// clear the float bit
const uint8_t SEF = 0xA5;	// set the float bit
// 0xA6 to 0xA9 currently unused
const uint8_t TSTATUSA = 0xAA;
const uint8_t TSTATUSB = 0xAB;

// Control flow instructions
const uint8_t JMP = 0xB0;
const uint8_t BRNE = 0xB1;
const uint8_t BREQ = 0xB2;
const uint8_t BRGT = 0xB3;
const uint8_t BRLT = 0xB4;
const uint8_t BRZ = 0xB5;
const uint8_t BRN = 0xB6;
const uint8_t BRPL = 0xB7;
// 0xB8 to 0xBB unused
const uint8_t IRQ = 0xBC;
const uint8_t RTI = 0xBD;
const uint8_t JSR = 0xBE;
const uint8_t RTS = 0xBF;

// Machine instructions
const uint8_t SYSCALL = 0xFA;
const uint8_t RESET = 0xFE;
const uint8_t HALT = 0xFF;


/*

Create an array of strings for the mnemonics as well as an array of their corresponding opcodes
This allows us to find the appropriate opcode for a given mnemonic easily simply by:
  1) searching through the list of strings, and
  2) indexing to the same place in the second array

*/
const std::string instructions_list[num_instructions] = { "NOOP", "LOADA", "STOREA", "TAB", "TAX", "TAY", "TASP", "TASTATUS", "INCA", "DECA", "LOADB", "STOREB", "TBA", "TBX", "TBY", "TBSP", "TBSTATUS", "INCB", "DECB", "LOADX", "STOREX", "TXA", "TXB", "TXY", "TXSP", "INCX", "DECX", "LOADY", "STOREY", "TYA", "TYB", "TYX", "TYSP", "INCY", "DECY", "ROL", "ROR", "LSL", "LSR", "INCM", "DECM", "ADDCA", "ADDCB", "MULTA", "MULTUA", "DIVA", "DIVUA", "ANDA", "ORA", "XORA", "CMPA", "CMPB", "CMPX", "CMPY", "PHA", "PHB", "PLA", "PLB", "TSPA", "TSPB", "TSPX", "TSPY", "INCSP", "DECSP", "CLC", "SEC", "CLN", "SEN", "CLF", "SEF", "TSTATUSA", "TSTATUSB", "JMP", "BRNE", "BREQ", "BRGT", "BRLT", "BRZ", "BRN", "BRPL", "IRQ", "RTI", "JSR", "RTS", "SYSCALL", "RESET", "HALT"};
const uint8_t opcodes[num_instructions] = { NOOP, LOADA, STOREA, TAB, TAX, TAY, TASP, TASTATUS, INCA, DECA, LOADB, STOREB, TBA, TBX, TBY, TBSP, TBSTATUS, INCB, DECB, LOADX, STOREX, TXA, TXB, TXY, TXSP, INCX, DECX, LOADY, STOREY, TYA, TYB, TYX, TYSP, INCY, DECY, ROL, ROR, LSL, LSR, INCM, DECM, ADDCA, ADDCB, MULTA, MULTUA, DIVA, DIVUA, ANDA, ORA, XORA, CMPA, CMPB, CMPX, CMPY, PHA, PHB, PLA, PLB, TSPA, TSPB, TSPX, TSPY, INCSP, DECSP, CLC, SEC, CLN, SEN, CLF, SEF, TSTATUSA, TSTATUSB, JMP, BRNE, BREQ, BRGT, BRLT, BRZ, BRN, BRPL, IRQ, RTI, JSR, RTS, SYSCALL, RESET, HALT };


// Some opcodes stand by themselves; keep an array of them so that we can easily check
const size_t num_standalone_opcodes = 48;
const uint8_t standalone_opcodes[num_standalone_opcodes] = { NOOP, TAB, TAY, TAX, TASP, TASTATUS, INCA, DECA, TBA, TBX, TBY, TBSP, TBSTATUS, INCB, DECB, TXA, TXB, TXY, TXSP, INCX, DECX, TYA, TYB, TYX, TYSP, INCY, DECY, ROL, PHA, PLA, PHB, PLB, TSPA, TSPB, TSPX, TSPY, INCSP, DECSP, CLC, SEC, CLN, SEN, CLF, SEF, TSTATUSA, TSTATUSB, RESET, HALT };
