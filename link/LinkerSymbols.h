/*

SIN Toolchain
LinkerSymbols.h
Copyright 2019 Riley Lannon

To make the code for the assembler and the linker cleaner/more readable/more maintainable, we will convert the symbol, relocation, and data tables used by the assembler and linker into enumerated types and structs

*/


#pragma once

#include <string>
#include <vector>
#include <cinttypes>


enum SymbolClass
{
	D,	// defined
	R,	// reserved
	C,	// constant
	M,	// macro
	U	// undefined (to be resolved by the linker)
};

struct AssemblerSymbol {
	/*
	
	A struct to contain symbols used in the Assembler. Keeps track of the symbol name, its value, its size (in bytes), and its class (Defined, Undefined, etc..).
	This struct is also used by the linker when it is resolving relocation addresses.

	*/
	
	std::string name;	// the symbol's name
	size_t value;	// the value associated with that symbol (e.g., for a label, the address to which that label points)
	size_t width;	// the size, in bytes, of the symbol (used with @rs directives)
	SymbolClass symbol_class;	// the symbol's class -- undefined, reserved, etc.

	AssemblerSymbol(std::string name, size_t address, size_t width, SymbolClass symbol_class);
	AssemblerSymbol();
	~AssemblerSymbol();
};

struct RelocationSymbol {
	/*
	
	A struct to keep track of elements in the assembler and linker relocation tables. These contain the symbol name and where it appears in the code so the linker can replace the data at the location with the appropriate data associated with the symbol.
	
	*/

	std::string name;	// the symbol's name
	size_t value;	// the address where it appears in the program

	RelocationSymbol(std::string name, size_t value);
	RelocationSymbol();
	~RelocationSymbol();
};

struct DataSymbol {
	/*
	
	A struct to keep track of elements in the assembler and linker data tables. This will handle symbols created by the @db assembler directive. These contain the symbol's name and any data associated with that symbol.
	
	*/

	std::string name;	// the symbol name
	std::vector<uint8_t> data;

	DataSymbol(std::string name, std::vector<uint8_t> data);
	~DataSymbol();
};
