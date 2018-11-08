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

IfThenElse::IfThenElse() {
	IfThenElse::statement_type = "ITE";
}



/*******************	FUNCTION CALL CLASS		********************/

Call::Call() {
}
