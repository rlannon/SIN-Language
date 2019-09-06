/*

SIN Toolchain
Statement.h
Copyright 2019 Riley Lannon

Contains the "Statement" class an its child classes. Such objects are generated by the Parser when creating the AST and used by the compiler to generate the appropriate assembly.

*/

#pragma once

#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include <sstream>

#include "Expression.h"
#include "../util/EnumeratedTypes.h"
#include "../util/DataType.h"


// Statement is the base class for all statements

class Statement
{
protected:
	stmt_type statement_type;	// tells us whether its an Allocation, and Assignment, an ITE...
	std::string scope_name;	// to track the scope name under which the statement is being executed
	unsigned int scope_level;	// to track the scope level
	unsigned int line_number;	// the line number on which the first token of the statement can be found in the file

	// TODO: add scope information to statements in Parser

public:
	stmt_type get_statement_type();

	unsigned int get_line_number();
	void set_line_number(unsigned int line_number);

	Statement();
	Statement(stmt_type statement_type, unsigned int line_number);
	virtual ~Statement();
};

class StatementBlock
{
public:
	std::vector<std::shared_ptr<Statement>> statements_list;
	bool has_return;	// for functions, a return statement is necessary; this will also help determine if all control paths have a return value

	StatementBlock();
	~StatementBlock();
};

class Include : public Statement
{
	std::string filename;
public:
	std::string get_filename();
	Include(std::string filename);
	Include();
};

class Declaration : public Statement
{
	/*
	
	When we want to add a symbol to our symbol table, but not include an implementation of that symbol, the 'decl' keyword is used; e.g.,
		decl int myInt;	<- declares an integer variable 'myInt'
		decl int length(alloc string to_get);	<- declares a function called 'length'
	
	This allows a program to add symbols to its table without the implementation of those symbols; they can be added to the executable file at link time.
	This is useful for compiled libraries, removing the requirement of compiling said library every time a project using it is compiled.
	
	*/

	DataType type;
	bool function_definition;	// whether it's the declaration of a function
	bool struct_definition;	// whether it's the declaration of a struct

	std::string var_name;

	std::shared_ptr<Expression> initial_value;
	std::vector<std::shared_ptr<Statement>> formal_parameters;
public:
	std::string get_var_name();

	DataType get_type_information();
	bool is_function();
	bool is_struct();

	std::shared_ptr<Expression> get_initial_value();
	std::vector<std::shared_ptr<Statement>> get_formal_parameters();

	Declaration(DataType type, std::string var_name, std::shared_ptr<Expression> initial_value = std::make_shared<Expression>(EXPRESSION_GENERAL), bool is_function = false, bool is_struct = false, std::vector<std::shared_ptr<Statement>> formal_parameters = {});
	Declaration();
};

class Allocation : public Statement
{
	/*
	
	For a statement like:
		alloc int myInt;
	we create an allocation statement like so:
		type			:	INT
		value			:	myInt
		initialized		:	false
		initial_value	:	(none) 
		length			:	0

	We can also use what is called "alloc-assign syntax" in SIN:
		alloc int myInt: 5;
	which will allocate the variable and make an initial assignment. In this case, the allocation looks like:
		type			:	INT
		value			:	myInt
		initialized		:	true
		initial_value	:	5

	This "alloc-assign" syntax is required for all const-qualified data types

	*/
	
	DataType type_information;
	std::string value;

	// If we have an alloc-define statement, we will need:
	bool initialized;	// whether the variable was defined upon allocation

	std::shared_ptr<LValue> struct_name;	// structs will require a name

	std::shared_ptr<Expression> initial_value;	// todo: use the parser to expand allocations with initial values into two statements
public:
	DataType get_type_information();
	static std::string get_var_type_as_string(Type to_convert);
	std::string get_var_name();

	bool was_initialized();
	std::shared_ptr<Expression> get_initial_value();

	Allocation(DataType type_information, std::string value, bool was_initialized = false, std::shared_ptr<Expression> initial_value = std::make_shared<Expression>());	// use default parameters to allow us to use alloc-define syntax, but we don't have to
	Allocation();
};

class Assignment : public Statement
{
	std::shared_ptr<Expression> lvalue;
	std::shared_ptr<Expression> rvalue_ptr;
public:
	// get the variables / expressions themselves
	std::shared_ptr<Expression> get_lvalue();
	std::shared_ptr<Expression> get_rvalue();

	Assignment(std::shared_ptr<Expression> lvalue, std::shared_ptr<Expression> rvalue);
	Assignment(LValue lvalue, std::shared_ptr<Expression> rvalue);
	Assignment();
};

class ReturnStatement : public Statement
{
	std::shared_ptr<Expression> return_exp;
public:
	std::shared_ptr<Expression> get_return_exp();

	ReturnStatement(std::shared_ptr<Expression> exp_ptr);
	ReturnStatement();
};

class IfThenElse : public Statement
{
	std::shared_ptr<Expression> condition;
	std::shared_ptr<StatementBlock> if_branch;
	std::shared_ptr<StatementBlock> else_branch;
public:
	std::shared_ptr<Expression> get_condition();
	std::shared_ptr<StatementBlock> get_if_branch();
	std::shared_ptr<StatementBlock> get_else_branch();

	IfThenElse(std::shared_ptr<Expression> condition_ptr, std::shared_ptr<StatementBlock> if_branch_ptr, std::shared_ptr<StatementBlock> else_branch_ptr);
	IfThenElse(std::shared_ptr<Expression> condition_ptr, std::shared_ptr<StatementBlock> if_branch_ptr);
	IfThenElse();
};

class WhileLoop : public Statement
{
	std::shared_ptr<Expression> condition;
	std::shared_ptr<StatementBlock> branch;
public:
	std::shared_ptr<Expression> get_condition();
	std::shared_ptr<StatementBlock> get_branch();

	WhileLoop(std::shared_ptr<Expression> condition, std::shared_ptr<StatementBlock> branch);
	WhileLoop();
};

class Definition : public Statement
{
	std::shared_ptr<Expression> name;	// todo: why are function names Expressions but names in allocations are strings?
	DataType return_type;
	std::vector<std::shared_ptr<Statement>> args;
	std::shared_ptr<StatementBlock> procedure;

	// TODO: add function qualities? currently, definitions just put "none" for the symbol's quality
public:
	std::shared_ptr<Expression> get_name();
	DataType get_return_type();
	std::shared_ptr<StatementBlock> get_procedure();
	std::vector<std::shared_ptr<Statement>> get_args();

	Definition(std::shared_ptr<Expression> name_ptr, DataType return_type, std::vector<std::shared_ptr<Statement>> args_ptr, std::shared_ptr<StatementBlock> procedure_ptr);
	Definition();
};

class Call : public Statement
{
	std::shared_ptr<LValue> func;	// the function name
	std::vector<std::shared_ptr<Expression>> args;	// arguments to the function
public:
	std::string get_func_name();
	size_t get_args_size();
	std::shared_ptr<Expression> get_arg(size_t num);

	Call(std::shared_ptr<LValue> func, std::vector<std::shared_ptr<Expression>> args);
	Call();
};

class InlineAssembly : public Statement
{
	std::string asm_type;
public:
	std::string get_asm_type();

	std::string asm_code;

	InlineAssembly(std::string asm_type, std::string asm_code);
	InlineAssembly();
};

class FreeMemory : public Statement
{
	LValue to_free;
public:
	LValue get_freed_memory();

	FreeMemory(LValue to_free);
	FreeMemory();
};
