#pragma once

#include <iostream>

/*

This header serves the purpose of defining the addressing modes used in SINASM as constants. This way, the code will be less obfuscated, and thus more readable, and more maintainable. These constants are used both in the SIN Assembler and in the SIN VM.

*/

namespace addressmode {
	const uint8_t absolute = 0x00;
	const uint8_t x_index = 0x01;
	const uint8_t y_index = 0x02;
	const uint8_t immediate = 0x03;
	// currently, addressing mode 0x04 is unused
	const uint8_t indirect_x = 0x05;
	const uint8_t indirect_y = 0x06;
	const uint8_t reg_a = 0x07;
	const uint8_t reg_b = 0x08;
}
