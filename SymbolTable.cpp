#include "SymbolTable.h"



void SymbolTable::insert(std::string name, Type type, std::string scope_name, int scope_level)
{
	if (this->is_in_symbol_table(name, scope_name)) {
		throw std::exception(("**** Symbol Table Error: \"" + name + "\"already in symbol table.").c_str());
	}
	else {
		this->symbols.push_back(std::make_tuple(name, type, scope_name, scope_level, false));	// an allocation is NOT a definition
	}
}

void SymbolTable::define(std::string symbol_name, std::string scope_name)
{
	if (is_in_symbol_table(symbol_name, scope_name)) {

	} else {
		throw std::exception(("**** Symbol Table Error: cannot find allocation for " + symbol_name).c_str());
	}
}



std::tuple<std::string, Type, std::string, int, bool>* SymbolTable::lookup(std::string symbol_name)
{
	// iterate through our vector
	bool found = false;
	std::vector<std::tuple<std::string, Type, std::string, int, bool>>::iterator iter = this->symbols.begin();

	while ((iter != this->symbols.end()) && !found) {
		// if our name is in the symbol table
		if (symbol_name == std::get<0>(*iter)) {
			found = true;	// set our 'found' flag to true
		}
		else {
			iter++;	// increment the iterator
		}
	}

	// return the address of the element pointed to by the iterator
	return &*iter;
}

bool SymbolTable::is_in_symbol_table(std::string symbol_name, std::string scope_name)
{
	// iterate through our vector
	bool found = false;
	std::vector<std::tuple<std::string, Type, std::string, int, bool>>::iterator iter = this->symbols.begin();

	while ((iter != this->symbols.end()) && !found) {
		// if we have an entry in the same scope of the same name
		if ((symbol_name == std::get<0>(*iter)) && (scope_name == std::get<2>(*iter))) {
			found = true;	// set our 'found' flag to true
		} else {
			iter++;	// increment the iterator
		}
	}

	// return our 'found' flag
	return found;
}

SymbolTable::SymbolTable()
{
}


SymbolTable::~SymbolTable()
{
}
