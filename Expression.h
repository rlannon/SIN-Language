#pragma once
#include <memory>
#include <vector>
#include <string>
#include <tuple>


// Define our custom types for expressions and statements

enum exp_operator {
	PLUS,
	MINUS,
	MULT,
	DIV,
	EQUAL,
	NOT_EQUAL,
	GREATER,
	LESS,
	GREATER_OR_EQUAL,
	LESS_OR_EQUAL,
	AND,
	NOT,
	OR,
	NO_OP
};

const exp_operator translate_operator(std::string op_string);

enum Type {
	INT,
	FLOAT,
	STRING,
	BOOL,
	VOID,
	NONE
};

const bool is_literal(std::string candidate_type);

const Type get_type_from_string(std::string candidate);

const std::string get_string_from_type(Type candidate);

// Base class for all expressions
class Expression
{
protected:
	std::string type;
public:
	std::string getExpType();
	Expression(std::string type);
	Expression();

	virtual ~Expression();
};

// Derived classes

class Literal : public Expression
{
	Type data_type;
	std::string value;
public:
	Type get_type();
	std::string get_value();
	Literal(Type data_type, std::string value);
	Literal();
};

// LValue -- a variable
class LValue : public Expression
{
	std::string value;
	std::string LValue_Type;
public:
	std::string getValue();
	void setValue(std::string new_value);

	LValue(std::string value, std::string LValue_Type);
	LValue(std::string value);
	LValue();
};

class Binary : public Expression
{
	exp_operator op;	// +, -, etc.
	std::shared_ptr<Expression> left_exp;
	std::shared_ptr<Expression> right_exp;
public:
	std::shared_ptr<Expression> get_left();
	std::shared_ptr<Expression> get_right();

	exp_operator get_operator();

	Binary(std::shared_ptr<Expression> left, std::shared_ptr<Expression> right, exp_operator op);
	Binary();
};

class Unary : public Expression
{
	exp_operator op;
	std::shared_ptr<Expression> operand;
public:
	exp_operator get_operator();
	std::shared_ptr<Expression> get_operand();

	Unary(std::shared_ptr<Expression> operand, exp_operator op);
	Unary();
};


// Functions are expressions if they return a value

class ValueReturningFunctionCall : public Expression
{
	std::shared_ptr<LValue> name;
	std::vector<std::shared_ptr<Expression>> args;
public:
	std::shared_ptr<LValue> get_name();
	std::string get_func_name();
	std::vector<std::shared_ptr<Expression>> get_args();
	std::shared_ptr<Expression> get_arg(int i);
	int get_args_size();

	ValueReturningFunctionCall(std::shared_ptr<LValue> name, std::vector<std::shared_ptr<Expression>> args);
	ValueReturningFunctionCall();
};
