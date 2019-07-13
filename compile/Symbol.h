/*

SIN Toolchain
Symbol.h
Copyright 2019 Riley Lannon

The definition of the "Symbol" object, used in the Compiler's Symbol Table. Symbols contain information regarding:
	- The name of the symbol
	- The data type
	- The subtype, if applicable
	- The name of the scope in which the symbol occurs
	- The level of the scope in which the symbol occurs
	- Whether the symbol has been defined
	- Whether the symbol has been freed (used for dynamic memory and garbage collection)
	- The symbol's quality (Const, Dynamic, Static...)
	- The offset from the start of the current scope's stack frame whether the symbol occurs; used for determining where local variables are stored

The definition of the "FunctionSymbol" object, which contains:
	- Formal parameters, if the symbol is a function

*/

#pragma once

#include <string>
#include <vector>
#include <tuple>

#include "../parser/Statement.h"
#include "../util/EnumeratedTypes.h"



struct Symbol
{
	/*

	A struct to contain our Symbol data; this contains the variable's name, type, scope level, whether it is defined, its stack offset (if a local variable)

	*/

	SymbolType symbol_type;

	std::string name;	// the name of the variable / function
	Type type;	// the variable type (for functions, the return type)
	Type sub_type;	// the subtype -- e.g., on "ptr<int>", the type is "ptr" and the subtype is "int"

	std::string scope_name;	// the name of the scope -- either "global" or the name of the function
	size_t scope_level;	// the /level/ of scope within the program; if we are in a loop or ite block, the level will increase

	bool defined;	// tracks whether the variable has been defined; we cannot use it before it is defined
	bool allocated;	// tracks whether dynamic memory has been allocated on the heap
	bool freed;	// tracks whether the variable has been freed; this is used for dynamic memory when we want to do garbage collection
	std::vector<SymbolQuality> qualities;	// tells us whether something is const, etc.

	size_t stack_offset;	// used for local symbols to determine the offset (in words) from the initial address of the SP

	size_t array_length;	// used only for arrays; contains the size of the array
	std::string struct_name;	// used only for structs; contains the name of the struct

	// constructor/destructor
	Symbol(std::string name, Type type, std::string scope_name, size_t scope_level, Type sub_type = NONE, std::vector<SymbolQuality> quality = {}, bool defined = false, size_t array_length = 0, std::string struct_name = "");
	Symbol();
	virtual ~Symbol();
};

struct FunctionSymbol : public Symbol
{
	/*
	
	Function symbols also contain the formal parameters for the function, as a vector of Statement objects

	*/
	
	std::vector<std::shared_ptr<Statement>> formal_parameters;

	FunctionSymbol(std::string name, Type type, std::string scope_name, size_t scope_level, Type sub_type = NONE, std::vector<SymbolQuality> quality = {},
		size_t array_length = 0, std::vector<std::shared_ptr<Statement>> formal_parameters = {});
	FunctionSymbol(Symbol base_symbol, std::vector<std::shared_ptr<Statement>> formal_parameters);
	FunctionSymbol();
	~FunctionSymbol();
};
