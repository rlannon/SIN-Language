/*

SIN Toolchain
StatusConstants.h
Copyright 2019 Riley Lannon

The bit constants of the VM's STATUS register. They will be implemented here so that bit numbers are not needed when using the STATUS register, and constant names can be used instead.

*/

#pragma once

#include <cinttypes>

namespace StatusConstants
{
	/*

	The status register has the following layout:
		7	6	5	4	3	2	1	0
		N	V	U	H	I	F	Z	C
	Flag meanings:
		N: Negative
			Set when the result of the previous operation was negative
		V: Overflow
			Set when there was an integer overflow, or when the sign of the result of an ADDCA or SUBCA instruction is incorrect
		U: Undefined
			Generated when the result of an operation is undefined
		H: HALT Signal
			The processor received a HALT signal and will stop execution
		I: Interrupt Signal
			When an interrupt is requested, this flag will be set
		F: Floating-point
			Indicates the result of an operation is a floating-point number
		Z: Zero
			The result of the operation was zero; used in comparison and conditional branching. If the operands are equal, the Z flag will be set
		C: Carry
			The carry bit from an ADDCA instruction, or the borrow bit from a SUBCA instruction. This flag is also set when the result of a CMP yielded a result of "greater", and cleared when the result is "less"
	
	*/

	const uint8_t negative = 128;
	const uint8_t overflow = 64;
	const uint8_t undefined = 32;
	const uint8_t halt = 16;
	const uint8_t interrupt = 8;
	const uint8_t floating_point = 4;
	const uint8_t zero = 2;
	const uint8_t carry = 1;
}
