#include "Interpreter.h"


/************************************************************************
******************										*****************
******************			STATIC FUNCTIONS			*****************
******************										*****************
*************************************************************************/


const bool Interpreter::toBool(std::string val) {
	if (val == "true" || val == "True" || val == "TRUE") {
		return true;
	}
	else if (val == "false" || val == "False" || val == "FALSE") {
		return false;
	}
	else {
		throw InterpreterException("Error in bool value", 2130);
	}
}

const std::string Interpreter::boolString(bool val) {
	if (val == true) {
		return "True";
	}
	else {
		return "False";
	}
}

const bool Interpreter::areCompatibleTypes(Type a, Type b) {
	if (a == b) {
		return true;
	}
	else if (match_ptr_types(a, b)) {
		return true;
	}
	else if (is_raw(a) || is_raw(b)) {
		return true;
	}
	else {
		return false;
	}
}



/************************************************************************
******************										*****************
******************		PRIVATE MEMBER FUNCTIONS		*****************
******************										*****************
*************************************************************************/


std::tuple<Type, std::string, std::string> Interpreter::getVar(std::string var_to_find, std::list<std::tuple<Type, std::string, std::string>>* vars_table) {
	for (std::list<std::tuple<Type, std::string, std::string>>::iterator var_iter = vars_table->begin(); var_iter != vars_table->end(); var_iter++) {
		if (std::get<1>(*var_iter) == var_to_find) {
			return *var_iter;
		}
	}

	// if we can't find it, we will reach this
	std::string err_msg = "Could not find variable '" + var_to_find + "' in this scope!";
	throw InterpreterException(err_msg.c_str(), 000);
}


std::string Interpreter::getVarValue(LValue variable, std::list<std::tuple<Type, std::string, std::string>>* vars_table) {
	// find the variable in the symbol table
	try {
		std::tuple<Type, std::string, std::string> var_in_table = this->getVar(variable.getValue(), vars_table);

		// now, check the type
		if (variable.getLValueType() == "var" || variable.getLValueType() == "var_address") {
			return std::get<2>(var_in_table);
		}
		else if (variable.getLValueType() == "var_dereferenced") {
			// our address is in position 2 in the tuple at *var_iter
			// convert it into an actual pointer int
			intptr_t address = (intptr_t)std::stoi(std::get<2>(var_in_table));

			// now that we have the address, get the variable at that address
			std::tuple<Type, std::string, std::string>* dereferenced = (std::tuple<Type, std::string, std::string>*)address;

			// make sure the pointer and the value it is referecing are compatible types
			if (match_ptr_types(std::get<0>(var_in_table), std::get<0>(*dereferenced))) {
				// return the dereferenced value
				return std::get<2>(*dereferenced);
			}
			else {
				// if they are not compatible, throw an error
				throw InterpreterException("Variable referenced does not match type to which it points!", 000);
			}
		}
	}
	catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
}

void Interpreter::setVarValue(LValue variable, std::tuple<Type, std::string> new_value, std::list<std::tuple<Type, std::string, std::string>>* vars_table) {
	// find the variable in the symbol table
	for (std::list<std::tuple<Type, std::string, std::string>>::iterator var_iter = vars_table->begin(); var_iter != vars_table->end(); var_iter++) {
		if (std::get<1>(*var_iter) == variable.getValue()) {
			// now, check the type
			if (variable.getLValueType() == "var" || variable.getLValueType() == "var_address") {
				// check for compatible types
				if (this->areCompatibleTypes(std::get<0>(*var_iter), std::get<0>(new_value))) {
					// update the value
					
					// currently, RAW is not supported in Interpreted SIN
					if (is_raw(std::get<0>(*var_iter)) || is_raw(std::get<0>(new_value))) {
						throw InterpreterException("Interpreted SIN does not support the use of the RAW type", 1234);
					}
					std::get<2>(*var_iter) = std::get<1>(new_value);
					return;
				}
				else {
					throw TypeMatchError(std::get<0>(*var_iter), std::get<0>(new_value));
				}
			}
			else if (variable.getLValueType() == "var_dereferenced") {
				// get the value stored in the pointer, which is an address
				intptr_t address = (intptr_t)std::stoi(std::get<2>(*var_iter));
				// get the value at that address
				std::tuple<Type, std::string, std::string>* dereferenced = (std::tuple<Type, std::string, std::string>*)address;

				// continue updating the address as long as we have a pointer type
				while (is_ptr_type(std::get<0>(*dereferenced))) {
					// get the address stored in the current variable 'dereferenced'
					address = (intptr_t)std::stoi(std::get<2>(*dereferenced));
					// put the tuple at that address in "dereferenced"
					dereferenced = (std::tuple<Type, std::string, std::string>*)address;
				}

				// check for type compatibility
				if (this->areCompatibleTypes(std::get<0>(*dereferenced), std::get<0>(new_value))) {
					// update the value at that address
					std::get<2>(*dereferenced) = std::get<1>(new_value);
					return;
				}
				else {
					//std::string err_msg = "Types not compatible in assignment! (cannot match '" + get_string_from_type(std::get<0>(new_value)) + "' with '" + get_string_from_type(std::get<0>(*dereferenced)) + "')";
					//throw InterpreterException(err_msg.c_str(), 000);
					throw TypeMatchError(std::get<0>(new_value), std::get<0>(*dereferenced));
				}
			}
			else {
				throw InterpreterException("Unrecognized LValue type!", 000);
			}
		}
	}
	// if we arrive here, we never found the variable requested
	throw InterpreterException("Could not find '" + variable.getValue() + "' in symbol table", 000);
}


// allocate a variable
void Interpreter::allocateVar(Allocation allocation, std::list<std::tuple<Type, std::string, std::string>>* vars_table) {
	// Initialize the variable with its type, name, and no value / NULL value
	std::tuple<Type, std::string, std::string> var = std::make_tuple<Type, std::string, std::string>(allocation.getVarType(), allocation.getVarName(), "");

	// Push our newly-allocated variable to our variables vector
	vars_table->push_back(var);
}

// define a function
void Interpreter::defineFunction(Definition definition) {
	Interpreter::function_table.push_back(definition);
}

// execute a single statement
void Interpreter::executeStatement(Statement* statement, std::list<std::tuple<Type, std::string, std::string>>* vars_table) {
	if (statement == NULL) {
		throw InterpreterException("Expected a statement", 3411);
	}
	else {
		std::string stmt_type = statement->get_type();

		if (stmt_type == "alloc") {
			Allocation* alloc = dynamic_cast<Allocation*>(statement);
			this->allocateVar(*alloc, vars_table);
		}
		else if (stmt_type == "def") {
			Definition* def = dynamic_cast<Definition*>(statement);
			this->defineFunction(*def);
		}
		else if (stmt_type == "assign") {
			Assignment* assign = dynamic_cast<Assignment*>(statement);
			this->evaluateAssignment(*assign, vars_table);
		}
		else if (stmt_type == "ite") {
			IfThenElse* if_block = dynamic_cast<IfThenElse*>(statement);
			Expression* condition = dynamic_cast<Expression*>(if_block->get_condition().get());
			if (toBool(std::get<1>(this->evaluateExpression(condition, vars_table)))) {
				StatementBlock* branch = dynamic_cast<StatementBlock*>(if_block->get_if_branch().get());
				this->executeBranch(*branch, vars_table);
			}
			else {
				StatementBlock* branch = dynamic_cast<StatementBlock*>(if_block->get_else_branch().get());
				if (branch) {
					this->executeBranch(*branch, vars_table);
				}
			}
		}
		else if (stmt_type == "while") {
			// parse a while loop
			WhileLoop* while_loop = dynamic_cast<WhileLoop*>(statement);
			Expression* condition = dynamic_cast<Expression*>(while_loop->get_condition().get());
			StatementBlock* branch = dynamic_cast<StatementBlock*>(while_loop->get_branch().get());
			bool loop = toBool(std::get<1>(this->evaluateExpression(condition, vars_table)));
			while (loop) {
				this->executeBranch(*branch, vars_table);
				loop = toBool(std::get<1>(this->evaluateExpression(condition, vars_table)));
			}
		}
		else if (stmt_type == "return") {
			// return statements are inappropriate here
			throw InterpreterException("A return statement is inappopriate here", 3412);
		}
		else if (stmt_type == "call") {
			Call* call = dynamic_cast<Call*>(statement);
			// check to see if it is a pre-defined function
			if (call->get_func_name() == "print") {
				// do an of the print
				// there is probably a better way to call our predefined functions, but this is ok for now
				if (call->get_args_size() != 1) {
					throw InterpreterException("'print' takes only one argument!'", 3140);
				}
				else {
					std::shared_ptr<Expression> _arg = call->get_arg(0);
					std::string arg_type = _arg->getExpType();

					if (_arg->getExpType() == "literal") {
						Literal* arg = dynamic_cast<Literal*>(_arg.get());
						std::cout << arg->get_value() << std::endl;
					}
					else if (arg_type == "LValue" || arg_type == "dereferenced" || arg_type == "address_of") {
						// an LValue for our argument
						LValue arg;

						// parse the argument correctly -- ensure we evaluate pointer values if necessary
						if (arg_type == "dereferenced") {
							Dereferenced* deref = dynamic_cast<Dereferenced*>(_arg.get());

							std::tuple<Type, std::string> dereferenced_value = this->evaluateExpression(deref, vars_table);
							std::cout << std::get<1>(dereferenced_value) << std::endl;
							return;
						}
						else if (arg_type == "address_of") {
							// if we have an address_of object
							AddressOf* addr_of = dynamic_cast<AddressOf*>(_arg.get());
							arg = addr_of->get_target();
							// get the variable at the address we want
							std::tuple<Type, std::string, std::string> ptd_val = this->getVar(arg.getValue(), vars_table);
							// print its address and return
							std::cout << &ptd_val << std::endl;
							return;
						}
						else if (arg_type == "LValue") {
							arg = *dynamic_cast<LValue*>(_arg.get());
						}

						// create a string containing our value and print it
						std::string val = this->getVarValue(arg, vars_table);
						std::cout << val << std::endl;
						return;
					}
					else if (arg_type == "binary") {
						Binary* arg = dynamic_cast<Binary*>(_arg.get());
						std::cout << std::get<1>(this->evaluateExpression(arg, vars_table)) << std::endl;
					}
					else if (arg_type == "value_returning") {
						ValueReturningFunctionCall* val_ret = dynamic_cast<ValueReturningFunctionCall*>(_arg.get());
						std::tuple<Type, std::string> _exp_tuple = this->evaluateValueReturningFunction(*val_ret, vars_table);
						std::cout << std::get<1>(_exp_tuple) << std::endl;
					}
				}
			}
			// otherwise, if we defined it,
			else {
				// call the function normally
				this->evaluateVoidFunction(*call, vars_table);
			}
		}
	}
}

// evaluate an assignment statement
void Interpreter::evaluateAssignment(Assignment assign, std::list<std::tuple<Type, std::string, std::string>>* vars_table) {
	LValue lvalue = assign.getLValue();
	Expression* RValue = dynamic_cast<Expression*>(assign.getRValue().get());

	Type lvalue_type = std::get<0>(this->getVar(lvalue.getValue(), vars_table));

	std::tuple<Type, std::string> evaluated_rvalue = this->evaluateExpression(RValue, vars_table);
	Type rvalue_type = std::get<0>(evaluated_rvalue);

	this->setVarValue(lvalue, evaluated_rvalue, vars_table);

	return;
}

// Execute a block of statements
void Interpreter::executeBranch(StatementBlock prog, std::list<std::tuple<Type, std::string, std::string>>* vars_table) {
	// iterate through the statement list supplied
	for (std::vector<std::shared_ptr<Statement>>::iterator statement_iter = prog.StatementsList.begin(); statement_iter != prog.StatementsList.end(); statement_iter++) {
		// get the statement to execute by using dynamic_cast on the statement
		Statement* to_execute = dynamic_cast<Statement*>(statement_iter->get());

		// execute the statement
		this->executeStatement(to_execute, vars_table);
	}
}

// Functions

void Interpreter::evaluateVoidFunction(Call func_to_evaluate, std::list<std::tuple<Type, std::string, std::string>>* parent_vars_table) {
	// get the definition for the function we want to evaluate
	Definition func_def = this->getDefinition(func_to_evaluate.get_func_name());

	std::list<std::tuple<Type, std::string, std::string>> local_vars;

	// create instances of all our local function variables
	if (func_to_evaluate.get_args_size() == func_def.get_args().size()) {
		for (int i = 0; i < func_def.get_args().size(); i++) {
			Allocation* current_arg = dynamic_cast<Allocation*>(func_def.get_args()[i].get());
			Expression* arg = dynamic_cast<Expression*>(func_to_evaluate.get_arg(i).get());
			std::tuple<Type, std::string> argv;
			if (arg->getExpType() == "LValue") {
				argv = this->evaluateExpression(arg, parent_vars_table);	// if we pass a variable into a function, we need to be able to evaluate it
			}
			else {
				argv = this->evaluateExpression(arg, &local_vars);	// if it's not a variable, we don't need the table
			}
			if (std::get<0>(argv) == current_arg->getVarType()) {
				local_vars.push_back(std::make_tuple(current_arg->getVarType(), current_arg->getVarName(), std::get<1>(argv)));
			}
			else {
				throw InterpreterException("Argument to function is of improper type, must be '" + get_string_from_type(current_arg->getVarType()) + "', not '" \
					+ get_string_from_type(std::get<0>(argv)) + "'!", 1141);
			}
		}
	}
	else {
		throw InterpreterException("Number of arguments in function call is not equal to number in definition!", 3140);
	}

	this->executeBranch(*dynamic_cast<StatementBlock*>(func_def.get_procedure().get()), &local_vars);

	return;
}

std::tuple<Type, std::string> Interpreter::evaluateValueReturningFunction(ValueReturningFunctionCall func_to_evaluate, std::list<std::tuple<Type, std::string, std::string>>* parent_vars_table) {
	// Get the definition for the function we want to evaluate
	Definition func_def = this->getDefinition(func_to_evaluate.get_func_name());

	std::list<std::tuple<Type, std::string, std::string>> local_vars;

	// create instances of all our local function variables
	if (func_to_evaluate.get_args_size() == func_def.get_args().size()) {
		for (int i = 0; i < func_def.get_args().size(); i++) {
			Allocation* current_arg = dynamic_cast<Allocation*>(func_def.get_args()[i].get());
			Expression* arg = dynamic_cast<Expression*>(func_to_evaluate.get_arg(i).get());
			std::tuple<Type, std::string> argv;
			if (arg->getExpType() == "LValue") {
				argv = this->evaluateExpression(arg, parent_vars_table);
			}
			else {
				argv = this->evaluateExpression(arg, &local_vars);
			}
			if (std::get<0>(argv) == current_arg->getVarType()) {
				local_vars.push_back(std::make_tuple(current_arg->getVarType(), current_arg->getVarName(), std::get<1>(argv)));
			}
			else {
				throw InterpreterException("Argument to function is of improper type, must be '" + get_string_from_type(current_arg->getVarType()) + "', not '" \
					+ get_string_from_type(std::get<0>(argv)) + "'!", 1141);
			}
		}
	}
	else {
		throw InterpreterException("Number of arguments in function call is not equal to number in definition!", 3140);
	}

	int i = 0;
	StatementBlock* procedure = dynamic_cast<StatementBlock*>(func_def.get_procedure().get());
	Expression* return_exp = new Expression();
	while (i < procedure->StatementsList.size()) {
		Statement* statement = dynamic_cast<Statement*>(procedure->StatementsList[i].get());
		if (statement->get_type() != "return") {
			this->executeStatement(statement, &local_vars);
		}
		else {
			ReturnStatement* return_stmt = dynamic_cast<ReturnStatement*>(statement);
			return_exp = dynamic_cast<Expression*>(return_stmt->get_return_exp().get());
		}
		i++;
	}
	std::tuple<Type, std::string> evaluated_return_exp = this->evaluateExpression(return_exp, &local_vars);
	return evaluated_return_exp;
}


// Expressions

std::tuple<Type, std::string> Interpreter::evaluateExpression(Expression* expr, std::list<std::tuple<Type, std::string, std::string>>* vars_table) {
	if (expr->getExpType() == "literal") {
		// evaluate a literal
		Literal* our_val = dynamic_cast<Literal*>(expr);
		return std::make_tuple(our_val->get_type(), our_val->get_value());
	}
	else if (expr->getExpType() == "LValue") {
		LValue* _var = dynamic_cast<LValue*>(expr);
		for (std::list<std::tuple<Type, std::string, std::string>>::iterator var = vars_table->begin(); var != vars_table->end(); ++var) {
			if (_var->getValue() == std::get<1>(*var)) {
				return std::make_tuple(std::get<0>(*var), std::get<2>(*var));
			}
		}
	}
	else if (expr->getExpType() == "address_of") {
		// turn the expr object into a pointer to an AddressOf object
		AddressOf* _addr = dynamic_cast<AddressOf*>(expr);

		// look for "var" with the same name as our _addr object
		for (std::list<std::tuple<Type, std::string, std::string>>::iterator var = vars_table->begin(); var != vars_table->end(); ++var) {
			// if the name of the LValue in _addr is in the variable table
			if (_addr->get_target().getValue() == std::get<1>(*var)) {
				// store the address of var in a intptr_t
				intptr_t address = reinterpret_cast<intptr_t>(&*var);
				std::string addr_string = std::to_string(address);
				Type ptr_addr_type = get_ptr_type(std::get<0>(*var));

				return std::make_tuple(ptr_addr_type, addr_string);
			}
		}
	}
	else if (expr->getExpType() == "dereferenced") {
		// turn the expr object into a pointer to a Dereferenced object
		Dereferenced* _deref = dynamic_cast<Dereferenced*>(expr);
		// the object contained within our "dereferenced" object -- we need to test whether it is an LValue or something else
		Expression* _deref_ptr = dynamic_cast<Expression*>(_deref->get_ptr_shared().get());
		LValue ptr;

		// first, check the type that _deref contains; it could contain another reference or an LValue
		if (_deref_ptr->getExpType() == "LValue") {
			// now, get the LValue in _deref
			ptr = _deref->get_ptr();
		}
		// if it is another reference
		else if (_deref_ptr->getExpType() == "dereferenced") {
			while (_deref_ptr->getExpType() == "dereferenced") {
				_deref = dynamic_cast<Dereferenced*>(_deref_ptr);
				_deref_ptr = dynamic_cast<Expression*>(_deref->get_ptr_shared().get());
			}
			if (_deref_ptr->getExpType() == "LValue") {
				ptr = *dynamic_cast<LValue*>(_deref_ptr);

				// "ptr" now holds "myPtrPtr"
				// now we must get the variable at that address
				Type current_type;

				std::tuple<Type, std::string> _ptr_data = this->evaluateExpression(&ptr, vars_table);

				intptr_t address = (intptr_t)std::stoi(std::get<1>(_ptr_data));
				std::tuple<Type, std::string, std::string>* _next = (std::tuple<Type, std::string, std::string>*)address;
				current_type = std::get<0>(*_next);

				while (current_type == PTRPTR) {
					address = (intptr_t)std::stoi(std::get<2>(*_next));
					_next = (std::tuple<Type, std::string, std::string>*)address;
					current_type = std::get<0>(*_next);
				}

				address = (intptr_t)std::stoi(std::get<2>(*_next));
				std::tuple<Type, std::string, std::string> to_return = *(std::tuple<Type, std::string, std::string>*)address;
				return std::make_tuple(std::get<0>(to_return), std::get<2>(to_return));
			}
			else {
				throw InterpreterException("Unexpected exception in pointer dereferencing", 000);
			}
		}

		// now that we have the LValue "ptr"
		// get the value in that variable
		for (std::list<std::tuple<Type, std::string, std::string>>::iterator var = vars_table->begin(); var != vars_table->end(); ++var) {
			// if the names match
			if (ptr.getValue() == std::get<1>(*var)) {
				// get the address of the variable by getting the address of our the variable pointed to by the iterator
				std::string addr_string = (std::get<2>(*var));
				intptr_t address = (intptr_t)std::stoi(addr_string);

				// now that we have the correct address, create a pointer to the tuple containing the proper information
				std::tuple<Type, std::string, std::string>* _dereferenced = (std::tuple<Type, std::string, std::string>*)address;

				// make sure the ptr type of var is equal to the pointer type that should point to _dereferenced
				// OR, one of them must be of type "ptrptr"
				if (match_ptr_types(std::get<0>(*var), std::get<0>(*_dereferenced))) {
					return std::make_tuple(std::get<0>(*_dereferenced), std::get<2>(*_dereferenced));
				}
				else {
					throw InterpreterException("Pointer types do not match (error encountered in dereference)", 000);
				}
			}
		}

		// TODO: what if we can't find the variable in the vector?

	}
	else if (expr->getExpType() == "binary") {
		// cast the expression to a binary so we have access to our functions
		Binary* binary_exp = dynamic_cast<Binary*>(expr);

		// Now, create our left side, right side, and operator objects
		Expression* left_exp = dynamic_cast<Expression*>(binary_exp->get_left().get());
		Expression* right_exp = dynamic_cast<Expression*>(binary_exp->get_right().get());
		exp_operator op = binary_exp->get_operator();

		std::tuple<Type, std::string> left_exp_result = this->evaluateExpression(left_exp, vars_table);
		std::tuple<Type, std::string> right_exp_result = this->evaluateExpression(right_exp, vars_table);

		Type left_t = std::get<0>(left_exp_result);
		Type right_t = std::get<0>(right_exp_result);

		// First, make sure our expressions are of the same type
		if (left_t == right_t) {
			return this->evaluateBinary(left_exp_result, right_exp_result, op);
		}
		// Expressions are not of the same type
		else {
			throw InterpreterException("Expressions in a binary expression must have the same type.", 3101);
		}
	}
	else if (expr->getExpType() == "unary") {
		// evaluate a unary expression
	}
	else if (expr->getExpType() == "value_returning") {
		ValueReturningFunctionCall* call = dynamic_cast<ValueReturningFunctionCall*>(expr);
		// check to make sure it's not a predefined function
		if (call->get_func_name() == "input") {
			// get input and return a string
			std::string input;
			if (call->get_args_size() != 1) {
				throw InterpreterException("'input' only takes one argument!", 3140);
			}
			else {
				std::shared_ptr<Expression> _arg = call->get_arg(0);
				if (_arg->getExpType() == "literal") {
					Literal* literal_arg = dynamic_cast<Literal*>(_arg.get());
					if (get_string_from_type(literal_arg->get_type()) == "string") {
						std::cout << literal_arg->get_value() << std::endl;
						std::getline(std::cin, input);
						return std::make_tuple(STRING, input);
					}
					else {
						throw InterpreterException("Argument must be of type 'string'", 1141);
					}
				}
				else if (_arg->getExpType() == "LValue") {
					LValue* lvalue_arg = dynamic_cast<LValue*>(_arg.get());
				}
				else if (_arg->getExpType() == "value_returning") {
					ValueReturningFunctionCall* val_ret = dynamic_cast<ValueReturningFunctionCall*>(_arg.get());
				}
			}
		}
		else if (call->get_func_name() == "stoi") {
			std::string to_convert;
			int converted;
			if (call->get_args_size() != 1) {
				throw InterpreterException("'stoi' only takes one argument!", 3140);
			}
			else {
				std::shared_ptr<Expression> _arg = call->get_arg(0);
				if (_arg->getExpType() == "literal") {
					Literal* literal_arg = dynamic_cast<Literal*>(_arg.get());
					if (get_string_from_type(literal_arg->get_type()) == "string") {
						to_convert = literal_arg->get_value();
						try {
							converted = std::stoi(to_convert);
							return std::make_tuple(INT, std::to_string(converted));
						}
						catch (const std::invalid_argument& ia) {
							std::cerr << "\n" << ia.what() << std::endl;
							throw InterpreterException("Cannot convert '" + to_convert + "' to type 'int'!", 2110);
						}
					}
				}
				else if (_arg->getExpType() == "LValue") {
					LValue* var = dynamic_cast<LValue*>(_arg.get());
					std::tuple<Type, std::string> evaluated = this->evaluateExpression(var, vars_table);
					if (std::get<0>(evaluated)) {
						to_convert = std::get<1>(evaluated);
						try {
							converted = std::stoi(to_convert);
							return std::make_tuple(INT, std::to_string(converted));
						}
						catch (std::invalid_argument& ia) {
							std::cerr << "\n" << ia.what() << std::endl;
							throw InterpreterException("Cannot convert '" + to_convert + "' to type 'int'!", 2110);
						}
					}
					else {
						throw InterpreterException("Cannot convert expressions of this type with 'stoi'!", 2110);
					}
				}
				else {
					throw InterpreterException("Cannot convert expressions of this type with 'stoi'!", 2110);
				}
			}
		}
		// then it is one we defined
		else {
			std::tuple<Type, std::string> evaluated_return_expression = this->evaluateValueReturningFunction(*call, vars_table);
			return evaluated_return_expression;
		}
	}
	else if (expr->getExpType() == "var") {

	}
}


// Returns a tuple containing the result of our binary expression
std::tuple<Type, std::string> Interpreter::evaluateBinary(std::tuple<Type, std::string> left, std::tuple<Type, std::string> right, exp_operator op) {
	// get the types of our operands
	Type left_t = std::get<0>(left);
	Type right_t = std::get<0>(right);
	// Evaluate the result of our expression by discerning what operation we are actually trying to do
	if (op == PLUS) {
		// Since the types are the same, we only need to test one of them
		if (left_t == INT) {
			int sum = std::stoi(std::get<1>(left)) + std::stoi(std::get<1>(right));
			return std::make_tuple(INT, std::to_string(sum));
		}
		else if (left_t == FLOAT) {
			float sum = std::stof(std::get<1>(left)) + std::stof(std::get<1>(right));
			return std::make_tuple(FLOAT, std::to_string(sum));
		}
		else if (left_t == STRING) {
			std::string concatenated = std::get<1>(left) + std::get<1>(right);
			return std::make_tuple(STRING, concatenated);
		}
		else {
			throw InterpreterException("Addition cannot be performed on expressions of this type", 1123);
		}
	}
	else if (op == MINUS) {
		if (left_t == INT) {
			int diff = std::stoi(std::get<1>(left)) - std::stoi(std::get<1>(right));
			return std::make_tuple(INT, std::to_string(diff));
		}
		else if (left_t == FLOAT) {
			float diff = std::stof(std::get<1>(left)) - std::stof(std::get<1>(right));
			return std::make_tuple(FLOAT, std::to_string(diff));
		}
		else {
			throw InterpreterException("Subtraction cannot be performed on expressions of this type", 1123);
		}
	}
	else if (op == MULT) {
		if (left_t == INT) {
			int product = std::stoi(std::get<1>(left)) * std::stoi(std::get<1>(right));
			return std::make_tuple(INT, std::to_string(product));
		}
		else if (left_t == FLOAT) {
			float product = std::stof(std::get<1>(left)) * std::stof(std::get<1>(right));
			return std::make_tuple(FLOAT, std::to_string(product));
		}
		// Add support for string repitition?
		else {
			throw InterpreterException("Multiplication cannot be performed on expressions of this type", 1123);
		}
	}
	else if (op == DIV) {
		if (left_t == INT) {
			int quotient = std::stoi(std::get<1>(left)) / std::stoi(std::get<1>(right));
			return std::make_tuple(INT, std::to_string(quotient));
		}
		else if (left_t == FLOAT) {
			float quotient = std::stof(std::get<1>(left)) / std::stof(std::get<1>(right));
			return std::make_tuple(FLOAT, std::to_string(quotient));
		}
		else {
			throw InterpreterException("Division cannot be performed on expressions of this type", 1123);
		}
	}
	else if (op == GREATER) {
		if (left_t == INT) {
			int _left = std::stoi(std::get<1>(left));
			int _right = std::stoi(std::get<1>(right));
			bool greater = _left > _right;
			return std::make_tuple(BOOL, boolString(greater));
		}
		else if (left_t == FLOAT) {
			float _left = std::stof(std::get<1>(left));
			float _right = std::stof(std::get<1>(right));
			bool greater = _left > _right;
			return std::make_tuple(BOOL, boolString(greater));
		}
		else {
			throw InterpreterException("Comparison cannot be performed on expressions of this type", 1123);
		}
	}
	else if (op == GREATER_OR_EQUAL) {
		if (left_t == INT) {
			int _left = std::stoi(std::get<1>(left));
			int _right = std::stoi(std::get<1>(right));
			bool greater = _left >= _right;
			return std::make_tuple(BOOL, boolString(greater));
		}
		else if (left_t == FLOAT) {
			float _left = std::stof(std::get<1>(left));
			float _right = std::stof(std::get<1>(right));
			bool greater = _left >= _right;
			return std::make_tuple(BOOL, boolString(greater));
		}
		else {
			throw InterpreterException("Comparison cannot be performed on expressions of this type", 1123);
		}
	}
	else if (op == LESS) {
		if (left_t == INT) {
			int _left = std::stoi(std::get<1>(left));
			int _right = std::stoi(std::get<1>(right));
			bool greater = _left < _right;
			return std::make_tuple(BOOL, boolString(greater));
		}
		else if (left_t == FLOAT) {
			float _left = std::stof(std::get<1>(left));
			float _right = std::stof(std::get<1>(right));
			bool greater = _left < _right;
			return std::make_tuple(BOOL, boolString(greater));
		}
		else {
			throw InterpreterException("Comparison cannot be performed on expressions of this type", 1123);
		}
	}
	else if (op == LESS_OR_EQUAL) {
		if (left_t == INT) {
			int _left = std::stoi(std::get<1>(left));
			int _right = std::stoi(std::get<1>(right));
			bool greater = _left <= _right;
			return std::make_tuple(BOOL, boolString(greater));
		}
		else if (left_t == FLOAT) {
			float _left = std::stof(std::get<1>(left));
			float _right = std::stof(std::get<1>(right));
			bool greater = _left <= _right;
			return std::make_tuple(BOOL, boolString(greater));
		}
		else {
			throw InterpreterException("Comparison cannot be performed on expressions of this type", 1123);
		}
	}
	else if (op == EQUAL) {
		bool equals = std::get<1>(left) == std::get<1>(right);
		return std::make_tuple(BOOL, boolString(equals));
	}
	else if (op == NOT_EQUAL) {
		bool not_equals = std::get<1>(left) != std::get<1>(right);
		return std::make_tuple(BOOL, boolString(not_equals));
	}
	else if (op == AND) {
		bool and = toBool(std::get<1>(left)) && toBool(std::get<1>(right));
		return std::make_tuple(BOOL, boolString(and));
	}
	else if (op == OR) {
		bool or = toBool(std::get<1>(left)) || toBool(std::get<1>(right));
		return std::make_tuple(BOOL, boolString(or ));
	}
	else {
		throw InterpreterException("Unrecognized operator in binary expression!", 3999);
	}
}


// get a function definition
Definition Interpreter::getDefinition(std::string func_to_find) {
	// create a list iterator which will iterate through our function list
	std::list<Definition>::iterator func_iter = this->function_table.begin();
	bool found = false;
	while (func_iter != this->function_table.end() && !found) {
		// get our function's name as an LValue pointer -- because function names are considered LValues
		// maybe they should be considered literals or some other class, as they aren't really LValues, but that is to change later
		LValue* table_name = dynamic_cast<LValue*>(func_iter->get_name().get());
		// if the two values are the same
		if (func_to_find == table_name->getValue()) {
			found = true;
		}
		// if they are not
		else {
			// increment func_it
			func_iter++;
		}
	}
	if (found) {
		// We have found the function
		return *func_iter;
	}
	else {
		throw InterpreterException("Could not find a definition for the function referenced", 3034);
	}
}



/************************************************************************
******************										*****************
******************		PUBLIC MEMBER FUNCTIONS			*****************
******************										*****************
*************************************************************************/


// The interpreter's entry function. Requires only an AST object (StatementBlock), and it will use the Interpreter object's global variable table.
// This function will also catch any interpreter errors thrown during execution
void Interpreter::interpretAST(StatementBlock AST) {
	try {
		this->executeBranch(AST, &this->var_table);
	}
	catch (InterpreterException &i_e) {
		std::cerr << "Interpreter Exception: \n\t" << i_e.what() << "\n\tCode: " << i_e.get_code() << std::endl;
	}
	return;
}

Interpreter::Interpreter()
{
}


Interpreter::~Interpreter()
{
}



/************************************************************************
******************										*****************
******************		INTERPRETER EXCEPTION			*****************
******************										*****************
*************************************************************************/


const char* InterpreterException::what() const {
	return InterpreterException::message_.c_str();
}

int InterpreterException::get_code() {
	return this->code_;
}

InterpreterException::InterpreterException(const std::string& err_message, const int& err_code) : message_(err_message), code_(err_code) {

}

InterpreterException::InterpreterException() {

}

const char* TypeMatchError::what() const {
	return TypeMatchError::message_.c_str();
}

TypeMatchError::TypeMatchError(const Type& a, const Type& b) : a_(a), b_(b) {
	TypeMatchError::message_ = "Cannot match '" + get_string_from_type(a_) + "' and '" + get_string_from_type(b_) + "'!";
	TypeMatchError::code_ = 450;
}
