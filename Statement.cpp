// Statement.cpp
// Implementation of the "Statement" class

# include "Statement.h"


std::string Statement::get_type() {
	return Statement::statement_type;
}

Statement::Statement() {
}

Statement::~Statement() {
}



/*******************	STATEMENT BLOCK CLASS	********************/


StatementBlock::StatementBlock() {
}

StatementBlock::~StatementBlock() {
}



/*******************	ALLOCATION CLASS	********************/


Type Allocation::getVarType() {
	return this->type;
}

std::string Allocation::getVarTypeAsString() {
	std::string types_list[4] = { "int", "float", "string", "bool" };
	Type _types[4] = { INT, FLOAT, STRING, BOOL };

	for (int i = 0; i < 4; i++) {
		if (this->type == _types[i]) {
			return types_list[i];
		}
		else {
			continue;
		}
	}

	// if we get here, the type was not in the list
	return "[unknown type]";
}

std::string Allocation::getVarName() {
	return this->value;
}

Allocation::Allocation(Type type, std::string value) {
	Allocation::statement_type = "alloc";
	Allocation::type = type;
	Allocation::value = value;
}

Allocation::Allocation() {
	Allocation::statement_type = "alloc";
}



/*******************	ASSIGNMENT CLASS	********************/


std::string Assignment::getLvalueName() {
	return this->lvalue.getValue();
}

std::string Assignment::getRValueType() {
	Expression* _rval = dynamic_cast<Expression*>(rvalue_ptr.get());
	return _rval->getExpType();
}

std::shared_ptr<Expression> Assignment::getRValue() {
	return this->rvalue_ptr;
}

Assignment::Assignment(LValue lvalue, std::shared_ptr<Expression> rvalue) {
	Assignment::statement_type = "assign";
	Assignment::lvalue = lvalue;
	Assignment::rvalue_ptr = rvalue;
}

Assignment::Assignment() {
	Assignment::statement_type = "assign";
}



/*******************	RETURN STATEMENT CLASS		********************/


std::shared_ptr<Expression> ReturnStatement::get_return_exp() {
	return this->return_exp;
}


ReturnStatement::ReturnStatement(std::shared_ptr<Expression> exp_ptr) {
	ReturnStatement::statement_type = "return";
	ReturnStatement::return_exp = exp_ptr;
}

ReturnStatement::ReturnStatement() {
	ReturnStatement::statement_type = "return";
}



/*******************	ITE CLASS		********************/

std::shared_ptr<Expression> IfThenElse::get_condition() {
	return IfThenElse::condition;
}

std::shared_ptr<StatementBlock> IfThenElse::get_if_branch() {
	return this->if_branch;
}

std::shared_ptr<StatementBlock> IfThenElse::get_else_branch() {
	return this->else_branch;
}


IfThenElse::IfThenElse(std::shared_ptr<Expression> condition_ptr, std::shared_ptr<StatementBlock> if_branch_ptr, std::shared_ptr<StatementBlock> else_branch_ptr) {
	IfThenElse::statement_type = "ite";
	IfThenElse::condition = condition_ptr;
	IfThenElse::if_branch = if_branch_ptr;
	IfThenElse::else_branch = else_branch_ptr;
}

IfThenElse::IfThenElse(std::shared_ptr<Expression> condition_ptr, std::shared_ptr<StatementBlock> if_branch_ptr) {
	IfThenElse::statement_type = "ite";
	IfThenElse::condition = condition_ptr;
	IfThenElse::if_branch = if_branch_ptr;
	IfThenElse::else_branch = NULL;
}

IfThenElse::IfThenElse() {
	IfThenElse::statement_type = "ite";
}



/*******************	WHILE LOOP CLASS		********************/

std::shared_ptr<Expression> WhileLoop::get_condition()
{
	return WhileLoop::condition;
}

std::shared_ptr<StatementBlock> WhileLoop::get_branch()
{
	return WhileLoop::branch;
}

WhileLoop::WhileLoop(std::shared_ptr<Expression> condition, std::shared_ptr<StatementBlock> branch) {
	WhileLoop::condition = condition;
	WhileLoop::branch = branch;
	WhileLoop::statement_type = "while";
}

WhileLoop::WhileLoop() {
}

/*******************	FUNCTION DEFINITION CLASS		********************/

std::shared_ptr<Expression> Definition::get_name() {
	return this->name;
}

std::shared_ptr<StatementBlock> Definition::get_procedure() {
	return this->procedure;
}

std::vector<std::shared_ptr<Statement>> Definition::get_args() {
	return this->args;
}

Definition::Definition(std::shared_ptr<Expression> name_ptr, Type return_type_ptr, std::vector<std::shared_ptr<Statement>> args_ptr, std::shared_ptr<StatementBlock> procedure_ptr) {
	Definition::name = name_ptr;
	Definition::args = args_ptr;
	Definition::procedure = procedure_ptr;
	Definition::return_type = return_type_ptr;
	Definition::statement_type = "def";
}

Definition::Definition() {
	Definition::statement_type = "def";
}


/*******************	FUNCTION CALL CLASS		********************/

std::string Call::get_func_name() {
	return this->func->getValue();
}

int Call::get_args_size() {
	return this->args.size();
}

std::shared_ptr<Expression> Call::get_arg(int num) {
	return this->args[num];
}

Call::Call(std::shared_ptr<LValue> func, std::vector<std::shared_ptr<Expression>> args) {
	Call::statement_type = "call";
	Call::func = func;
	Call::args = args;
}

Call::Call() {
	Call::statement_type = "call";
}
