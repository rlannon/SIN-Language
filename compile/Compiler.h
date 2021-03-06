/*

SIN Toolchain
Compiler.h
Copyright 2019 Riley Lannon

This class defines the SIN Compiler; given an AST produced by the Parser, will produce a .sina file that can execute the given code in the SIN VM.
Since it is such a massive class, it should always be allocated on the heap.

todo: split compiler (code generator) into multiple classes

*/

#pragma once

#include <vector>
#include <string>
#include <sstream>

#include "../util/VMMemoryMap.h"	// define where the blocks of memory begin and end in our target VM
#include "../parser/Parser.h"
#include "SymbolTable.h"	// for our symbol table object
#include "../assemble/Assembler.h"	// so we can assemble our compiled files into .sinc files
#include "../util/Exceptions.h"	// so that we can use our custom exceptions
#include "../util/DataWidths.h"	// for maintainability and avoiding obfuscation, avoid hard coding data widths where possible
#include "../util/FloatingPoint.h"	// contains useful utility functions for converting data to floating-point representation
#include "../util/DataType.h"	// for DataType class, useful for storing Type and Subtype
#include "../util/SyscallConstants.h"
#include "../util/Signals.h"


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

	// todo: allocate symbol_table on the heap? it may be a large object
	SymbolTable symbol_table;	// create an object for our symbol table

	size_t stack_offset;	// track the offset from the stack frame's base address; this allows us to store local variables in the stack

	size_t current_scope;	// tells us what scope level we are currently in
	std::string current_scope_name;

	size_t strc_number;	// the next available number for a string constant
	size_t branch_number;	// the next available number for a branch ID

	/* 
	The following functions returns the type of the expression passed into it once fully evaluated
	Note unary and binary trees are not fully parsed, only the first left-hand operand is returned -- any errors in type will be found once the tree or unary value is actually evaluated
	*/
	DataType get_expression_data_type(std::shared_ptr<Expression> to_evaluate, unsigned int line_number = 0);
	bool is_signed(std::shared_ptr<Expression> to_evaluate, unsigned int line_number = 0);	// we may need to determine whether an expression is signed or not
	bool types_are_compatible(std::shared_ptr<Expression> left, std::shared_ptr<Expression> right, unsigned int line_number = 0);

	// Evaluate trees -- generate the assembly to represent that evaluation
	std::stringstream evaluate_binary_tree(Binary bin_exp, unsigned int line_number, size_t max_offset = 0, DataType left_type = NONE);
	std::stringstream evaluate_unary_tree(Unary unary_exp, unsigned int line_number, size_t max_offset = 0);

	std::vector<std::string>* object_file_names;
	void include_file(Include include_statement);	// add a file to the solution

	void handle_declaration(Declaration declaration_statement);		// adds the symbol from the Declaration to the symbol table

	std::stringstream fetch_value(std::shared_ptr<Expression> to_fetch, unsigned int line_number, size_t max_offset);	// produces asm code to put the result of the specified expression in A

	std::stringstream move_sp_to_target_address(size_t target_offset, bool preserve_registers = false);

	std::stringstream allocate(Allocation allocation_statement, size_t* max_offset = nullptr);	// handle an "alloc" statement
	std::stringstream alloc_global(Symbol* to_allocate, unsigned int line_number, size_t max_offset, std::shared_ptr<Expression> initial_value = nullptr);	// allocate a global variable
	std::stringstream alloc_local(Symbol* to_allocate, unsigned int line_number, size_t* max_offset, std::shared_ptr<Expression> initial_value = nullptr);	// allocate a local variable
	std::stringstream define_global_constant(Symbol* to_allocate, unsigned int line_number, size_t max_offset, std::shared_ptr<Expression> initial_value);

	std::stringstream define(Definition definition_statement);	// add a function definition (using a definition statement)
	std::stringstream call(Call call_statement, size_t max_offset = 0);

	std::stringstream assign(Assignment assignment_statement, size_t max_offset = 0);
	std::stringstream string_assignment(Symbol* target_symbol, std::shared_ptr<Expression> rvalue, unsigned int line_number = 0, size_t max_offset = 0);
	std::stringstream dynamic_assignment(Symbol* target_symbol, std::shared_ptr<Expression> rvalue, unsigned int line_number = 0, size_t max_offset = 0);
	std::stringstream pointer_assignment(Dereferenced lvalue, std::shared_ptr<Expression> rvalue, unsigned int line_number = 0, size_t max_offset = 0);

	std::stringstream ite(IfThenElse ite_statement, size_t max_offset = 0);
	std::stringstream while_loop(WhileLoop while_statement, size_t max_offset = 0);
	std::stringstream return_value(ReturnStatement return_statement, size_t previous_offset, unsigned int line_number = 0);

	std::stringstream compile_to_sinasm(StatementBlock AST, unsigned int local_scope_level, std::string local_scope_name = "global", size_t max_offset = 0, size_t stack_frame_base = 0);	// compiles SIN code and writes SINASM code to output_file; modifies the member's vector pointer to list the dependencies
public:
	void produce_sina_file(std::string sina_filename, bool include_builtins = true);	// opens a file and calls the actual compilation routine; in a separate function so that we can use recursion
	std::stringstream compile_to_stringstream(bool include_builtins = true);

	Compiler(std::istream& sin_file, uint8_t _wordsize, std::vector<std::string>* object_file_names, std::vector<std::string>* included_libraries, bool include_builtins = true);	// the compiler is initialized using a file, and it will lex and parse it; the parameter 'include_builtins' will default to 'true', but we will be able to supress it
	Compiler();
	~Compiler();
};
