// Expression.cpp
// Implementation of the Expression class

#include "Expression.h"


const exp_operator translate_operator(std::string op_string) {
	std::string string_operators_list[13] = { "+", "-", "*", "/", "=", "!=", ">", "<", ">=", "<=", "&", "!", "|" };
	exp_operator operators_list[13] = { PLUS, MINUS, MULT, DIV, EQUAL, NOT_EQUAL, GREATER, LESS, GREATER_OR_EQUAL, LESS_OR_EQUAL, AND, NOT, OR };

	for (int i = 0; i < 13; i++) {
		if (op_string == string_operators_list[i]) {
			return operators_list[i];
		}
		else {
			continue;
		}
	}
	// if we arrive here, we have not found a match; therefore, we must return NO_OP
	return NO_OP;
}

const bool is_literal(std::string candidate_type) {
	if (candidate_type == "int" || candidate_type == "float" || candidate_type == "bool" || candidate_type == "string") {
		return true;
	}
	else {
		return false;
	}
}

const Type get_type(std::string candidate) {
	// if it can, this function gets the proper type of an input string
	// an array of the valid types as strings
	std::string string_types[] = { "int", "float", "string", "bool" };
	Type _types[] = { INT, FLOAT, STRING, BOOL };

	// for test our candidate against each item in the array of string_types; if we have a match, return the Type at the same position
	for (int i = 0; i < 4; i++) {
		if (candidate == string_types[i]) {
			// if we have a match, return it
			return _types[i];
		}
		else {
			continue;
		}
	}

	// if we arrive here, we have not found the type we were looking for
	return NONE;
}

std::string Expression::getExpType() {
	return this->type;
}

Expression::Expression(std::string type) {
	Expression::type = type;
}

Expression::Expression() {
}

Expression::~Expression() {
}



Type Literal::get_type() {
	return this->data_type;
}

std::string Literal::get_value() {
	return this->value;
}

Literal::Literal(Type data_type, std::string value) {
	Literal::type = "literal";
	Literal::data_type = data_type;
	Literal::value = value;
}

Literal::Literal() {
	Literal::type = "literal";
}


std::string LValue::getValue() {
	return this->value;
}

void LValue::setValue(std::string new_value) {
	this->value = new_value;
}

LValue::LValue(std::string value, std::string LValue_Type) {
	LValue::value = value;
	LValue::type = "LValue";
	LValue::LValue_Type = LValue_Type;
}

LValue::LValue(std::string value) {
	LValue::value = value;
	LValue::type = "LValue";
	LValue::LValue_Type = "var";
}

LValue::LValue() {
	LValue::type = "LValue";
	LValue::value = "";
	LValue::LValue_Type = "var";
}



Binary::Binary(std::shared_ptr<Expression> left_exp, std::shared_ptr<Expression> right_exp, exp_operator op) {
	Binary::left_exp = left_exp;
	Binary::right_exp = right_exp;
	Binary::op = op;
	Binary::type = "binary";
}

Binary::Binary() {
	Binary::type = "binary";
}



exp_operator Unary::get_operator() {
	return this->op;
}

std::shared_ptr<Expression> Unary::get_operand() {
	return this->operand;
}

Unary::Unary(std::shared_ptr<Expression> operand, exp_operator op) {
	Unary::type = "unary";
	Unary::operand = operand;
	Unary::op = op;
}

Unary::Unary() {
	Unary::type = "unary";
}



// Parsing function calls

ValueReturningFunctionCall::ValueReturningFunctionCall(std::shared_ptr<Expression> name, std::vector<std::shared_ptr<Expression>> args) {
	ValueReturningFunctionCall::name = name;
	ValueReturningFunctionCall::args = args;
}

ValueReturningFunctionCall::ValueReturningFunctionCall() {
}
