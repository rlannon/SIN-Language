#pragma once

#include <cinttypes>

/*

This header serves the purpose of defining the addressing modes used in SINASM as constants. This way, the code will be less obfuscated, and thus more readable, and more maintainable. These constants are used both in the SIN Assembler and in the SIN VM.

Here is a brief overview of the following addressing modes:

	The following are available for every instruction that takes an operand:
		absolute	-	$1234		-	use memory location
	
		x_index		-	$1234, x	-	use memory location + value in x
		y_index		-	$1234, y	-	"	"		"		+ value in y
	
		indirect indexed	-	($00), y	-	use the value at the supplied address as the address from which to fetch/store a value, indexed with a register
		indexed indirect	-	($00, x)	-	use the value at the indexed address as the address from which to fetch/store a value

	The following may not be used with store or bitshift instructions:
		immediate	-	#$1234		-	use supplied value

	The following are limited to a few select instructions:
		reg_a		-	A	-	use the a register as operand for operation
		reg_b		-	B	-	use the b register as operand for operation

	The following addressing modes may also be used with loadR and storeR instructions:
		single		-	S $1234		-	Handles a _single byte_ rather than a whole word
	The 'S' addressing modes are to be used when you wish to read data from or write data to a single byte rather than a whole word; this may only be used with absolute and indexed (direct or indirect) modes

*/

namespace addressingmode {
	const uint8_t absolute = 0x00;

	const uint8_t x_index = 0x01;
	const uint8_t y_index = 0x02;

	const uint8_t immediate = 0x03;

	// currently, 0x04 is unused

	const uint8_t indirect_indexed_x = 0x05;	// syntax is (addr), x
	const uint8_t indirect_indexed_y = 0x06;

	const uint8_t indexed_indirect_x = 0x07;	// syntax is (addr, x)
	const uint8_t indexed_indirect_y = 0x08;

	const uint8_t reg_a = 0x09;
	const uint8_t reg_b = 0x0A;

	// single byte modes; these mirror their word counterparts (+0x00 vs +0x10)
	const uint8_t absolute_short = 0x10;
	
	const uint8_t x_index_short = 0x11;
	const uint8_t y_index_short = 0x12;

	const uint8_t indirect_indexed_x_short = 0x15;
	const uint8_t indirect_indexed_y_short = 0x16;
	
	const uint8_t indexed_indirect_x_short = 0x17;
	const uint8_t indexed_indirect_y_short = 0x18;
}
