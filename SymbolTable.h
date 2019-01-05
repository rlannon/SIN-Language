#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <exception>

#include "Expression.h"	// "Type" enum

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
	std::string name;
	Type type;
	std::string scope_name;
	int scope_level;
	bool defined;

	size_t stack_offset;	// used for local symbols to determine the offset (in words) from the initial address of the SP

	Symbol(std::string name, Type type, std::string scope_name, int scope_level);
	~Symbol();
};


class SymbolTable
{
	friend class Compiler;	// allow the compiler to access these members

	std::vector<Symbol> symbols;

	void insert(std::string name, Type type, std::string scope_name, int scope_level);
	void define(std::string symbol_name, std::string scope_name);	// list the symbol of a given name in a given scope as defined

	void remove(std::string symbol_name, std::string scope_name, int scope_level);	// removes a symbol from the table; used to remove symbols from the table in ITE branches and loops

	Symbol* lookup(std::string symbol_name, std::string scope_name="");	// look for the symbol in the supplied scope; if none is supplied, look through whole table
	bool is_in_symbol_table(std::string symbol_name, std::string scope_name);
public:
	SymbolTable();
	~SymbolTable();
};

