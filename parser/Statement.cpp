/*

SIN Toolchain
Statement.cpp
Copyright 2019 Riley Lannon

The implementation of the Statement parent class and its various child classes

*/


#include "Statement.h"


stmt_type Statement::get_statement_type() {
	return Statement::statement_type;
}

unsigned int Statement::get_line_number()
{
	return this->line_number;
}

void Statement::set_line_number(unsigned int line_number)
{
	this->line_number = line_number;
}

Statement::Statement() {
	// create default scope names and levels
	// TODO: remove scope name and level in symbol?
	this->scope_level = 0;
	this->scope_name = "global";
}

Statement::Statement(stmt_type statement_type, unsigned int line_number) : statement_type(statement_type), line_number(line_number) {
	
}

Statement::~Statement() {
}



/*******************	STATEMENT BLOCK CLASS	********************/


StatementBlock::StatementBlock() {
}

StatementBlock::~StatementBlock() {
}



/*******************		INCLUDE CLASS		********************/


std::string Include::get_filename() {
	return this->filename;
}

Include::Include(std::string filename) : filename(filename) {
	this->statement_type = INCLUDE;
}

Include::Include() {
	this->statement_type = INCLUDE;
}

/*******************		DECLARATION CLASS		********************/


Type Declaration::get_data_type() {
	return this->data_type;
}

Type Declaration::get_subtype() {
	return this->subtype;
}

size_t Declaration::get_length() {
	return this->array_length;
}

std::vector<SymbolQuality> Declaration::get_qualities() {
	return this->qualities;
}

std::vector<std::shared_ptr<Statement>> Declaration::get_formal_parameters() {
	return this->formal_parameters;
}

// Constructors
Declaration::Declaration(Type data_type, std::string var_name, Type subtype, size_t array_length, std::vector<SymbolQuality> qualities, std::vector<std::shared_ptr<Statement>> formal_parameters) : data_type(data_type), var_name(var_name), subtype(subtype), array_length(array_length), qualities(qualities), formal_parameters(formal_parameters)
{
	this->statement_type = DECLARATION;
}

Declaration::Declaration() {
	this->statement_type = DECLARATION;
}

/*******************	ALLOCATION CLASS	********************/


Type Allocation::get_var_type() {
	return this->type;
}

Type Allocation::get_var_subtype() {
	return this->subtype;
}

std::string Allocation::get_var_type_as_string(Type to_convert) {
	std::string types_list[4] = { "int", "float", "string", "bool" };
	Type _types[4] = { INT, FLOAT, STRING, BOOL };

	for (int i = 0; i < 4; i++) {
		if (to_convert == _types[i]) {
			return types_list[i];
		}
		else {
			continue;
		}
	}

	// if we get here, the type was not in the list
	return "[unknown type]";
}

std::string Allocation::get_var_name() {
	return this->value;
}

size_t Allocation::get_array_length()
{
	return this->array_length;
}

std::vector<SymbolQuality> Allocation::get_qualities()
{
	return this->qualities;
}

bool Allocation::was_initialized()
{
	return this->initialized;
}

std::shared_ptr<Expression> Allocation::get_initial_value()
{
	return this->initial_value;
}

void Allocation::add_symbol_quality(SymbolQuality new_quality)
{
	if (this->qualities[0] == NO_QUALITY) {
		this->qualities.erase(this->qualities.begin(), this->qualities.begin() + 1);
	}

	if (new_quality == NO_QUALITY) {
		this->qualities.push_back(new_quality);
	}
	else {
		throw std::runtime_error("**** Error in Statement.cpp : Cannot add 'NO_QUALITY' to the symbol");
	}
}

void Allocation::set_symbol_qualities(std::vector<SymbolQuality> qualities)
{
	this->qualities = qualities;
}

Allocation::Allocation(Type type, std::string value, Type subtype, bool initialized, std::shared_ptr<Expression> initial_value, std::vector<SymbolQuality> quality, size_t length) : type(type), value(value), subtype(subtype), initialized(initialized), initial_value(initial_value), qualities(quality), array_length(length) {
	Allocation::statement_type = ALLOCATION;
}

Allocation::Allocation() {
	Allocation::type = NONE;
	Allocation::subtype = NONE;	// will remain 'NONE' unless 'type' is a ptr or array
	Allocation::initialized = false;
	Allocation::statement_type = ALLOCATION;
	Allocation::qualities = { NO_QUALITY };
	Allocation::array_length = 0;
}



/*******************	ASSIGNMENT CLASS	********************/

std::shared_ptr<Expression> Assignment::get_lvalue() {
	return this->lvalue;
}

std::shared_ptr<Expression> Assignment::get_rvalue() {
	return this->rvalue_ptr;
}

Assignment::Assignment(std::shared_ptr<Expression> lvalue, std::shared_ptr<Expression> rvalue) : lvalue(lvalue), rvalue_ptr(rvalue) {
	Assignment::statement_type = ASSIGNMENT;
}

Assignment::Assignment(LValue lvalue, std::shared_ptr<Expression> rvalue) : rvalue_ptr(rvalue) {
	this->lvalue = std::make_shared<LValue>(lvalue);
	this->statement_type = ASSIGNMENT;
}

Assignment::Assignment() {
	Assignment::statement_type = ASSIGNMENT;
}



/*******************	RETURN STATEMENT CLASS		********************/


std::shared_ptr<Expression> ReturnStatement::get_return_exp() {
	return this->return_exp;
}


ReturnStatement::ReturnStatement(std::shared_ptr<Expression> exp_ptr) {
	ReturnStatement::statement_type = RETURN_STATEMENT;
	ReturnStatement::return_exp = exp_ptr;
}

ReturnStatement::ReturnStatement() {
	ReturnStatement::statement_type = RETURN_STATEMENT;
}



/*******************	ITE CLASS		********************/

std::shared_ptr<Expression> IfThenElse::get_condition() {
	return this->condition;
}

std::shared_ptr<StatementBlock> IfThenElse::get_if_branch() {
	return this->if_branch;
}

std::shared_ptr<StatementBlock> IfThenElse::get_else_branch() {
	return this->else_branch;
}

IfThenElse::IfThenElse(std::shared_ptr<Expression> condition_ptr, std::shared_ptr<StatementBlock> if_branch_ptr, std::shared_ptr<StatementBlock> else_branch_ptr) {
	IfThenElse::statement_type = IF_THEN_ELSE;
	IfThenElse::condition = condition_ptr;
	IfThenElse::if_branch = if_branch_ptr;
	IfThenElse::else_branch = else_branch_ptr;
}

IfThenElse::IfThenElse(std::shared_ptr<Expression> condition_ptr, std::shared_ptr<StatementBlock> if_branch_ptr) {
	IfThenElse::statement_type = IF_THEN_ELSE;
	IfThenElse::condition = condition_ptr;
	IfThenElse::if_branch = if_branch_ptr;
	IfThenElse::else_branch = NULL;
}

IfThenElse::IfThenElse() {
	IfThenElse::statement_type = IF_THEN_ELSE;
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

WhileLoop::WhileLoop(std::shared_ptr<Expression> condition, std::shared_ptr<StatementBlock> branch) : condition(condition), branch(branch) {
	WhileLoop::statement_type = WHILE_LOOP;
}

WhileLoop::WhileLoop() {
}

/*******************	FUNCTION DEFINITION CLASS		********************/

std::shared_ptr<Expression> Definition::get_name() {
	return this->name;
}

Type Definition::get_return_type()
{
	return this->return_type;
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
	Definition::statement_type = DEFINITION;
}

Definition::Definition() {
	Definition::statement_type = DEFINITION;
}


/*******************	FUNCTION CALL CLASS		********************/

std::string Call::get_func_name() {
	return this->func->getValue();
}

size_t Call::get_args_size() {
	return this->args.size();
}

std::shared_ptr<Expression> Call::get_arg(size_t num) {
	return this->args[num];
}

Call::Call(std::shared_ptr<LValue> func, std::vector<std::shared_ptr<Expression>> args) : func(func), args(args) {
	Call::statement_type = CALL;
}

Call::Call() {
	Call::statement_type = CALL;
}


/*******************		INLINE ASM CLASS		********************/

std::string InlineAssembly::get_asm_type()
{
	return this->asm_type;
}

InlineAssembly::InlineAssembly(std::string assembly_type, std::string asm_code) : asm_type(assembly_type), asm_code(asm_code) {
	InlineAssembly::statement_type = INLINE_ASM;
}

InlineAssembly::InlineAssembly() {
	InlineAssembly::statement_type = INLINE_ASM;
}


/*******************		FREE MEMORY CLASS		********************/

LValue FreeMemory::get_freed_memory() {
	return this->to_free;
}

FreeMemory::FreeMemory(LValue to_free) : to_free(to_free) {
	FreeMemory::statement_type = FREE_MEMORY;
}

FreeMemory::FreeMemory() {
	FreeMemory::statement_type = FREE_MEMORY;
}
