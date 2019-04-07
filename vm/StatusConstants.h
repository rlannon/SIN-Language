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
		N	V	0	H	D	F	Z	C
	Flag meanings:
		N: Negative
		V: Overflow
		H: HALT instruction executed
		D: Dynamic memory failbit
		F: Floating-point
		Z: Zero
		C: Carry
	Notes:
		- The Z flag is also used for equality; compare instructions will set the zero flag if the operands are equal
	
	*/

	const uint8_t negative = 128;
	const uint8_t overflow = 64;
	// currently, the 5th bit is unused
	const uint8_t halt = 16;
	const uint8_t dynamic_fail = 8;
	const uint8_t floating_point = 4;
	const uint8_t zero = 2;
	const uint8_t carry = 1;
}
