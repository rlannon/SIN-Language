#pragma once
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "Expression.h"


// Statement is the base class for all statements

class Statement
{
protected:
	std::string statement_type;
public:
	std::string get_type();

	Statement();
	virtual ~Statement();
};

class StatementBlock
{
public:
	std::vector<std::unique_ptr<Statement>> StatementsList;

	StatementBlock();
	~StatementBlock();
};

class Allocation : public Statement
{
	Type type;
	std::string value;
public:
	std::string getVarType();
	std::string getVarName();
	Allocation(Type type, std::string value);
	Allocation();
};

class Assignment : public Statement
{
	LValue lvalue;
	std::shared_ptr<Expression> rvalue_ptr;
public:
	std::string getLvalueName();
	std::string getRValueType();
	Assignment(LValue lvalue, std::shared_ptr<Expression> rvalue);
	Assignment();
};

class IfThenElse : public Statement
{
	Expression condition;
	StatementBlock if_branch;
	StatementBlock else_branch;
public:
	IfThenElse();
};

class Call : public Statement
{
	std::shared_ptr<Expression> func;	// the function name
	std::vector<std::shared_ptr<Expression>> args;	// arguments to the function
public:
	Call();
};
