#include "BinaryIO.h"

// Implementation of the functions declared in 'BinaryIO.h'

// READ/WRITE UINT8_T

uint8_t BinaryIO::readU8(std::istream& file) {
	uint8_t val;
	uint8_t bytes[2];
	file.read((char*)bytes, 1);
	val = bytes[0];
	return val;
}
void BinaryIO::writeU8(std::ostream& file, uint8_t val) {
	uint8_t bytes[2];

	bytes[0] = val;

	file.write((char*)bytes, 1);
}

// READ/WRITE UINT16_T

uint16_t BinaryIO::readU16(std::istream& file, std::string byteorder) {
	uint16_t val;
	uint8_t bytes[2];

	file.read((char*)bytes, 2); // read 2 bytes from the file

	// copy the bytes into "val" according to our byte order
	if (byteorder == "little") {
		val = bytes[0] | (bytes[1] << 8);
	}
	else if (byteorder == "big") {
		val = (bytes[1] << 8) | bytes[0];
	}
	// if it is not little and it is not "big" , throw an exception
	else {
		throw std::exception(("Invalid byte order specifier '" + byteorder + "'; must be 'big' or 'little' (or none specified).").c_str());
	}

	return val;
}
void BinaryIO::writeU16(std::ostream& file, uint16_t val, std::string byteorder) {
	uint8_t bytes[2];

	if (byteorder == "little") {
		// extract individual bytes from our 16-bit value
		bytes[0] = (val) & 0xFF;	// low byte
		bytes[1] = (val >> 8) & 0xFF;	// high byte
	}
	else if (byteorder == "big") {
		// extract the individual bytes
		bytes[0] = (val >> 8) & 0xFF;	// high byte
		bytes[1] = (val) & 0xFF;	// low byte
	}
	else {
		throw std::exception(("Invalid byte order specifier '" + byteorder + "'; must be 'big' or 'little' (or none specified).").c_str());
	}

	file.write((char*)bytes, 2);
}

uint32_t BinaryIO::readU32(std::istream& file, std::string byteorder) {
	uint32_t val;
	uint8_t bytes[4];

	file.read((char*)bytes, 4);

	// copy the bytes into "val" according to our byte order
	if (byteorder == "little") {
		val = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
	}
	else if (byteorder == "big") {
		val = (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
	}
	else {
		throw std::exception(("Invalid byte order specifier '" + byteorder + "'; must be 'big' or 'little' (or none specified).").c_str());
	}

	return val;
}
void BinaryIO::writeU32(std::ostream& file, uint32_t val, std::string byteorder) {
	uint8_t bytes[4];

	// extract individual bytes from 32-bit value according to our byteorder
	if (byteorder == "little") {
		bytes[0] = (val) & 0xFF; // lowest
		bytes[1] = (val >> 8) & 0xFF;
		bytes[2] = (val >> 16) & 0xFF;
		bytes[3] = (val >> 24) & 0xFF;
	}
	else if (byteorder == "big") {
		bytes[0] = (val >> 24) & 0xFF;	// highest
		bytes[1] = (val >> 16) & 0xFF;
		bytes[2] = (val >> 8) & 0xFF;
		bytes[3] = (val) & 0xFF;
	}
	else {
		throw std::exception(("Invalid byte order specifier '" + byteorder + "'; must be 'big' or 'little' (or none specified).").c_str());
	}

	file.write((char*)bytes, 4);
}

// CONVERT TO/FROM FLOAT & UINT32_T

uint32_t BinaryIO::convertFloat(float n) {
	return *reinterpret_cast<uint32_t*>(&n);
}
float BinaryIO::convertUnsigned(uint32_t n) {
	return reinterpret_cast<float&> (n);
}

// READ/WRITE STRING AND STRING LENGTH

std::string BinaryIO::readString(std::istream& file, std::string byteorder) {
	uint16_t len = readU16(file, byteorder);	// current max string length (for a single string) is 2^16 characters

	char* buffer = new char[len];
	file.read(buffer, len);

	std::string str(buffer, len);
	delete[] buffer;

	return str;
}
void BinaryIO::writeString(std::ostream& file, std::string str, std::string byteorder) {
	// check to make sure the string length does not exceed our maximum allowed size
	if (str.length() > 0xFFFF) {
		throw std::exception("String length too large; length must be able to be expressed as a 16-bit integer (i.e. it must be between 0 and 65,535 bytes long)");
	}
	else {
		uint16_t len = (uint16_t)str.length(); // note this means our max string length (for /one single/ string) is 65K; this should be okay
		writeU16(file, len, byteorder);

		file.write(str.c_str(), len);
	}
}
