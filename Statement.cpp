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


std::string Allocation::getVarType() {
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

Assignment::Assignment(LValue lvalue, std::shared_ptr<Expression> rvalue) {
	Assignment::statement_type = "assign";
	Assignment::lvalue = lvalue;
	Assignment::rvalue_ptr = rvalue;
}

Assignment::Assignment() {
	Assignment::statement_type = "assign";
}



/*******************	ITE CLASS		********************/

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



/*******************	FUNCTION CALL CLASS		********************/

Definition::Definition(std::shared_ptr<Expression> name_ptr, std::vector<std::shared_ptr<Statement>> args_ptr, std::shared_ptr<StatementBlock> procedure_ptr) {
	Definition::name = name_ptr;
	Definition::args = args_ptr;
	Definition::procedure = procedure_ptr;
	Definition::statement_type = "def";
}

Definition::Definition() {
	Definition::statement_type = "def";
}


/*******************	FUNCTION CALL CLASS		********************/

Call::Call(std::shared_ptr<Expression> func, std::vector<std::shared_ptr<Expression>> args) {
	Call::statement_type = "call";
	Call::func = func;
	Call::args = args;
}

Call::Call() {
	Call::statement_type = "call";
}
