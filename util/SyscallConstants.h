/*

SIN Toolchain
SyscallConstants.h
Copyright 2019 Riley Lannon

This file is to be used for the names of the various syscall constants so that their use can be more maintainable.

*/

#pragma once

#include <cinttypes>  // for uint16_t

const uint16_t STD_FILEOPEN_R = 0x10;
const uint16_t STD_FILEOPEN_w = 0x11;
const uint16_t STD_FILECLOSE = 0x12;

const uint16_t STD_READ = 0x13;

const uint16_t STD_OUT = 0x14;
const uint16_t STD_OUT_HEX = 0x15;

const uint16_t MEMFREE = 0x20;
const uint16_t MEMALLOC = 0x21;
const uint16_t MEMREALLOC = 0x22;
const uint16_t MEMREALLOC_SAFE = 0x23;

const uint16_t SYS_EXIT = 0xFF;
