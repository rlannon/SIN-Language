// Expression.cpp
// Implementation of the Expression class

#include "Expression.h"


const exp_operator translate_operator(std::string op_string) {
	const size_t num_operators = 16;

	// our operator strings and list;
	std::string string_operators_list[num_operators] = { "+", "-", "*", "/", "=", "!=", ">", "<", ">=", "<=", "&", "!", "|", "%", "and", "or" };
	exp_operator operators_list[num_operators] = { PLUS, MINUS, MULT, DIV, EQUAL, NOT_EQUAL, GREATER, LESS, GREATER_OR_EQUAL, LESS_OR_EQUAL, BIT_AND, NOT, BIT_OR, MODULO, AND, OR };

	size_t i = 0;
	bool found = false;
	while (i < num_operators && !found) {
		if (op_string == string_operators_list[i]) {
			found = true;
		}
		else {
			i += 1;
		}
	}

	// return the operator; if we didn't find the one we wanted, return NO_OP
	if (found) {
		return operators_list[i];
	}
	else {
		return NO_OP;
	}
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
	std::string string_types[] = { "int", "float", "string", "bool", "void", "ptr", "raw" };
	Type _types[] = { INT, FLOAT, STRING, BOOL, VOID, PTR, RAW };

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
	std::string string_types[] = { "int", "float", "string", "bool", "void", "ptr", "raw" };
	Type _types[] = { INT, FLOAT, STRING, BOOL, VOID, PTR, RAW };

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


// Lists

std::vector<std::shared_ptr<Expression>> ListExpression::get_list()
{
	return this->list_members;
}

ListExpression::ListExpression(std::vector<std::shared_ptr<Expression>> list_members) : list_members(list_members)
{
	this->expression_type = LIST;
}

ListExpression::ListExpression() {
	this->expression_type = LIST;
	this->list_members = {};
}

ListExpression::~ListExpression() {

}


// Pointers

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



// sizeof expressions

std::string SizeOf::get_type() {
	return this->to_check;
}

SizeOf::SizeOf(std::string to_check) : to_check(to_check) {
	this->expression_type = SIZE_OF;
}

SizeOf::SizeOf() {
	this->expression_type = SIZE_OF;
}

std::shared_ptr<Expression> Indexed::get_index_value()
{
	return this->index_value;
}

Indexed::Indexed(std::string value, std::string LValue_type, std::shared_ptr<Expression> index_init) : index_value(index_init)
{
	this->value = value;
	this->LValue_Type = LValue_type;
	this->expression_type = INDEXED;
}

Indexed::Indexed()
{
	this->expression_type = INDEXED;
}
