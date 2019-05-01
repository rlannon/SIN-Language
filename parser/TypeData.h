/*

SIN Toolchain
TypeData.h
Copyright 2019 Riley Lannon

The definition of a struct to store type data about a variable. This is used by the Parser when creating allocation and declaration statements. Whenever type and quality data must be retrieved, this struct will be appropriate.

*/

#pragma once

#include <vector>
#include "../util/EnumeratedTypes.h"	// Type, SymbolQuality, etc.

struct TypeData {
	/*
	
	The only required member is data_type; symbols may have no subtype, no array length, and no qualities
	
	*/
	
	Type data_type;
	Type subtype;
	size_t array_length;
	std::vector<SymbolQuality> qualities;

	TypeData();
	TypeData(Type data_type, Type subtype = NONE, size_t array_length = 0, std::vector<SymbolQuality> qualities = {});
};
