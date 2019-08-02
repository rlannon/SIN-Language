/*

SIN Toolchain
DataType.cpp
Copyright 2019 Riley Lannon

Implementation of the DataType class

*/

#include "DataType.h"

bool DataType::operator==(const DataType right)
{
	return (this->primary == right.primary) && (this->subtype == right.subtype);
}

bool DataType::operator!=(const DataType right)
{
	return (this->primary != right.primary) || (this->subtype != right.subtype);
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

	if (this->primary == RAW || to_compare.get_type() == RAW) {
		return true;
	}
	else if ((this->primary == PTR && to_compare.get_type() == PTR) || (this->primary == ARRAY && to_compare.get_type() == ARRAY))
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

		if (to_compare.get_type() == ARRAY)
		{
			left = to_compare.get_subtype();
		}
		else {
			left = to_compare.get_type();
		}

		return right == left;
	}
}

Type DataType::get_type()
{
	return this->primary;
}

Type DataType::get_subtype()
{
	return this->subtype;
}

std::vector<SymbolQuality> DataType::get_qualities() {
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
	/*

	Add the qualities in 'to_add' to the type information, ignoring duplicates

	*/

	// todo: skip duplicates
	this->qualities.insert(this->qualities.begin(), to_add.begin(), to_add.end());
}

DataType::DataType(Type primary, Type subtype, std::vector<SymbolQuality> qualities, size_t array_length) :
	primary(primary),
	subtype(subtype),
	qualities(qualities),
	array_length(array_length)
{

}

DataType::DataType()
{
	this->primary = NONE;
	this->subtype = NONE;
	this->qualities = {};
	this->array_length = 0;
}

DataType::~DataType()
{

}
