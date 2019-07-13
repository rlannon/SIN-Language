/*

SIN Toolchain
DataWidths.h
Copyright 2019 Riley Lannon

Contains the widths (in bytes) of the various SIN data types.
This is to be used in lieu of hard-coded values (wherever they may be used) within the Compiler's source for readability.

*/

#pragma once

const size_t SINVM_INT_MIN = 0;
const size_t SINVM_INT_MAX = 0xFFFF;

const size_t WORD_W = 2;	// the SINVM uses a 16-bit word

const size_t BOOL_W = 2;	// a boolean is one word, though it _can_ be stored in a single byte
const size_t INT_W = 2;
const size_t HALF_W = 2;	// half-precision floating point numbers are 16 bits wide
const size_t SINGLE_W = 4;	// single-precision are 32 bits
const size_t DOUBLE_W = 8;  // double-precision are 64 bits

const size_t PTR_W = 2;	// pointers are always one word (the width of the address bus)

const size_t STRING_W = 4;	// strings are two words wide--one for the string length, one for its address
