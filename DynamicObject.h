#pragma once
#include <cinttypes>		// for uint__t types

class DynamicObject
{
	uint16_t start_address;	// start address for the object
	uint16_t size;	// size of the object, in bytes
public:
	uint16_t get_start_address();
	uint16_t get_size();

	bool operator==(const DynamicObject right);

	DynamicObject(uint16_t start_address, uint16_t size);
	DynamicObject();
	~DynamicObject();
};

