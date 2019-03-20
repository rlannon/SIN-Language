/*

SIN Toolchain
Compiler.h
Copyright 2019 Riley Lannon

This class defines the SIN Compiler; given an AST produced by the Parser, will produce a .sina file that can execute the given code in the SIN VM.

*/

#pragma once

#include <vector>
#include <list>
#include <string>
#include <tuple>
#include <sstream>

#include "VMMemoryMap.h"	// define where the blocks of memory begin and end in our target VM
#include "Parser.h"
#include "SymbolTable.h"	// for our symbol table object
#include "Assembler.h"	// so we can assemble our compiled files into .sinc files
#include "Exceptions.h"	// so that we can use our custom exceptions


class Compiler
{
	// our asm type
	const std::string asm_type = "sinasm16";

	// a list of library names that are being included; if one is already on the list and included in another file, it is skipped (otherwise we would get a linker error saying we have duplicate definitions)
	std::vector<std::string>* library_names;

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

	size_t stack_offset;

	size_t current_scope;	// tells us what scope level we are currently in
	std::string current_scope_name;

	size_t strc_number;	// the next available number for a string constant
	size_t branch_number;	// the next available number for a branch ID

	/* 
	The following functions returns the type of the expression passed into it once fully evaluated
	Note unary and binary trees are not fully parsed, only the first left-hand operand is returned -- any errors in type will be found once the tree or unary value is actually evaluated
	*/
	Type get_expression_data_type(std::shared_ptr<Expression> to_evaluate, bool get_subtype = false);
	bool is_signed(std::shared_ptr<Expression> to_evaluate, unsigned int line_number = 0);	// we may need to determine whether an expression is signed or not

	std::stringstream evaluate_binary_tree(Binary bin_exp, unsigned int line_number, size_t max_offset = 0, Type left_type = NONE);	// writes a binary tree to the file
	std::stringstream evaluate_unary_tree(Unary unary_exp, unsigned int line_number, size_t max_offset = 0);	// writes the evaluation of a unary value

	std::vector<std::string>* object_file_names;
	void include_file(Include include_statement);	// add a file to the solution

	std::stringstream fetch_value(std::shared_ptr<Expression> to_fetch, unsigned int line_number = 0, size_t max_offset = 0);	// produces asm code to put the result of the specified expression in A

	std::stringstream move_sp_to_target_address(size_t target_offset, bool preserve_registers = false);

	std::stringstream string_assignment(Symbol* target_symbol, std::shared_ptr<Expression> rvalue, unsigned int line_number = 0, size_t max_offset = 0);

	std::stringstream allocate(Allocation allocation_statement, size_t* max_offset = nullptr);	// add a variable to the symbol table (using an allocation statement)
	std::stringstream define(Definition definition_statement);	// add a function definition (using a definition statement)
	std::stringstream assign(Assignment assignment_statement, size_t max_offset = 0);
	std::stringstream call(Call call_statement, size_t max_offset = 0);
	std::stringstream ite(IfThenElse ite_statement, size_t max_offset = 0);
	std::stringstream while_loop(WhileLoop while_statement, size_t max_offset = 0);
	std::stringstream return_value(ReturnStatement return_statement, size_t previous_offset, unsigned int line_number = 0);

	std::stringstream compile_to_sinasm(StatementBlock AST, unsigned int local_scope_level, std::string local_scope_name = "global", size_t max_offset = 0, size_t stack_frame_base = 0);	// compiles SIN code and writes SINASM code to output_file; modifies the member's vector pointer to list the dependencies
public:
	void produce_sina_file(std::string sina_filename, bool include_builtins = true);	// opens a file and calls the actual compilation routine; in a separate function so that we can use recursion
	std::stringstream compile_to_stringstream(bool include_builtins = true);

	// TODO: add a compiler error class so we can automatically print out error messages with codes and line numbers

	Compiler(std::istream& sin_file, uint8_t _wordsize, std::vector<std::string>* object_file_names, std::vector<std::string>* included_libraries, bool include_builtins = true);	// the compiler is initialized using a file, and it will lex and parse it; the parameter 'include_builtins' will default to 'true', but we will be able to supress it
	Compiler();
	~Compiler();
};
