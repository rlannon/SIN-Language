#pragma once

// Our standard library includes
#include <iostream>
#include <vector>
#include <tuple>
#include <memory>
#include <string>
#include <cstdint>
#include <list>

// Our custom headers / classes
#include "parser/Parser.h"
#include "parser/Expression.h"
#include "parser/Statement.h"

// TODO: add line numbers to interpreter error messages

typedef struct InterpreterSymbol
{
	Type data_type;
	Type subtype;
	std::string name;
	std::string value;

	InterpreterSymbol(Type data_type, std::string name, std::string value, Type subtype = NONE);
	InterpreterSymbol();
};

class Interpreter
{
	// our symbol tables will be std::list<> because they can be safely pointed to
	std::list<InterpreterSymbol> var_table;	// variable's type, name, and value
	std::list<Definition> function_table;

	// convert to and from boolean type
	static const bool toBool(std::string val);
	static const std::string boolString(bool val);
	static const bool are_compatible_types(Type a, Type b);	// tells us if the types are compatible with one another

	// get a variable value as a string
	std::string getVarValue(LValue variable, std::list<InterpreterSymbol>* vars_table);
	// set a variable's value
	void setVarValue(LValue variable, std::tuple<Type, std::string> new_value, std::list<InterpreterSymbol>* vars_table);

	// allocate a variable
	void allocateVar(Allocation allocation, std::list<InterpreterSymbol>* vars_table);
	// define a function
	void defineFunction(Definition definition);

	// execute a single statement
	void executeStatement(Statement* statement, std::list<InterpreterSymbol>* vars_table);
	// evaluate an assignment statement
	void evaluateAssignment(Assignment assign, std::list<InterpreterSymbol>* vars_table);

	// execute a branch or block of statements
	void executeBranch(StatementBlock prog, std::list<InterpreterSymbol>* vars_table);

	// evaluate function calls
	void evaluateVoidFunction(Call func_to_evaluate, std::list<InterpreterSymbol>* parent_vars_table);
	std::tuple<Type, std::string> evaluateValueReturningFunction(ValueReturningFunctionCall func_to_evaluate, std::list<InterpreterSymbol>* parent_vars_table);

	// evaluate an expression
	std::tuple<Type, std::string> evaluateExpression(Expression* expr, std::list<InterpreterSymbol>* vars_table);
	std::tuple<Type, std::string> evaluateBinary(std::tuple<Type, std::string> left, std::tuple<Type, std::string> right, exp_operator op);

	// return a definition object for a function of a given name
	Definition getDefinition(std::string func_to_find);
	// return all information for a variable of a given name
	InterpreterSymbol getVar(std::string var_to_find, std::list<InterpreterSymbol>* vars_table);
public:
	// entry point to the interpreter; runs a branch of statements passed into it, starting at the global scope
	void interpretAST(StatementBlock AST);

	Interpreter();
	~Interpreter();
};



// Specific exceptions for the interpreter, a child class of the standard exception class
class InterpreterException : public std::exception {
protected:
	std::string message_;
	int code_;
public:
	explicit InterpreterException();
	explicit InterpreterException(const std::string& err_message, const int& err_code);
	virtual const char* what() const noexcept;
	int get_code();
};

class TypeMatchError : public InterpreterException {
	Type a_;
	Type b_;
public:
	virtual const char* what() const noexcept;
	explicit TypeMatchError(const Type& a, const Type& b);
};
