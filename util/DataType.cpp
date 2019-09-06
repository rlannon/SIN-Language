/*

SIN Toolchain
DataType.cpp
Copyright 2019 Riley Lannon

Implementation of the DataType class

*/

#include "DataType.h"

/*

SymbolQualities implementation

*/

bool SymbolQualities::is_const()
{
	return const_q;
}

bool SymbolQualities::is_dynamic()
{
	return dynamic_q;
}

bool SymbolQualities::is_static()
{
	return static_q;
}

bool SymbolQualities::is_signed()
{
	return signed_q;
}

bool SymbolQualities::is_unsigned()
{
	return unsigned_q;
}

void SymbolQualities::add_qualities(std::vector<SymbolQuality> to_add)
{
	// simply populate the vector; since we are adding, we don't really care about the original values
	for (std::vector<SymbolQuality>::iterator it = to_add.begin(); it != to_add.end(); it++)
	{
		if (*it == CONSTANT)
		{
			const_q = true;
		}
		else if (*it == STATIC)
		{
			static_q = true;
		}
		else if (*it == DYNAMIC)
		{
			dynamic_q = true;
		}
		else if (*it == SIGNED)
		{
			signed_q = true;
		}
		else if (*it == UNSIGNED)
		{
			unsigned_q = true;
			signed_q = false;
		}
		else {
			continue;
		}
	}
}

SymbolQualities::SymbolQualities(std::vector<SymbolQuality> qualities)
{
	// start with our default values
	const_q = false;
	static_q = false;
	dynamic_q = false;
	signed_q = false;
	unsigned_q = false;

	// then, populate according to the vector
	for (std::vector<SymbolQuality>::iterator it = qualities.begin(); it != qualities.end(); it++)
	{
		if (*it == CONSTANT)
		{
			const_q = true;
		}
		else if (*it == STATIC)
		{
			static_q = true;
		}
		else if (*it == DYNAMIC)
		{
			dynamic_q = true;
		}
		else if (*it == SIGNED)
		{
			signed_q = true;
		}
		else if (*it == UNSIGNED)
		{
			unsigned_q = true;
		}
		else {
			continue;
		}
	}
}

SymbolQualities::SymbolQualities(bool c, bool s, bool d, bool sg, bool us) :
	const_q(c),
	static_q(s),
	dynamic_q(d),
	signed_q(sg),
	unsigned_q(us)
{
	if (SymbolQualities::unsigned_q) {
		SymbolQualities::signed_q = false;
	}

	if (SymbolQualities::const_q) {
		SymbolQualities::static_q, SymbolQualities::dynamic_q = false;
	}
}

SymbolQualities::SymbolQualities()
{
	// everything defaults to false if nothing is given
	const_q = false;
	static_q = false;
	dynamic_q = false;
	signed_q = false;
	unsigned_q = false;
}

SymbolQualities::~SymbolQualities()
{

}

/*

We can use the overloaded operators to do any of the following:
	- See if two DataType objects are equal
	- See if DataType is equal to {Type, Type}
	- See if DataType is equal to Type

*/

bool DataType::operator==(const DataType right)
{
	return (this->primary == right.primary) && (this->subtype == right.subtype);
}

bool DataType::operator!=(const DataType right)
{
	return (this->primary != right.primary) || (this->subtype != right.subtype);
}

bool DataType::operator==(const Type right[2])
{
	return (this->primary == right[0]) && (this->subtype == right[1]);
}

bool DataType::operator!=(const Type right[2])
{
	return (this->primary != right[0]) || (this->subtype != right[1]);
}

bool DataType::operator==(const Type right)
{
	return this->primary == right;
}

bool DataType::operator!=(const Type right)
{
	return this->primary != right;
}

bool DataType::is_compatible(DataType to_compare)
{
	/*
	
	Compares 'self' with 'to_compare'. Types are compatible if one of the following is true:
		- if pointer or array type:
			- subtypes are equal
			- one of the subtypes is RAW
		- left OR right is RAW

	*/

	if (this->primary == RAW || to_compare.get_primary() == RAW) {
		return true;
	}
	else if ((this->primary == PTR && to_compare.get_primary() == PTR) || (this->primary == ARRAY && to_compare.get_primary() == ARRAY))
	{
		// cast the subtypes to DataType (with a subtype of NONE) and call is_compatible on them
		return static_cast<DataType>(this->subtype).is_compatible(static_cast<DataType>(to_compare.get_subtype()));
	}
	else {
		Type right;
		Type left;

		if (this->primary == ARRAY)
		{
			right = this->subtype;
		}
		else {
			right = this->primary;
		}

		if (to_compare.get_primary() == ARRAY)
		{
			left = to_compare.get_subtype();
		}
		else {
			left = to_compare.get_primary();
		}

		return right == left;
	}
}

Type DataType::get_primary()
{
	return this->primary;
}

Type DataType::get_subtype()
{
	return this->subtype;
}

SymbolQualities DataType::get_qualities() {
	return this->qualities;
}

size_t DataType::get_array_length() {
	return this->array_length;
}

void DataType::set_primary(Type new_primary) {
	this->primary = new_primary;
}

void DataType::set_subtype(Type new_subtype) {
	this->subtype = new_subtype;
}

void DataType::add_qualities(std::vector<SymbolQuality> to_add) {
	// simply use the "SymbolQualities::add_qualities" function
	this->qualities.add_qualities(to_add);
}

DataType::DataType(Type primary, Type subtype, std::vector<SymbolQuality> qualities, size_t array_length) :
	primary(primary),
	subtype(subtype),
	array_length(array_length)
{
	this->qualities = SymbolQualities(qualities);	// use the vector constructor
	
	// if the type is int, set signed to true if it is not unsigned
	if (primary == INT && !this->qualities.is_unsigned()) {
		this->add_qualities({ SIGNED });
	}
	else if (primary == FLOAT)
	{
		this->add_qualities({ SIGNED });
	}
}

DataType::DataType()
{
	this->primary = NONE;
	this->subtype = NONE;
	this->qualities = SymbolQualities();	// no qualities to start
	this->array_length = 0;
}

DataType::~DataType()
{

}
