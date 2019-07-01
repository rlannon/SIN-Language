#pragma once
#include <memory>
#include <vector>
#include <string>
#include <tuple>
#include <exception>

#include "../util/EnumeratedTypes.h"


const exp_operator translate_operator(std::string op_string);	// given the string name for an exp_operator, returns that exp_operator

const int num_types = 11;

const bool is_literal(std::string candidate_type);

const Type get_type_from_string(std::string candidate);

const std::string get_string_from_type(Type candidate);



// Base class for all expressions
class Expression
{
protected:
	exp_type expression_type;	// replace "string type" with "exp_type expression_type"
public:
	exp_type get_expression_type();	// tells us whether it's a literal, lvalue, binary...
	//Expression(std::string type);
	Expression(exp_type expression_type);
	Expression();

	virtual ~Expression();
};

// Derived classes

class Literal : public Expression
{
	Type data_type;
	Type subtype;
	std::string value;
public:
	Type get_type();
	std::string get_value();
	Literal(Type data_type, std::string value, Type subtype = NONE);
	Literal();
};

// LValue -- a variable
class LValue : public Expression
{
protected:
	std::string value;	// the name of the variable
	std::string LValue_Type;	// the type -- var, var_dereferenced, or var_address
public:
	std::string getValue();
	std::string getLValueType();

	void setValue(std::string new_value);
	void setLValueType(std::string new_lvalue_type);

	LValue(std::string value, std::string LValue_Type);
	LValue(std::string value);
	LValue();
};

class ListExpression : public Expression
{
	std::vector<std::shared_ptr<Expression>> list_members;
public:
	std::vector<std::shared_ptr<Expression>> get_list();

	ListExpression(std::vector<std::shared_ptr<Expression>> list_members);
	ListExpression();
	~ListExpression();
};

// Indexed expressions are a child of an LValue
class Indexed : public LValue
{
	std::shared_ptr<Expression> index_value;	// the index value is simply an expression
public:
	std::shared_ptr<Expression> get_index_value();

	Indexed(std::string value, std::string LValue_type, std::shared_ptr<Expression> index_init);
	Indexed();
};

// Address Of -- the address of a variable
class AddressOf : public Expression
{
	//LValue target;	// the variable whose information we want
	LValue target;
public:
	LValue get_target();	// return the target variable

	AddressOf(LValue target);
	AddressOf();
};

// Dereferenced -- the value of a dereferenced ptr
class Dereferenced : public Expression
{
	std::shared_ptr<Expression> ptr;
public:
	LValue get_ptr();
	//LValue getReferencedLValue();

	std::shared_ptr<Expression> get_ptr_shared();

	Dereferenced(std::shared_ptr<Expression> ptr);
	Dereferenced();
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


// sizeof expressions

class SizeOf : public Expression
{
	std::string to_check;	// because the sizeof expression could be a struct, the typename will be stored as a string
public:
	std::string get_type();

	SizeOf(std::string to_check);
	SizeOf();
};
