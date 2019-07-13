/*

SIN Toolchain
FPU.h
Copyright 2019 Riley Lannon

Definition of the FPU (floating-point unit) class

*/

#pragma once

#include <cinttypes>
#include "../util/DataWidths.h"
#include "StatusConstants.h"

class FPU {
	// Since the FPU can operate in 16- or 32-bit mode, there are separate implementations for each function

	uint16_t* REG_A;
	uint16_t* REG_B;

	uint8_t* STATUS;

	uint32_t combine_registers();
	void split_to_registers(uint32_t to_split);

	static uint32_t unpack_16(uint16_t to_unpack);
	static uint16_t pack_32(uint32_t to_pack);
public:
	// 16-bit
	void fadda(uint16_t right);
	void fsuba(uint16_t right);
	void fmulta(uint16_t right);
	void fdiva(uint16_t right);

	// 32-bit
	void single_fadda(uint32_t right);
	void single_fsuba(uint32_t right);
	void single_fmulta(uint32_t right);
	void single_fdiva(uint32_t right);

	FPU(uint16_t* REG_A, uint16_t* REG_B, uint8_t* STATUS);
	FPU();
	~FPU();
};
