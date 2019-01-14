#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <exception>

#include "Expression.h"	// "Type" enum
#include "Statement.h"

/*

The symbol table is structured like so:
	string symbol_name
	string symbol_type
		
	string scope_name
		gives a unique identifier to the scope name
	int scope_level
		gives an indentation level; two different scope names will have members inaccessible to one another, but they will use the same page in memory
	bool defined
		tells us whether the symbol has been defined, or merely allocated

*/


typedef struct Symbol
{
	/*
	
	A struct to contain our Symbol data; this contains the variable's name, type, scope level, whether it is defined, its stack offset (if a local variable), and, if it is a function, a vector of Statements containing that symbol's formal parameters
	
	*/
	
	std::string name;	// the name of the variable / function
	Type type;	// the variable type (for functions, the return type)

	std::string scope_name;	// the name of the scope -- either "global" or the name of the function
	int scope_level;	// the /level/ of scope within the program; if we are in a loop or ite block, the level will increase

	bool defined;	// tracks whether the variable has been defined; we cannot use it before it is defined
	std::string quality;	// tells us whether something is const, etc.

	size_t stack_offset;	// used for local symbols to determine the offset (in words) from the initial address of the SP

	// TODO: change the way formal parameters are handled? could iterate through that symbol scope to look for variables instead...
	std::vector<std::shared_ptr<Statement>> formal_parameters;	// used only for function symbols

	// constructor/destructor
	Symbol(std::string name, Type type, std::string scope_name, int scope_level, std::string quality = "none", bool defined = false, std::vector<std::shared_ptr<Statement>> formal_parameters = {});
	Symbol();
	~Symbol();
};


class SymbolTable
{
	friend class Compiler;	// allow the compiler to access these members

	std::vector<Symbol> symbols;

	void insert(std::string name, Type type, std::string scope_name, int scope_level, std::string quality = "none", bool intialized = false, std::vector<std::shared_ptr<Statement>> formal_parameters = {});
	void define(std::string symbol_name, std::string scope_name);	// list the symbol of a given name in a given scope as defined

	void remove(std::string symbol_name, std::string scope_name, int scope_level);	// removes a symbol from the table; used to remove symbols from the table in ITE branches and loops

	Symbol* lookup(std::string symbol_name, std::string scope_name="");	// look for the symbol in the supplied scope; if none is supplied, look through whole table
	bool is_in_symbol_table(std::string symbol_name, std::string scope_name);
public:
	SymbolTable();
	~SymbolTable();
};

