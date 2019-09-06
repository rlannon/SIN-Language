/*

BinaryIO
Copyright 2019 Riley Lannon

This header lays out all of the necessary functions for writing and reading binary files. It establishes our fundamental data types and functions for reading/writing data in binary format.
Note that these functions are used for reading and writing *unsigned* 8/16/32 bit values; for signed values, simply cast the signed integer to an unsigned value before writing and cast back to a signed value after reading.

Note also that the library's default byte order is little endian, but big endian can also be used by using the byteorder parameter.

*/

#pragma once

#include <iostream>
#include <string>

// put all of these functions into a namespace
namespace BinaryIO {

	// We need readU8 and writeU8 because of the terminator character -- we can't simply read or write 1 byte with file.write() and file.read().
	uint8_t readU8(std::istream& file);
	void writeU8(std::ostream& file, uint8_t val);  // write a single byte to the file

	uint16_t readU16(std::istream& file, std::string byteorder = "little");
	void writeU16(std::ostream& file, uint16_t val, std::string byteorder = "little");  // write 2 bytes to the file

	uint32_t readU32(std::istream& file, std::string byteorder = "little");
	void writeU32(std::ostream& file, uint32_t val, std::string byteorder = "little");  // write 4 bytes to the file

	// store floats as uint32_t; convert back to float when we read the value out
	uint32_t convertFloat(float n);
	float convertUnsigned(uint32_t n);

	// Must write the length of the string to the file, then the string data so we know how long our string actually is
	// Note the current max string length is 2^16 (ASCII) characters
	std::string readString(std::istream& file, std::string byteorder = "little");
	void writeString(std::ostream& file, std::string str, std::string byteorder = "little");
}
