/*

SIN Toolchain
TypeData.cpp
Copyright 2019 Riley Lannon

The implementation of the member functions for struct TypeData (the constructors)

*/

#include "TypeData.h"

TypeData::TypeData() {
	
}

TypeData::TypeData(Type data_type, Type subtype, size_t array_length, std::vector<SymbolQuality> qualities) : data_type(data_type), subtype(subtype), array_length(array_length), qualities(qualities) {

}
