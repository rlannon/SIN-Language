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
	- Formal parameters, if the symbol is a function

*/

#pragma once

#include <string>
#include <vector>
#include <tuple>

#include "Statement.h"
#include "EnumeratedTypes.h"



typedef struct Symbol
{
	/*

	A struct to contain our Symbol data; this contains the variable's name, type, scope level, whether it is defined, its stack offset (if a local variable), and, if it is a function, a vector of Statements containing that symbol's formal parameters

	*/

	std::string name;	// the name of the variable / function
	Type type;	// the variable type (for functions, the return type)
	Type sub_type;	// the subtype -- e.g., on "ptr<int>", the type is "ptr" and the subtype is "int"

	std::string scope_name;	// the name of the scope -- either "global" or the name of the function
	size_t scope_level;	// the /level/ of scope within the program; if we are in a loop or ite block, the level will increase

	bool defined;	// tracks whether the variable has been defined; we cannot use it before it is defined
	bool freed;	// tracks whether the variable has been freed; this is used for dynamic memory when we want to do garbage collection
	SymbolQuality quality;	// tells us whether something is const, etc.

	size_t stack_offset;	// used for local symbols to determine the offset (in words) from the initial address of the SP

	// TODO: change the way formal parameters are handled? could iterate through that symbol scope to look for variables instead...
	std::vector<std::shared_ptr<Statement>> formal_parameters;	// used only for function symbols

	// constructor/destructor
	Symbol(std::string name, Type type, std::string scope_name, size_t scope_level, Type sub_type = NONE, SymbolQuality quality = NO_QUALITY, bool defined = false, std::vector<std::shared_ptr<Statement>> formal_parameters = {});
	Symbol();
	~Symbol();
};
