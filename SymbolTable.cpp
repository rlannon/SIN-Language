#include "SymbolTable.h"

// Our Symbol object

Symbol::Symbol(std::string name, Type type, std::string scope_name, int scope_level, Type sub_type, SymbolQuality quality, bool defined, std::vector<std::shared_ptr<Statement>> formal_parameters) : name(name), type(type), scope_name(scope_name), scope_level(scope_level), sub_type(sub_type), quality(quality), defined(defined), formal_parameters(formal_parameters) {
	this->stack_offset = 0;
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
}

Symbol::~Symbol() {

}

// Our SymbolTable object

void SymbolTable::insert(std::string name, Type type, std::string scope_name, int scope_level, Type sub_type, SymbolQuality quality, bool initialized, std::vector<std::shared_ptr<Statement>> formal_parameters)
{	
	if (this-> is_in_symbol_table(name, scope_name)) {
		throw std::runtime_error(("**** Symbol Table Error: '" + name + "'already in symbol table.").c_str());
	}
	else {
		this->symbols.push_back(Symbol(name, type, scope_name, scope_level, sub_type, quality, initialized, formal_parameters));	// an allocation is NOT a definition
	}
}

void SymbolTable::insert(Symbol to_add) {
	if (this->is_in_symbol_table(to_add.name, to_add.scope_name)) {
		throw std::runtime_error("**** Symbol Table Error: '" + to_add.name + "'already in symbol table.");
	}
	else {
		this->symbols.push_back(to_add);	// an allocation is NOT a definition
	}
}

void SymbolTable::define(std::string symbol_name, std::string scope_name)
{
	if (is_in_symbol_table(symbol_name, scope_name)) {

	} else {
		throw std::runtime_error(("**** Symbol Table Error: cannot find allocation for " + symbol_name).c_str());
	}
}



void SymbolTable::remove(std::string symbol_name, std::string scope_name, int scope_level) {
	/*
	
	Intended for use in local scopes, specifically ITE and While loops to remove any symbols that were declared within. This way, they cannot be accessed in scopes of the same level (or higher) that are not within that block.

	Iterates through the symbol table, checking for a variable matching the symbol name within the scope and level specified, and removes it if it finds one.

	*/

	std::vector<Symbol>::iterator symbol_iter = this->symbols.begin();
	
	while (symbol_iter != this->symbols.end()) {
		if ((symbol_iter->name == symbol_name) && (symbol_iter->scope_name == scope_name) && (symbol_iter->scope_level == scope_level)) {
			// remove the symbol, but do not increment the symbol_iter; it now will point to the element after the one we just erased
			this->symbols.erase(symbol_iter);
		}
		else {
			symbol_iter++;
		}
	}
}



Symbol* SymbolTable::lookup(std::string symbol_name, std::string scope_name)
{
	// iterate through our vector
	bool found = false;
	std::vector<Symbol>::iterator iter = this->symbols.begin();

	while ((iter != this->symbols.end()) && !found) {
		// if our name is in the symbol table
		if (symbol_name == iter->name) {
			// if we need to look at the scope
			if (scope_name != "") {
				// ensure the scope name also matches
				if (scope_name == iter->scope_name) {
					found = true;	// we have found the symbol
				}
				else {
					iter++;	// increment the iterator
				}
			}
			// if we are just checking for any match
			else {
				found = true;	// set our 'found' flag to true
			}
		}
		else {
			iter++;	// increment the iterator
		}
	}

	if (found) {
		// return the address of the element pointed to by the iterator
		return &*iter;
	}
	else {
		throw std::runtime_error("Could not find '" + symbol_name + "' in symbol table! (scope was '" + scope_name + "')");
	}
}

bool SymbolTable::is_in_symbol_table(std::string symbol_name, std::string scope_name)
{
	// iterate through our vector
	bool found = false;
	std::vector<Symbol>::iterator iter = this->symbols.begin();

	while ((iter != this->symbols.end()) && !found) {
		// if we have an entry in the same scope of the same name
		if ((symbol_name == iter->name) && (scope_name == iter->scope_name)) {
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
