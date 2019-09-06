/*

SIN Toolchain
Signals.h
Copyright 2019 Riley Lannon

This header contains the numeric constants for the various signals used by the SIN VM to handle exception.

*/

#pragma once

#include <cinttypes>

const uint8_t SINSIGRESET = 0x09;	// reset the system
const uint8_t SINSIGFPE = 0x0A; // floating-point error; used for arithmetic errors
const uint8_t SINSIGSYS = 0x0B; // system call argument error
const uint8_t SINSIGILL = 0x0C; // illegal instruction
const uint8_t SINSIGSTKFLT = 0x0D;  // stack fault
const uint8_t SINSIGSEGV = 0x0E;  // segmentation violation (non-recoverable)
const uint8_t SINSIGKILL = 0x0F;  // kill (non-recoverable)
