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


class SymbolTable
{
	friend class Compiler;	// allow the compiler to access these members

	std::vector< std::tuple<std::string, Type, std::string, int, bool> > symbols;

	void insert(std::string name, Type type, std::string scope_name, int scope_level);
	void define(std::string symbol_name, std::string scope_name);

	std::tuple<std::string, Type, std::string, int, bool>* lookup(std::string symbol_name);
	bool is_in_symbol_table(std::string symbol_name, std::string scope_name);
public:
	SymbolTable();
	~SymbolTable();
};

