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
	StatementBlock AST;	// the whole AST; used for loading the AST from the file and our initial compiler call
	size_t AST_index;
	std::shared_ptr<Statement> get_next_statement(StatementBlock AST);	// get the next statement in the AST
	std::shared_ptr<Statement> get_current_statement(StatementBlock AST);	// get the current statement in the AST (AST.statements_list[AST_index])
	
	// the assembly file we are writing to
	std::ofstream sina_file;
	// a stringstream so we can copy our subroutines/functions in the end of the file
	std::stringstream functions_ss;

	uint8_t _wordsize;	// our target wordsize

	SymbolTable symbol_table;	// create an object for our symbol table

	unsigned int current_scope;	// tells us what scope level we are currently in
	std::string current_scope_name;
	unsigned int next_available_addr;	// the next available address in the local page

	int _DATA_PTR;	// holds the next memory address to use for a variable in the _DATA section

	void produce_binary_tree(Binary bin_exp);	// writes a binary tree to the file
	void multiply(Binary mult_exp);	// write a multiplication statement
	void divide(Binary div_exp);	// write a division statement

	std::vector<std::string>* object_file_names;
	void include_file(Include include_statement);	// add a file to the solution

	std::stringstream allocate(Allocation allocation_statement, size_t* stack_offset = nullptr);	// add a variable to the symbol table (using an allocation statement)
	std::stringstream define(Definition definition_statement);	// add a function definition (using a definition statement)
	std::stringstream assign(Assignment assignment_statement, size_t* stack_offset = nullptr);
	std::stringstream call(Call call_statement);

	std::stringstream compile_to_sinasm(StatementBlock AST, unsigned int local_scope_level, std::string local_scope_name = "global", size_t* stack_offset = nullptr);	// compiles SIN code and writes SINASM code to output_file; modifies the member's vector pointer to list the dependencies
public:
	void produce_sina_file(std::string sina_filename);	// opens a file and calls the actual compilation routine; in a separate function so that we can use recursion
	std::stringstream compile_to_stringstream();

	Compiler(std::istream& sin_file, uint8_t _wordsize, std::vector<std::string>* object_file_names);	// the compiler is initialized using a file, and it will lex and parse it
	~Compiler();
};
