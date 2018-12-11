#pragma once

#include <iostream>
#include <string>

/*

This header lays out all of the necessary functions for writing and reading binary files. It establishes our fundamental data types and functions for reading/writing those types to binary files.
This header only needs to be included in the .cpp files for a file format specification; the header files shouldn't need any of these functions.

Note that these functions are used for reading and writing *unsigned* 8/16/32 bit values; for signed values, simply cast the signed integer to an unsigned value before writing and cast back to a signed value after reading.

Included here also also functions for returning the proper data path; this is by default set to data/ but having one place to change it makes it easier for future changes in library hierarchy.

*/

// We need readU8 and writeU8 because of the terminator character -- we can't simply read or write 1 byte with file.write() and file.read().
uint8_t readU8(std::istream& file);
void writeU8(std::ostream& file, uint8_t val);

uint16_t readU16(std::istream& file);
void writeU16(std::ostream& file, uint16_t val);

uint32_t readU32(std::istream& file);
void writeU32(std::ostream& file, uint32_t val);

// store float as uint8_t, uint16_t, or uint32_t
uint32_t convertFloat(float n);
float convertUnsigned(uint32_t n);

// Must write the length of the string to the file, then the string data so we know how long our string actually is
// Note the max string length is 2^16, far longer than we will be needing, but we want to be safe
std::string readString(std::istream& file);
void writeString(std::ostream& file, std::string str);
