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
	std::string string_types[] = { "int", "float", "string", "bool", "void", "intptr", "floatptr", "stringptr", "boolptr", "voidptr", "ptrptr", "raw8", "raw16", "raw32" };
	Type _types[] = { INT, FLOAT, STRING, BOOL, VOID, INTPTR, FLOATPTR, STRINGPTR, BOOLPTR, VOIDPTR, PTRPTR, RAW8, RAW16, RAW32 };

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
	std::string string_types[] = { "int", "float", "string", "bool", "void", "intptr", "floatptr", "stringptr", "boolptr", "voidptr", "ptrptr", "raw8", "raw16", "raw32" };
	Type _types[] = { INT, FLOAT, STRING, BOOL, VOID, INTPTR, FLOATPTR, STRINGPTR, BOOLPTR, VOIDPTR, PTRPTR, RAW8, RAW16, RAW32 };

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

const Type get_ptr_type(Type candidate) {
	if (candidate == INT) {
		return INTPTR;
	}
	else if (candidate == FLOAT) {
		return FLOATPTR;
	}
	else if (candidate == STRING) {
		return STRINGPTR;
	}
	else if (candidate == BOOL) {
		return BOOLPTR;
	}
	else if (candidate == VOID) {
		return VOIDPTR;
	}
	else if (candidate == INTPTR || candidate == FLOATPTR || candidate == STRINGPTR || candidate == BOOLPTR || candidate == VOIDPTR || candidate == PTRPTR) {
		return PTRPTR;
	}
}

const bool is_ptr_type(Type candidate) {
	return (candidate == INTPTR || candidate == FLOATPTR || candidate == STRINGPTR || candidate == BOOLPTR || candidate == VOIDPTR || candidate == PTRPTR);
}

const bool match_ptr_types(Type ptr_type, Type pointed_type) {
	// returns true if the types are compatible
	if (ptr_type == get_ptr_type(pointed_type)) {
		return true;
	}
	else {
		if (ptr_type == PTRPTR && is_ptr_type(pointed_type)) {
			return true;
		}
		else {
			return false;
		}
	}
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

std::string Expression::getExpType() {
	return this->type;
}

Expression::Expression(std::string type) : type(type) {
	// uses initializer list
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

Literal::Literal(Type data_type, std::string value) : data_type(data_type), value(value) {
	Literal::type = "literal";
}

Literal::Literal() {
	Literal::type = "literal";
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

LValue::LValue(std::string value, std::string LValue_Type) {
	LValue::value = value;
	LValue::type = "LValue";
	LValue::LValue_Type = LValue_Type;
}

LValue::LValue(std::string value) : value(value) {
	LValue::type = "LValue";
	LValue::LValue_Type = "var";
}

LValue::LValue() {
	LValue::type = "LValue";
	LValue::value = "";
	LValue::LValue_Type = "var";
}



LValue AddressOf::get_target() {
	return this->target;
}

AddressOf::AddressOf(LValue target) : target(target) {
	AddressOf::type = "address_of";
}

AddressOf::AddressOf() {
	AddressOf::type = "address_of";
}



LValue Dereferenced::get_ptr() {
	if (this->ptr->getExpType() == "LValue") {
		LValue* lvalue = dynamic_cast<LValue*>(this->ptr.get());
		return *lvalue;
	}
	else {
		std::string msg = "Cannot convert " + this->ptr->getExpType() + " to 'LValue'";
		throw std::exception(msg.c_str());
	}
}

//LValue Dereferenced::getReferencedLValue() {
//	if (this->getExpType() == "LValue") {
//		return this->get_ptr();
//	}
//	else if (this->getExpType() == "dereferenced") {
//		Dereferenced* next_ptr = dynamic_cast<Dereferenced*>(this->ptr.get());
//		return next_ptr->getReferencedLValue();
//	}
//}

std::shared_ptr<Expression> Dereferenced::get_ptr_shared() {
	return this->ptr;
}

Dereferenced::Dereferenced(std::shared_ptr<Expression> ptr) : ptr(ptr) {
	this->type = "dereferenced";
}

Dereferenced::Dereferenced() {
	Dereferenced::type = "dereferenced";
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

Unary::Unary(std::shared_ptr<Expression> operand, exp_operator op) : operand(operand), op(op) {
	Unary::type = "unary";
}

Unary::Unary() {
	Unary::type = "unary";
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
	ValueReturningFunctionCall::type = "value_returning";
}

ValueReturningFunctionCall::ValueReturningFunctionCall() {
	ValueReturningFunctionCall::type = "value_returning";
}
