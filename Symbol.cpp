/*

SIN Toolchain
Symbol.cpp
Copyright 2019 Riley Lannon

The implementation of the Symbol struct

*/


#include "Symbol.h"


// Our Symbol object

Symbol::Symbol(std::string name, Type type, std::string scope_name, size_t scope_level, Type sub_type, SymbolQuality quality, bool defined, std::vector<std::shared_ptr<Statement>> formal_parameters) : name(name), type(type), scope_name(scope_name), scope_level(scope_level), sub_type(sub_type), quality(quality), defined(defined), formal_parameters(formal_parameters) {
	this->stack_offset = 0;
	this->freed = false;
}

Symbol::Symbol() {
	this->name = "";
	this->type = NONE;
	this->scope_name = "";
	this->scope_level = 0;
	this->quality = NO_QUALITY;
	this->defined = false;
	this->formal_parameters = {};
	this->stack_offset = 0;
	this->freed = false;
}

Symbol::~Symbol() {

}