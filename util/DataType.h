/*

SIN Toolchain
DataType.h
Copyright 2019 Riley Lannon

Contains the definitions of the 'SymbolQualities' and 'DataType' classes.
SymbolQualities contains the object used by the DataType class to store qualities so std::vector is not needed
DataType contains the type, subtype, and qualities of a given expression alongside methods to evaluate and comapre it.

*/

#pragma once

#include <vector>
#include "EnumeratedTypes.h"

class SymbolQualities
{
	bool const_q;	// our qualities -- since these are almost all reserved in C++, suffix with _q, for "quality"
	bool static_q;
	bool dynamic_q;
	bool signed_q;
	bool unsigned_q;
public:
	bool is_const();	// accessors
	bool is_static();
	bool is_dynamic();
	bool is_signed();
	bool is_unsigned();

	void add_qualities(std::vector<SymbolQuality> to_add);

	SymbolQualities(std::vector<SymbolQuality> qualities);
	SymbolQualities(bool is_const, bool is_static, bool is_dynamic, bool is_signed, bool is_unsigned);
	SymbolQualities();
	~SymbolQualities();
};

class DataType
{
	Type primary;
	Type subtype;
	SymbolQualities qualities;
	size_t array_length;
public:
	bool operator==(const DataType right);
	bool operator!=(const DataType right);

	bool operator==(const Type right[2]);
	bool operator!=(const Type right[2]);

	bool operator==(const Type right);
	bool operator!=(const Type right);

	Type get_primary();
	Type get_subtype();
	SymbolQualities get_qualities();
	size_t get_array_length();

	void set_primary(Type new_primary);
	void set_subtype(Type new_subtype);
	void add_qualities(std::vector<SymbolQuality> to_add);

	bool is_compatible(DataType to_compare);

	DataType(Type primary, Type subtype = NONE, std::vector<SymbolQuality> qualities = {}, size_t array_length = 0);
	DataType();
	~DataType();
};
