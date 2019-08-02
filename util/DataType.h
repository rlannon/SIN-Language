/*

SIN Toolchain
DataType.h
Copyright 2019 Riley Lannon

Contains the definition of the 'DataType' class, which contains the type and subtype of a given expression alongside methods to evaluate and comapre it.

*/

#pragma once

#include <vector>
#include "EnumeratedTypes.h"

class DataType
{
	Type primary;
	Type subtype;
	std::vector<SymbolQuality> qualities;
	size_t array_length;
public:
	bool operator==(const DataType right);
	bool operator!=(const DataType right);

	Type get_type();
	Type get_subtype();
	std::vector<SymbolQuality> get_qualities();
	size_t get_array_length();

	void set_primary(Type new_primary);
	void set_subtype(Type new_subtype);
	void add_qualities(std::vector<SymbolQuality> to_add);

	bool is_compatible(DataType to_compare);

	DataType(Type primary, Type subtype = NONE, std::vector<SymbolQuality> qualities = {}, size_t array_length = 0);
	DataType();
	~DataType();
};
