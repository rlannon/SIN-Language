#pragma once

#include <vector>
#include <list>
#include <string>
#include <tuple>
#include <sstream>

#include "VMMemoryMap.h"	// define where the blocks of memory begin and end in our target VM
//#include "Lexer.h"	// included in "Parser.h"
#include "Parser.h"
// #include "Expression.h"	// included in "Parser.h"
// #include "Statement.h"	// included in "Parser.h"
#include "SymbolTable.h"	// for our symbol table object
#include "Assembler.h"	// so we can assemble our compiled files into .sinc files

/*

The Compiler for the SIN language. Given an AST produced by the Parser, will produce a .sina file that can execute the given code in the SIN VM.

Symbol Table:
	The symbol table contains the symbol name, the type, and the scope; whenever the symbol is referenced in the program, 

*/

class Compiler
{
	// AST and functions for navigating it
	StatementBlock AST;
	size_t AST_index;	// tells us what Statement number we are on in our AST object
	std::shared_ptr<Statement> get_next_statement();	// get the next statement in the AST
	std::shared_ptr<Statement> get_current_statement();	// get the current statement in the AST (AST.statements_list[AST_index])
	
	uint8_t _wordsize;	// our target wordsize

	SymbolTable symbol_table;	// create an object for our symbol table

	int current_scope;	// tells us what scope level we are currently in

	int _DATA_PTR;	// holds the next memory address to use for a variable in the _DATA section

	std::vector<std::string>* object_file_names;
	void include_file(Include include_statement);	// add a file to the solution

	void allocate(Allocation allocation_statement);	// add a variable to the symbol table (using an allocation statement)
	void define(Definition definition_statement);	// add a function definition (using a definition statement)

	void compile_to_sinasm(std::ostream& output_file);	// compiels SIN code and writes SINASM code to output_file; modifies the member's vector pointer to list the dependencies
public:
	void compile(std::string sina_filename);	// opens a file and calls the actual compilation routine; in a separate function so that we can use recursion

	Compiler(std::istream& sin_file, uint8_t _wordsize, std::vector<std::string>* object_file_names);	// the compiler can also be initialized using a file, and it will lex and parse it
	~Compiler();
};

