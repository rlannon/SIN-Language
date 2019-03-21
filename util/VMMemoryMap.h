#pragma once

/*

For SIN VM version 1

Contains the constants that define where our various blocks of memory in the VM begin

Our addresses range from $0000 to $FFFF (65k):
	- All variables will get stored from $0000 up to (but not including) $1400
	- An input buffer lives from $1400 to $17FF
	- The stack lives from $1800 to (but not including) $2400
	- The call stack lives from $2400 to (but not including) $2600
	- All included program data lives from $2600 to $f000
	- The arguments and command line data take up the last few pages -- $f000 to $ffff

Note that the stacks in the VM grow /downward/ while the heap grows /upward/

*/

// declare how much memory the virtual machine has
const size_t memory_size = 0x10000;	// 16k available to the VM

// declare our start addresses for different sections of memory

// lower limit
const size_t _MEMORY_MIN = 0x0000;

// the zero page will be reserved for use as a table of pointers
const size_t _POINTER_TABLE_BOTTOM = 0x0002;	// do not start at address 0x00 so that null pointers will point to nothing
const size_t _LOCAL_DYNAMIC_POINTER = 0x0002;	// this address serves as a temp variable to hold a pointer to dynamic memory during allocation
const size_t _POINTER_TABLE_TOP = 0x00FF;

// the RS directive, which creates global variables, will allocate variables starting at 0x0100 and have 3 pages
const size_t _RS_START = 0x0100;
const size_t _RS_END = 0x03FF;

// any variables allocated on the heap will be allocated starting at 0x0400 up to the input buffer; this is where all dynamic memory will be stored
const size_t _HEAP_START = 0x0400;
const size_t _HEAP_MAX = 0x13FF;

const size_t _STRING_BUFFER_START = 0x1400;	// a ~1K buffer for string and input data
const size_t _STRING_BUFFER_MAX = 0x17FF;

// the stack -- this is used for all of our scope data, and so it gets quite a bit of memory
const size_t _STACK = 0x23FF;	// our stack grows downwards
const size_t _STACK_BOTTOM = 0x1800;	// the bottom of the stack

// call stack gets 2 pages
const size_t _CALL_STACK = 0x25FF;	// the call stack also grows downwards
const size_t _CALL_STACK_BOTTOM = 0x2400;

// program data itself
const size_t _PRG_TOP = 0xEFFF;	// the limit for our program data
const size_t _PRG_BOTTOM = 0x2600;	// our lowest possible memory address for the program

// program environment / command - line arguments
const size_t _ARG = 0xF000;	// f000 - ffff available for command-line/environment arguments

// upper limit
const size_t _MEMORY_MAX = 0xFFFF;
