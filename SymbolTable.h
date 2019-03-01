/*

SIN Toolchain
SymbolTable.h
Copyright 2019 Riley Lannon

A class to manage the symbol table for the compiler. Contains a vector of symbols as well as functions to search through the table.
The Compiler is a friend of this class; it is the only class which should be able to access anything about the symbols.

*/


#pragma once

#include "Symbol.h"
#include "Exceptions.h"

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


class SymbolTable
{
	friend class Compiler;	// allow the compiler to access these members

	std::vector<Symbol> symbols;

	void insert(std::string name, Type type, std::string scope_name, size_t scope_level, Type sub_type = NONE, SymbolQuality quality = NO_QUALITY, bool intialized = false, std::vector<std::shared_ptr<Statement>> formal_parameters = {});
	void insert(Symbol to_add);
	void define(std::string symbol_name, std::string scope_name);	// list the symbol of a given name in a given scope as defined

	void remove(std::string symbol_name, std::string scope_name, size_t scope_level);	// removes a symbol from the table; used to remove symbols from the table in ITE branches and loops

	Symbol* lookup(std::string symbol_name, std::string scope_name="");	// look for the symbol in the supplied scope; if none is supplied, look through whole table
	bool is_in_symbol_table(std::string symbol_name, std::string scope_name);
public:
	SymbolTable();
	~SymbolTable();
};

