#pragma once
// Our standard library includes
#include <iostream>
#include <vector>
#include <tuple>
#include <memory>
#include <string>

// Our custom headers / classes
#include "Parser.h"
#include "Expression.h"
#include "Statement.h"


class Interpreter
{
	std::vector<std::tuple<Type, std::string, std::string>> vars_table; // a vector of tuples containing the type, name, and value
	std::vector<Definition> functions_table;

	static const bool to_bool(std::string val);
	static const std::string bool_string(bool val);

	std::string get_var_value(std::string var_name, std::vector<std::tuple<Type, std::string, std::string>>* vars_table);
	void set_var_value(std::string var_name, std::string new_value, std::vector<std::tuple<Type, std::string, std::string>>* vars_table);
public:
	// If there is an interpretation error, use this function to print it; identical to the Parser error function
	static const void error(std::string message, int code);

	// get our statements list
	std::vector<std::tuple<Type, std::string, std::string>>* get_vars_table();

	void allocate_var(Allocation allocation, std::vector<std::tuple<Type, std::string, std::string>>* vars_table);
	void define_function(Definition definition);

	Definition get_definition(std::string func_to_find);

	void execute_statements(StatementBlock prog, std::vector<std::tuple<Type, std::string, std::string>>* vars_table);
	void evaluate_void_function(Call func_to_evaluate);
	std::tuple<Type, std::string> evaluate_value_returning_function(ValueReturningFunctionCall func_to_evaluate);
	void evaluate_statement(Statement* statement, std::vector<std::tuple<Type, std::string, std::string>>* vars_table);

	void evaluate_assignment(Assignment assign, std::vector<std::tuple<Type, std::string, std::string>>* vars_table);
	std::tuple<Type, std::string> evaluate_expression(Expression* expr, std::vector<std::tuple<Type, std::string, std::string>>* vars_table);
	
	template<typename T> T eval_sum(T const& left, T const& right);

	Interpreter();
	~Interpreter();
};

class LocalScope : public Interpreter
{
public:
	void evaluate_statement(Statement* statement);
};

