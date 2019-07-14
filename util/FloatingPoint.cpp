/*

SIN Toolchain
FloatingPoint.cpp
Copyright 2019 Riley Lannon

*/

#include "FloatingPoint.h"

uint32_t unpack_16(uint16_t to_unpack) {
	/*

	Unpacks a 16-bit floating point number as a 32-bit one

	The exponents have a bias of 15 and 127, respectively; subtract 15 or 127 when determining the actual exponent.

	16-bit format is as follows:
		0	00000	0000000000
		S	Exp		Mantissa (11)

	32-bit is as follows:
		0	00000000	00000000000000000000000
		S	Exp (5)		Mantissa (23)

	So the floating point number 12.3 would be:
		16:		0	10010		1000100110
		32:		0	10000010	10001001100110011001101
	Note that converting 16->32 would yield:
		32:		0	10000010	10001001100000000000000
	which is 12.296875, not quite 12.3

	*/

	uint8_t sign = (to_unpack & 0x8000) >> 15;
	uint8_t exponent = (to_unpack & 0x7C00) >> 10;	// exponent is 5 bits
	uint16_t mantissa = (to_unpack & 0x3FF);	// mantissa is 10 bits

	// adjust the exponent
	if ((exponent & 16) == 16) {
		exponent -= 15;		// subtract the half-precision bias
		exponent += 127;	// add in the single-precision bias
	}

	// assemble the 32-bit float by putting all the bits together, shifting appropriately
	uint32_t single = 0;
	single |= (sign << 31);
	single |= (exponent << 23);
	single |= (mantissa) << 13;

	return single;
}

uint16_t pack_32(uint32_t to_pack) {
	// Packs a 32-bit floating point number as a 16-bit one
	uint8_t sign = (to_pack & 0x80000000) >> 31;
	uint8_t exponent = (to_pack & 0x7F800000) >> 23;
	uint16_t mantissa = (to_pack & 0x00FFFFFF) >> 13;

	// adjust the exponent
	if ((exponent & 128) == 128) {
		exponent -= 127;
		exponent += 15;
	}

	uint16_t half = 0;

	half |= (sign << 15);
	half |= (exponent << 10);
	half |= mantissa;

	return half;
}