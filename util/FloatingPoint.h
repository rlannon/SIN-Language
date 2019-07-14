/*

SIN Toolchain
FloatingPoint.h
Copyright 2019 Riley Lannon

unpack_16 and pack_32 were originally a part of the FPU class, but they were moved here because their functionality will prove useful in Compiler as well.

*/

#pragma once
#include <cinttypes>

uint32_t unpack_16(uint16_t to_unpack);
uint16_t pack_32(uint32_t to_pack);
