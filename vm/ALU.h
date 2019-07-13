/*

SIN Toolchain
ALU.h
Copyright 2019 Riley Lannon

This contains the definition of the ALU class, which is used by the SIN VM to carry out instructions pertaining to the ALU (i.e., arithmetic and logic).
Note this does not contain the FPU implementation; that can be found in FPU.h

*/

#pragma once

#include <cinttypes>
#include "../util/DataWidths.h"
#include "StatusConstants.h"


class ALU {
	/*

	Since the ALU will be modifying register values, we need pointers to the ones it can access
	It only ever needs to access the accumulator (stores its values there), the B register (for storing remainders in divisions), and the STATUS register (it may need to update flags)

	*/
	uint16_t* REG_A;
	uint16_t* REG_B;
	uint8_t* STATUS;
public:
	/*
	
	All of the manipulation functions will be public. The left argument is always the A register (to which this object has a pointer), and the right argument is always supplied.
	
	*/
	void add(uint16_t right);
	void sub(uint16_t right);

	void mult_unsigned(uint16_t right);
	void mult_signed(uint16_t right);

	void div_unsigned(uint16_t right);
	void div_signed(uint16_t right);

	ALU(uint16_t* REG_A, uint16_t* REG_B, uint8_t* STATUS);
	ALU();
	~ALU();
};
