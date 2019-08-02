/*

SIN Toolchain
Symbol.cpp
Copyright 2019 Riley Lannon

The implementation of the Symbol struct

*/


#include "Symbol.h"

// Our Symbol object
Symbol::Symbol(std::string name, DataType type, std::string scope_name, size_t scope_level, bool defined, std::string struct_name) : 
	name(name), 
	type_information(type), 
	scope_name(scope_name), 
	scope_level(scope_level), 
	defined(defined),
	struct_name(struct_name) 
{
	this->symbol_type = VARIABLE;
	this->stack_offset = 0;
	this->allocated = false;
	this->freed = false;
}

Symbol::Symbol() {
	this->symbol_type = VARIABLE;
	this->name = "";
	this->type_information = DataType();
	this->scope_name = "";
	this->scope_level = 0;
	this->defined = false;
	this->allocated = false;
	this->struct_name = "";
	this->stack_offset = 0;
	this->freed = false;
}

Symbol::~Symbol() {

}

FunctionSymbol::FunctionSymbol(std::string name, DataType type_information, std::string scope_name, size_t scope_level, std::vector<std::shared_ptr<Statement>> formal_parameters) :
	Symbol(name, type_information, scope_name, scope_level, true), formal_parameters(formal_parameters)
{
	this->symbol_type = FUNCTION_DEFINITION;	// override the Symbol constructor's definition of this member
}

FunctionSymbol::FunctionSymbol(Symbol base_symbol, std::vector<std::shared_ptr<Statement>> formal_parameters) :
	Symbol(base_symbol), formal_parameters(formal_parameters)
{
	this->symbol_type = FUNCTION_DEFINITION;
}

FunctionSymbol::FunctionSymbol() {

}

FunctionSymbol::~FunctionSymbol() {

}
