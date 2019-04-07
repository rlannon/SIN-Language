/*

SIN Toolchain
ALU.cpp
Copyright 2019 Riley Lannon

This file contains the implementation of all of the SINVM functions that pertain to its ALU.

*/

#include "ALU.h"

void ALU::add(uint16_t right)
{	
	/*
	
	Adds REG_A to some value 'right' with carry.
	
	The operation affects the following flags:
		- N: if the sign bit is set, sets the N flag
		- V: if neither value is signed (less than 0x8000), but the sign bit is set in the result, the overflow flag is set
		- Z: if the result is zero, sets the zero flag
		- C: if the result is greater than 0xFFFF, the C flag is set
	
	*/

	// get the result
	uint16_t result = *this->REG_A + right + (*this->STATUS & StatusConstants::carry);	// we add the value of the carry bit in

	// update the STATUS register; first, clear the N, V, Z, and C flags
	*this->STATUS &= (0xFFFF - (StatusConstants::negative + StatusConstants::overflow + StatusConstants::zero + StatusConstants::carry));

	// if the result is zero, set the Z flag and then we are done -- the other flags can't possibly be set if the result is 0
	if (result == 0) {
		*this->STATUS |= StatusConstants::zero;
	}
	// otherwise, we will need to update the other flags
	else {
		// if the MSB is set in the result, set the N flag
		if (result & 0x8000) {
			*this->STATUS |= StatusConstants::negative;
		}

		// if (result - right) is not equal to REG_A, we need to set the carry flag
		if (result - right != *this->REG_A) {
			*this->STATUS |= StatusConstants::carry;
		}

		// if the sign bit is not set in either initial value, but it's set in the result, we need to set the overflow flag
		if ( (!(right & 0x8000) && !(*this->REG_A & 0x8000)) && (result & 0x8000)) {
			*this->STATUS |= StatusConstants::overflow;
		}
	}

	// finally, put the result into REG_A and return
	*this->REG_A = result;
	return;
}

void ALU::sub(uint16_t right)
{
	/*
	
	Subtract with carry 'right' from REG_A, storing the result in REG_A
	
	It affects the following flags:
		- N: if the sign bit is set, the N flag is set
		- V: if the N flag
		- Z: if the result is 0, the Z flag is set and the N and V flags are cleared
		- C: cleared to indicate a borrow occurring

	Note the subtraction algorithm does not just subtract right from REG_A; if the carry bit is set, this will occur, but if it is clear, it indicates a borrow occurred and the result will be one less than expected.
	This mode of operation allows for easier 32-bit arithmetic because if a borrow occurs in the low word, the subtraction of the high word will account for it

	*/

	// get the result
	uint16_t result = 0xFFFF + *this->REG_A - right + (*this->STATUS & StatusConstants::carry);

	// next, clear the appropriate bits in the status register
	*this->STATUS &= 0xFF - (StatusConstants::negative + StatusConstants::overflow + StatusConstants::zero + StatusConstants::carry);

	// next, if (result + right) == REG_A, we need to set the carry
	if (result + right == *this->REG_A) {
		*this->STATUS |= StatusConstants::carry;
	}
	// otherwise, set the overflow
	else {
		*this->STATUS |= StatusConstants::overflow;
	}

	// if the result was zero, set the Z flag
	if (result == 0) {
		*this->STATUS |= StatusConstants::zero;
	}
	// otherwise, we may need to set the N flag, if the MSB is set
	else if (result & 0x8000) {
		*this->STATUS |= StatusConstants::negative;
	}
	// if the result is not zero, but the MSB is not set, we are done

	// set REG_A equal to result
	*this->REG_A = result;

	return;
}

void ALU::mult_unsigned(uint16_t right)
{
	// Perform unsigned multiplication on two values

	// perform the multiplication
	uint16_t result = *this->REG_A * right;

	// check for integer overflow; we won't have overflow if the result is equal to REG_A divided by right
	if (result / right == *this->REG_A) {
		*this->REG_A = result;
	}
	else {
		// if we have an overflow, set the V flag
		*this->STATUS |= StatusConstants::overflow;
	}
	
	// we are done; return
	return;
}

void ALU::mult_signed(uint16_t right)
{
	/*
	
	Perform signed multiplication on two values. We also have to check to ensure there is no integer overflow or underflow
	First, check to see if the sign bit is set for either value; if so, perform two's complement on that value
		To convert from signed, simply flip all of the bits in the _signed_ notation and add 1
		To convert to signed, subtract one and flip all of the bits
		Note bits can be flipped simply by using XOR 0xFFFF (all 1s will become 0 and all 0s will become 1)
	Next, perform the multiplication
	Next, check to see if the sign bit on the result is set; if so, we have integer overflow. Also, check to see if the REG_A is equal to result / right
	Finally, if exactly one of the sign bits was set, perform two's complement on the result; else, leave it

	*/

	bool left_signed = *this->REG_A & 0x8000;	// if the most significant bit is set, the value is signed
	bool right_signed = right & 0x8000;
	uint16_t result;

	// perform twos complement, if necessary
	if (left_signed) {
		*this->REG_A ^= 0xFFFF;
		*this->REG_A += 1;
	}
	if (right_signed) {
		right ^= 0xFFFF;
		right += 1;
	}

	// perform the multiplication
	result = *this->REG_A * right;

	// check to see if the MSB is set, or if REG_A is not equal to result / right
	if ((result & 0x8000) || (result / right != *this->REG_A)) {
		*this->STATUS |= StatusConstants::overflow;	// if so, set the overflow flag
	}

	// now, if exactly one bit was set, perform two's complement; if the flags are the same, we don't need to do it
	if (left_signed == right_signed) {
		*this->STATUS &= (0xFF - StatusConstants::negative);	// clear the negative flag by using STATUS AND (0xFF - bit_number)
	}
	else {
		// perform two's complement
		*this->REG_A -= 1;
		*this->REG_A ^= 0xFFFF;

		*this->STATUS |= StatusConstants::negative;	// set the negative flag
	}

	// we are done; return
	return;
}

void ALU::div_unsigned(uint16_t right)
{

}

void ALU::div_signed(uint16_t right)
{

}

ALU::ALU(uint16_t * REG_A, uint16_t* REG_B, uint8_t * STATUS) : REG_A(REG_A), REG_B(REG_B), STATUS(STATUS)
{
}

ALU::ALU() {
	// if we are not given any initial values, initialize all of the data members to nullptr
	this->REG_A = nullptr;
	this->REG_B = nullptr;
	this->STATUS = nullptr;
}

ALU::~ALU()
{
}
