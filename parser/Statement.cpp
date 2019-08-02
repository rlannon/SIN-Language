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
	this->statements_list = {};
	this->has_return = false;
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


std::string Declaration::get_var_name() {
	return this->var_name;
}

DataType Declaration::get_type_information() {
	return this->type;
}

bool Declaration::is_function()
{
	return this->function_definition;
}

bool Declaration::is_struct()
{
	return this->struct_definition;
}

std::shared_ptr<Expression> Declaration::get_initial_value()
{
	return this->initial_value;
}

std::vector<std::shared_ptr<Statement>> Declaration::get_formal_parameters() {
	return this->formal_parameters;
}

// Constructors
Declaration::Declaration(DataType type, std::string var_name, std::shared_ptr<Expression> initial_value, bool is_function, bool is_struct, std::vector<std::shared_ptr<Statement>> formal_parameters) :
	type(type),
	var_name(var_name),
	initial_value(initial_value),
	function_definition(is_function),
	struct_definition(is_struct),
	formal_parameters(formal_parameters)
{
	this->statement_type = DECLARATION;
}

Declaration::Declaration() {
	this->statement_type = DECLARATION;
}

/*******************	ALLOCATION CLASS	********************/


DataType Allocation::get_type_information() {
	return this->type_information;
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

bool Allocation::was_initialized()
{
	return this->initialized;
}

std::shared_ptr<Expression> Allocation::get_initial_value()
{
	return this->initial_value;
}

Allocation::Allocation(DataType type_information, std::string value, bool initialized, std::shared_ptr<Expression> initial_value) :
	type_information(type_information),
	value(value),
	initialized(initialized),
	initial_value(initial_value)
{
	Allocation::statement_type = ALLOCATION;
}

Allocation::Allocation() {
	Allocation::type_information = type_information;
	Allocation::initialized = false;
	Allocation::statement_type = ALLOCATION;
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
	IfThenElse::else_branch = nullptr;
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

std::vector<SymbolQuality> Definition::get_qualities()
{
	return this->qualities;
}

Type Definition::get_return_type()
{
	return this->return_type;
}

Type Definition::get_return_subtype()
{
	return this->return_subtype;
}

std::shared_ptr<StatementBlock> Definition::get_procedure() {
	return this->procedure;
}

std::vector<std::shared_ptr<Statement>> Definition::get_args() {
	return this->args;
}

Definition::Definition(std::shared_ptr<Expression> name_ptr, Type return_type, Type return_subtype, std::vector<SymbolQuality> qualities, std::vector<std::shared_ptr<Statement>> args_ptr, std::shared_ptr<StatementBlock> procedure_ptr):
	name(name_ptr),
	return_type(return_type),
	return_subtype(return_subtype),
	qualities(qualities),
	args(args_ptr),
	procedure(procedure_ptr)
{
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
