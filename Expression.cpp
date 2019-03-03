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

const Type get_type_from_string(std::string candidate) {
	// if it can, this function gets the proper type of an input string
	// an array of the valid types as strings
	std::string string_types[] = { "int", "float", "string", "bool", "void", "ptr", "raw8", "raw16", "raw32" };
	Type _types[] = { INT, FLOAT, STRING, BOOL, VOID, PTR, RAW8, RAW16, RAW32 };

	// for test our candidate against each item in the array of string_types; if we have a match, return the Type at the same position
	for (int i = 0; i < num_types; i++) {
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

const std::string get_string_from_type(Type candidate) {
	// reverse of the above function
	std::string string_types[] = { "int", "float", "string", "bool", "void", "ptr", "raw8", "raw16", "raw32" };
	Type _types[] = { INT, FLOAT, STRING, BOOL, VOID, PTR, RAW8, RAW16, RAW32 };

	// for test our candidate against each item in the array of string_types; if we have a match, return the string at the same position
	for (int i = 0; i < num_types; i++) {
		if (candidate == _types[i]) {
			// if we have a match, return it
			return string_types[i];
		}
		else {
			continue;
		}
	}

	// if we arrive here, we have not found the type we are looking for
	return "none (error occurred)";
}

const Type get_raw_type(int _size) {
	if (_size == 8) {
		return RAW8;
	}
	else if (_size == 16) {
		return RAW16;
	}
	else if (_size == 32) {
		return RAW32;
	}
	else {
		return NONE;
	}
}

const bool is_raw(Type _t) {
	return (_t == RAW8 || _t == RAW16 || _t == RAW32);
}

exp_type Expression::get_expression_type()
{
	return this->expression_type;
}


Expression::Expression(exp_type expression_type) : expression_type(expression_type) {
	// uses initializer list
}

Expression::Expression() {
	Expression::expression_type = EXPRESSION_GENERAL;	// give it a default value so we don't try to use an uninitialized variable
}

Expression::~Expression() {
}



Type Literal::get_type() {
	return this->data_type;
}

std::string Literal::get_value() {
	return this->value;
}

Literal::Literal(Type data_type, std::string value, Type subtype) : value(value), data_type(data_type), subtype(subtype) {
	Literal::expression_type = LITERAL;
}

Literal::Literal() {
	Literal::data_type = NONE;	// give it a default value so we don't try to access an uninitialized value
	Literal::subtype = NONE;	// will remain 'NONE' unless our data_type is ptr or array
	Literal::expression_type = LITERAL;
}


std::string LValue::getValue() {
	return this->value;
}

std::string LValue::getLValueType() {
	return this->LValue_Type;
}

void LValue::setValue(std::string new_value) {
	this->value = new_value;
}

void LValue::setLValueType(std::string new_lvalue_type) {
	this->LValue_Type = new_lvalue_type;
}

LValue::LValue(std::string value, std::string LValue_Type) : value(value) {
	LValue::expression_type = LVALUE;
	LValue::LValue_Type = LValue_Type;
}

LValue::LValue(std::string value) : value(value) {
	LValue::expression_type = LVALUE;
	LValue::LValue_Type = "var";
}

LValue::LValue() {
	LValue::expression_type = LVALUE;
	LValue::value = "";
	LValue::LValue_Type = "var";
}



LValue AddressOf::get_target() {
	return this->target;
}

AddressOf::AddressOf(LValue target) : target(target) {
	AddressOf::expression_type = ADDRESS_OF;
}

AddressOf::AddressOf() {
	AddressOf::expression_type = ADDRESS_OF;
}



LValue Dereferenced::get_ptr() {
	if (this->ptr->get_expression_type() == LVALUE) {
		LValue* lvalue = dynamic_cast<LValue*>(this->ptr.get());
		return *lvalue;
	}
	else {
		throw std::runtime_error("Cannot convert type");
	}
}

std::shared_ptr<Expression> Dereferenced::get_ptr_shared() {
	return this->ptr;
}

Dereferenced::Dereferenced(std::shared_ptr<Expression> ptr) : ptr(ptr) {
	this->expression_type = DEREFERENCED;
}

Dereferenced::Dereferenced() {
	Dereferenced::expression_type = DEREFERENCED;
}



std::shared_ptr<Expression> Binary::get_left() {
	return this->left_exp;
}

std::shared_ptr<Expression> Binary::get_right() {
	return this->right_exp;
}

exp_operator Binary::get_operator() {
	return this->op;
}

Binary::Binary(std::shared_ptr<Expression> left_exp, std::shared_ptr<Expression> right_exp, exp_operator op) : left_exp(left_exp), right_exp(right_exp), op(op) {
	Binary::expression_type = BINARY;
}

Binary::Binary() {
	Binary::op = NO_OP;	// initialized to no_op so we don't ever run into uninitialized variables
	Binary::expression_type = BINARY;
}



exp_operator Unary::get_operator() {
	return this->op;
}

std::shared_ptr<Expression> Unary::get_operand() {
	return this->operand;
}

Unary::Unary(std::shared_ptr<Expression> operand, exp_operator op) : operand(operand), op(op) {
	Unary::expression_type = UNARY;
}

Unary::Unary() {
	Unary::op = NO_OP;	// initialize to no_op so we don't ever try to access uninitialized data
	Unary::expression_type = UNARY;
}



// Parsing function calls

std::shared_ptr<LValue> ValueReturningFunctionCall::get_name() {
	return this->name;
}

std::string ValueReturningFunctionCall::get_func_name() {
	return this->name->getValue();
}

std::vector<std::shared_ptr<Expression>> ValueReturningFunctionCall::get_args() {
	return this->args;
}

std::shared_ptr<Expression> ValueReturningFunctionCall::get_arg(int i) {
	return this->args[i];
}

int ValueReturningFunctionCall::get_args_size() {
	return this->args.size();
}

ValueReturningFunctionCall::ValueReturningFunctionCall(std::shared_ptr<LValue> name, std::vector<std::shared_ptr<Expression>> args) : name(name), args(args) {
	ValueReturningFunctionCall::expression_type = VALUE_RETURNING_CALL;
}

ValueReturningFunctionCall::ValueReturningFunctionCall() {
	ValueReturningFunctionCall::expression_type = VALUE_RETURNING_CALL;
}
