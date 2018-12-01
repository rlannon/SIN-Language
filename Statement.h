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
	std::vector<std::shared_ptr<Statement>> StatementsList;

	StatementBlock();
	~StatementBlock();
};

class Allocation : public Statement
{
	Type type;
	std::string value;
public:
	Type getVarType();
	std::string getVarTypeAsString();
	std::string getVarName();
	Allocation(Type type, std::string value);
	Allocation();
};

class Assignment : public Statement
{
	LValue lvalue;
	std::shared_ptr<Expression> rvalue_ptr;
public:
	// get the variable names / expression types
	std::string getLvalueName();
	std::string getRValueType();

	// get the variables / expressions themselves
	LValue getLValue();
	std::shared_ptr<Expression> getRValue();

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
	std::shared_ptr<Expression> name;
	Type return_type;
	std::vector<std::shared_ptr<Statement>> args;
	std::shared_ptr<StatementBlock> procedure;
public:
	std::shared_ptr<Expression> get_name();
	Type get_return_type();
	std::shared_ptr<StatementBlock> get_procedure();
	std::vector<std::shared_ptr<Statement>> get_args();

	Definition(std::shared_ptr<Expression> name_ptr, Type return_type_ptr, std::vector<std::shared_ptr<Statement>> args_ptr, std::shared_ptr<StatementBlock> procedure_ptr);
	Definition();
};

class Call : public Statement
{
	std::shared_ptr<LValue> func;	// the function name
	std::vector<std::shared_ptr<Expression>> args;	// arguments to the function
public:
	std::string get_func_name();
	int get_args_size();
	std::shared_ptr<Expression> get_arg(int num);

	Call(std::shared_ptr<LValue> func, std::vector<std::shared_ptr<Expression>> args);
	Call();
};
