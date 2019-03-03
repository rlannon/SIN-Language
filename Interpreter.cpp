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

const bool Interpreter::are_compatible_types(Type a, Type b) {
	if (a == b) {
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


InterpreterSymbol Interpreter::getVar(std::string var_to_find, std::list<InterpreterSymbol>* vars_table) {
	for (std::list<InterpreterSymbol>::iterator var_iter = vars_table->begin(); var_iter != vars_table->end(); var_iter++) {
		if (var_iter->name == var_to_find) {
			return *var_iter;
		}
	}

	// if we can't find it, we will reach this
	std::string err_msg = "Could not find variable '" + var_to_find + "' in this scope!";
	throw InterpreterException(err_msg.c_str(), 000);
}


std::string Interpreter::getVarValue(LValue variable, std::list<InterpreterSymbol>* vars_table) {
	// find the variable in the symbol table
	try {
		InterpreterSymbol var_in_table = this->getVar(variable.getValue(), vars_table);

		// now, check the type
		if (variable.getLValueType() == "var" || variable.getLValueType() == "var_address") {
			return var_in_table.value;
		}
		else if (variable.getLValueType() == "var_dereferenced") {
			// our address is in position 2 in the tuple at *var_iter
			// convert it into an actual pointer int
			intptr_t address = (intptr_t)std::stoi(var_in_table.value);

			// now that we have the address, get the variable at that address
			InterpreterSymbol* dereferenced = (InterpreterSymbol*)address;

			// make sure the pointer and the value it is referecing are compatible types
			if (are_compatible_types(var_in_table.data_type, dereferenced->data_type)) {
				// return the dereferenced value
				return dereferenced->value;
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

void Interpreter::setVarValue(LValue variable, std::tuple<Type, std::string> new_value, std::list<InterpreterSymbol>* vars_table) {
	// find the variable in the symbol table
	for (std::list<InterpreterSymbol>::iterator var_iter = vars_table->begin(); var_iter != vars_table->end(); var_iter++) {
		if (var_iter->name == variable.getValue()) {
			// now, check the type
			if (variable.getLValueType() == "var" || variable.getLValueType() == "var_address") {
				// check for compatible types
				if (this->are_compatible_types(var_iter->data_type, std::get<0>(new_value)) || this->are_compatible_types(var_iter->subtype, std::get<0>(new_value))) {
					// update the value
					
					// currently, RAW is not supported in Interpreted SIN
					if (is_raw(var_iter->data_type) || is_raw(std::get<0>(new_value))) {
						throw InterpreterException("Interpreted SIN does not support the use of the RAW type", 1234);
					}
					var_iter->value = std::get<1>(new_value);
					return;
				}
				else {
					throw TypeMatchError(var_iter->data_type, std::get<0>(new_value));
				}
			}
			else if (variable.getLValueType() == "var_dereferenced") {
				// get the value stored in the pointer, which is an address
				intptr_t address = (intptr_t)std::stoi(var_iter->value);
				// get the value at that address
				InterpreterSymbol* dereferenced = (InterpreterSymbol*)address;

				// continue updating the address as long as we have a pointer type
				while (dereferenced->data_type == PTR) {
					// get the address stored in the current variable 'dereferenced'
					address = (intptr_t)std::stoi(dereferenced->value);
					// put the tuple at that address in DEREFERENCED
					dereferenced = (InterpreterSymbol*)address;
				}

				// check for type compatibility
				if (this->are_compatible_types(dereferenced->data_type, std::get<0>(new_value))) {
					// update the value at that address
					dereferenced->value = std::get<1>(new_value);
					return;
				}
				else {
					throw TypeMatchError(std::get<0>(new_value), dereferenced->data_type);
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
void Interpreter::allocateVar(Allocation allocation, std::list<InterpreterSymbol>* vars_table) {
	// get the initial value
	std::tuple<Type, std::string> initial_value;
	if (allocation.get_initial_value()->get_expression_type() == EXPRESSION_GENERAL) {
		initial_value = std::make_tuple(NONE, "");
	}
	else {
		 initial_value = this->evaluateExpression(allocation.get_initial_value().get(), vars_table);
	}

	// if we have an initialized value that isn't the proper type, throw an error
	if ((std::get<0>(initial_value) != NONE) && (std::get<0>(initial_value) != allocation.get_var_type())) {
		throw std::runtime_error("Mismatched type in alloc-define statement! (line " + std::to_string(allocation.get_line_number()) + ")");
	}
	else {
		// Initialize the variable with its type, name
		InterpreterSymbol var = InterpreterSymbol(allocation.get_var_type(), allocation.get_var_name(), std::get<1>(initial_value), allocation.get_var_subtype());

		// Push our newly-allocated variable to our variables vector
		vars_table->push_back(var);
	}
}

// define a function
void Interpreter::defineFunction(Definition definition) {
	Interpreter::function_table.push_back(definition);
}

// execute a single statement
void Interpreter::executeStatement(Statement* statement, std::list<InterpreterSymbol>* vars_table) {
	if (statement == NULL) {
		throw InterpreterException("Expected a statement", 3411);
	}
	else {
		stmt_type stmt_type = statement->get_statement_type();

		if (stmt_type == ALLOCATION) {
			Allocation* alloc = dynamic_cast<Allocation*>(statement);
			this->allocateVar(*alloc, vars_table);
		}
		else if (stmt_type == DEFINITION) {
			Definition* def = dynamic_cast<Definition*>(statement);
			this->defineFunction(*def);
		}
		else if (stmt_type == ASSIGNMENT) {
			Assignment* assign = dynamic_cast<Assignment*>(statement);
			this->evaluateAssignment(*assign, vars_table);
		}
		else if (stmt_type == IF_THEN_ELSE) {
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
		else if (stmt_type == WHILE_LOOP) {
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
		else if (stmt_type == RETURN_STATEMENT) {
			// return statements are inappropriate here
			throw InterpreterException("A return statement is inappopriate here", 3412);
		}
		else if (stmt_type == CALL) {
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
					exp_type arg_type = _arg->get_expression_type();

					if (_arg->get_expression_type() == LITERAL) {
						Literal* arg = dynamic_cast<Literal*>(_arg.get());
						std::cout << arg->get_value() << std::endl;
					}
					else if (arg_type == LVALUE || arg_type == DEREFERENCED || arg_type == ADDRESS_OF) {
						// an LValue for our argument
						LValue arg;

						// parse the argument correctly -- ensure we evaluate pointer values if necessary
						if (arg_type == DEREFERENCED) {
							Dereferenced* deref = dynamic_cast<Dereferenced*>(_arg.get());

							std::tuple<Type, std::string> dereferenced_value = this->evaluateExpression(deref, vars_table);
							std::cout << std::get<1>(dereferenced_value) << std::endl;
							return;
						}
						else if (arg_type == ADDRESS_OF) {
							// if we have an address_of object
							AddressOf* addr_of = dynamic_cast<AddressOf*>(_arg.get());
							arg = addr_of->get_target();
							// get the variable at the address we want
							InterpreterSymbol ptd_val = this->getVar(arg.getValue(), vars_table);
							// print its address and return
							std::cout << &ptd_val << std::endl;
							return;
						}
						else if (arg_type == LVALUE) {
							arg = *dynamic_cast<LValue*>(_arg.get());
						}

						// create a string containing our value and print it
						std::string val = this->getVarValue(arg, vars_table);
						std::cout << val << std::endl;
						return;
					}
					else if (arg_type == BINARY) {
						Binary* arg = dynamic_cast<Binary*>(_arg.get());
						std::cout << std::get<1>(this->evaluateExpression(arg, vars_table)) << std::endl;
					}
					else if (arg_type == VALUE_RETURNING_CALL) {
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
void Interpreter::evaluateAssignment(Assignment assign, std::list<InterpreterSymbol>* vars_table) {
	LValue lvalue;
	if (assign.get_lvalue()->get_expression_type() == LVALUE) {
		lvalue = *dynamic_cast<LValue*>(assign.get_lvalue().get());
	}

	Expression* RValue = dynamic_cast<Expression*>(assign.get_rvalue().get());

	Type lvalue_type = this->getVar(lvalue.getValue(), vars_table).data_type;

	std::tuple<Type, std::string> evaluated_rvalue = this->evaluateExpression(RValue, vars_table);
	Type rvalue_type = std::get<0>(evaluated_rvalue);

	this->setVarValue(lvalue, evaluated_rvalue, vars_table);

	return;
}

// Execute a block of statements
void Interpreter::executeBranch(StatementBlock prog, std::list<InterpreterSymbol>* vars_table) {
	// iterate through the statement list supplied
	for (std::vector<std::shared_ptr<Statement>>::iterator statement_iter = prog.statements_list.begin(); statement_iter != prog.statements_list.end(); statement_iter++) {
		// get the statement to execute by using dynamic_cast on the statement
		Statement* to_execute = dynamic_cast<Statement*>(statement_iter->get());

		// execute the statement
		this->executeStatement(to_execute, vars_table);
	}
}

// Functions

void Interpreter::evaluateVoidFunction(Call func_to_evaluate, std::list<InterpreterSymbol>* parent_vars_table) {
	// get the definition for the function we want to evaluate
	Definition func_def = this->getDefinition(func_to_evaluate.get_func_name());

	std::list<InterpreterSymbol> local_vars;

	// create instances of all our local function variables
	if (func_to_evaluate.get_args_size() == func_def.get_args().size()) {
		for (size_t i = 0; i < func_def.get_args().size(); i++) {
			Allocation* current_arg = dynamic_cast<Allocation*>(func_def.get_args()[i].get());
			Expression* arg = dynamic_cast<Expression*>(func_to_evaluate.get_arg(i).get());
			std::tuple<Type, std::string> argv;
			if (arg->get_expression_type() == LVALUE) {
				argv = this->evaluateExpression(arg, parent_vars_table);	// if we pass a variable into a function, we need to be able to evaluate it
			}
			else {
				argv = this->evaluateExpression(arg, &local_vars);	// if it's not a variable, we don't need the table
			}
			if (std::get<0>(argv) == current_arg->get_var_type()) {
				local_vars.push_back(InterpreterSymbol(current_arg->get_var_type(), current_arg->get_var_name(), std::get<1>(argv)));
			}
			else {
				throw InterpreterException("Argument to function is of improper type, must be '" + get_string_from_type(current_arg->get_var_type()) + "', not '" \
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

std::tuple<Type, std::string> Interpreter::evaluateValueReturningFunction(ValueReturningFunctionCall func_to_evaluate, std::list<InterpreterSymbol>* parent_vars_table) {
	// Get the definition for the function we want to evaluate
	Definition func_def = this->getDefinition(func_to_evaluate.get_func_name());

	std::list<InterpreterSymbol> local_vars;

	// create instances of all our local function variables
	if (func_to_evaluate.get_args_size() == func_def.get_args().size()) {
		for (size_t i = 0; i < func_def.get_args().size(); i++) {
			Allocation* current_arg = dynamic_cast<Allocation*>(func_def.get_args()[i].get());
			Expression* arg = dynamic_cast<Expression*>(func_to_evaluate.get_arg(i).get());
			std::tuple<Type, std::string> argv;
			if (arg->get_expression_type() == LVALUE) {
				argv = this->evaluateExpression(arg, parent_vars_table);
			}
			else {
				argv = this->evaluateExpression(arg, &local_vars);
			}
			if (std::get<0>(argv) == current_arg->get_var_type()) {
				local_vars.push_back(InterpreterSymbol(current_arg->get_var_type(), current_arg->get_var_name(), std::get<1>(argv)));
			}
			else {
				throw InterpreterException("Argument to function is of improper type, must be '" + get_string_from_type(current_arg->get_var_type()) + "', not '" \
					+ get_string_from_type(std::get<0>(argv)) + "'!", 1141);
			}
		}
	}
	else {
		throw InterpreterException("Number of arguments in function call is not equal to number in definition!", 3140);
	}

	size_t i = 0;
	StatementBlock* procedure = dynamic_cast<StatementBlock*>(func_def.get_procedure().get());
	Expression* return_exp = new Expression();
	while (i < procedure->statements_list.size()) {
		Statement* statement = dynamic_cast<Statement*>(procedure->statements_list[i].get());
		if (statement->get_statement_type() != RETURN_STATEMENT) {
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

std::tuple<Type, std::string> Interpreter::evaluateExpression(Expression* expr, std::list<InterpreterSymbol>* vars_table) {
	if (expr->get_expression_type() == LITERAL) {
		// evaluate a literal
		Literal* our_val = dynamic_cast<Literal*>(expr);
		return std::make_tuple(our_val->get_type(), our_val->get_value());
	}
	else if (expr->get_expression_type() == LVALUE) {
		LValue* _var = dynamic_cast<LValue*>(expr);
		for (std::list<InterpreterSymbol>::iterator var = vars_table->begin(); var != vars_table->end(); ++var) {
			if (_var->getValue() == var->name) {
				return std::make_tuple(var->data_type, var->value);
			}
		}
	}
	else if (expr->get_expression_type() == ADDRESS_OF) {
		// turn the expr object into a pointer to an AddressOf object
		AddressOf* _addr = dynamic_cast<AddressOf*>(expr);

		// look for "var" with the same name as our _addr object
		for (std::list<InterpreterSymbol>::iterator var = vars_table->begin(); var != vars_table->end(); ++var) {
			// if the name of the LValue in _addr is in the variable table
			if (_addr->get_target().getValue() == var->name) {
				// store the address of var in a intptr_t
				intptr_t address = reinterpret_cast<intptr_t>(&*var);
				std::string addr_string = std::to_string(address);
				Type ptr_addr_type = var->data_type;	// TODO: overhaul Interpreter pointer types

				return std::make_tuple(ptr_addr_type, addr_string);
			}
		}
	}
	else if (expr->get_expression_type() == DEREFERENCED) {
		// turn the expr object into a pointer to a Dereferenced object
		Dereferenced* _deref = dynamic_cast<Dereferenced*>(expr);
		// the object contained within our DEREFERENCED object -- we need to test whether it is an LValue or something else
		Expression* _deref_ptr = dynamic_cast<Expression*>(_deref->get_ptr_shared().get());
		LValue ptr;

		// first, check the type that _deref contains; it could contain another reference or an LValue
		if (_deref_ptr->get_expression_type() == LVALUE) {
			// now, get the LValue in _deref
			ptr = _deref->get_ptr();
		}
		// if the pointer contains another reference
		else if (_deref_ptr->get_expression_type() == DEREFERENCED) {
			while (_deref_ptr->get_expression_type() == DEREFERENCED) {
				_deref = dynamic_cast<Dereferenced*>(_deref_ptr);
				_deref_ptr = dynamic_cast<Expression*>(_deref->get_ptr_shared().get());
			}
			if (_deref_ptr->get_expression_type() == LVALUE) {
				ptr = *dynamic_cast<LValue*>(_deref_ptr);

				// "ptr" now holds "myPtrPtr"
				// now we must get the variable at that address
				Type current_type;
				Type current_subtype;

				std::tuple<Type, std::string> _ptr_data = this->evaluateExpression(&ptr, vars_table);

				intptr_t address = (intptr_t)std::stoi(std::get<1>(_ptr_data));
				InterpreterSymbol* _next = (InterpreterSymbol*)address;
				current_type = _next->data_type;
				current_subtype = _next->subtype;

				while (current_type == PTR && current_subtype == PTR) {
					address = (intptr_t)std::stoi(_next->value);
					_next = (InterpreterSymbol*)address;
					current_type = _next->data_type;
					current_subtype = _next->subtype;
				}

				address = (intptr_t)std::stoi(_next->value);
				InterpreterSymbol to_return = *(InterpreterSymbol*)address;
				return std::make_tuple(to_return.data_type, to_return.value);
			}
			else {
				throw InterpreterException("Unexpected exception in pointer dereferencing", 000);
			}
		}

		// now that we have the LValue "ptr"
		// get the value in that variable
		for (std::list<InterpreterSymbol>::iterator var = vars_table->begin(); var != vars_table->end(); ++var) {
			// if the names match
			if (ptr.getValue() == var->name) {
				// get the address of the variable by getting the address of our the variable pointed to by the iterator
				std::string addr_string = var->value;
				intptr_t address = (intptr_t)std::stoi(addr_string);

				// now that we have the correct address, create a pointer to the symbol containing the proper information
				InterpreterSymbol* _dereferenced = (InterpreterSymbol*)address;

				// make sure type of var is equal to the subtype of _dereferenced
				if (are_compatible_types(var->subtype, _dereferenced->data_type)) {
					return std::make_tuple(_dereferenced->subtype, _dereferenced->value);
				}
				else {
					throw InterpreterException("Pointer types do not match (error encountered in dereference)", 000);
				}
			}
		}

		// TODO: what if we can't find the variable in the vector?

	}
	else if (expr->get_expression_type() == BINARY) {
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
	else if (expr->get_expression_type() == UNARY) {
		// evaluate a unary expression
		Unary* unary_expression = dynamic_cast<Unary*>(expr);
		Expression* operand = unary_expression->get_operand().get();
		exp_operator unary_operator = unary_expression->get_operator();
		
		// declare our operand value and its data type; initialize them to nothing so that we don't generate compiler warnings
		std::string operand_value = "";
		Type operand_type = NONE;

		// get the value and type for the operand
		if (operand->get_expression_type() == LITERAL) {
			Literal* literal_expression = dynamic_cast<Literal*>(operand);
			operand_value = literal_expression->get_value();
			operand_type = literal_expression->get_type();
		}
		else if (operand->get_expression_type() == LVALUE) {
			LValue* lvalue_expression = dynamic_cast<LValue*>(operand);
			InterpreterSymbol var_to_evaluate = this->getVar(lvalue_expression->getValue(), vars_table);	// tuple = variable's type, variable's name, variable's value
			// assign the type and value to our previously declared members
			operand_type = var_to_evaluate.data_type;
			operand_value = var_to_evaluate.value;
		}
		else {
			// if it isn't a valid type, throw an exception
			// TODO: get the invalid type error number in interpretation
			throw InterpreterException("**** Cannot perform unary evaluation", 000);
		}

		if (unary_operator == PLUS){
			// simply return the value -- unary plus does nothing
			std::tuple<Type, std::string> value_to_return = std::make_tuple(operand_type, operand_value);
			return value_to_return;
		}
		else if (unary_operator == MINUS) {
			std::tuple<Type, std::string> value_to_return;
			// make sure the data type is one that we can use minus with
			if (operand_type == INT) {
				// get the value, change its sign, and return it
				int converted_operand_value = std::stoi(operand_value);
				converted_operand_value = -converted_operand_value;
				operand_value = std::to_string(converted_operand_value);
				return std::make_tuple(operand_type, operand_value);
			}
			else if (operand_type == FLOAT) {
				// get the value, change its sign, and return it
				float converted_operand_value = std::stof(operand_value);
				converted_operand_value = -converted_operand_value;
				operand_value = std::to_string(converted_operand_value);
				return std::make_tuple(operand_type, operand_value);
			}
			else {
				throw InterpreterException("**** Cannot perform 'MINUS' unary operation on expressions of type '" + get_string_from_type(operand_type) + "'; must be 'int' or 'float'", 000);
			}
		}
		else if (unary_operator == NOT) {
			// make sure the data type is one we can use "not" with (i.e., a bool)
			if (operand_type == BOOL) {
				if (operand_value == "True") {
					return std::make_tuple(operand_type, "False");
				}
				else if (operand_value == "False") {
					return std::make_tuple(operand_type, "True");
				}
				else {
					throw InterpreterException("Type was bool, but received unexpected value.", 000);
				}
			}
			else {
				throw InterpreterException("**** Cannot perform 'NOT' unary operation on expressions of type '" + get_string_from_type(operand_type) + "'; type must be 'bool'", 000);
			}
		}
	}
	else if (expr->get_expression_type() == VALUE_RETURNING_CALL) {
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
				if (_arg->get_expression_type() == LITERAL) {
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
				else if (_arg->get_expression_type() == LVALUE) {
					LValue* lvalue_arg = dynamic_cast<LValue*>(_arg.get());
				}
				else if (_arg->get_expression_type() == VALUE_RETURNING_CALL) {
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
				if (_arg->get_expression_type() == LITERAL) {
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
				else if (_arg->get_expression_type() == LVALUE) {
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


const char* InterpreterException::what() const noexcept {
	return InterpreterException::message_.c_str();
}

int InterpreterException::get_code() {
	return this->code_;
}

InterpreterException::InterpreterException(const std::string& err_message, const int& err_code) : message_(err_message), code_(err_code) {

}

InterpreterException::InterpreterException() {

}

const char* TypeMatchError::what() const noexcept {
	return TypeMatchError::message_.c_str();
}

TypeMatchError::TypeMatchError(const Type& a, const Type& b) : a_(a), b_(b) {
	TypeMatchError::message_ = "Cannot match '" + get_string_from_type(a_) + "' and '" + get_string_from_type(b_) + "'!";
	TypeMatchError::code_ = 450;
}

InterpreterSymbol::InterpreterSymbol(Type data_type, std::string name, std::string value, Type subtype) : data_type(data_type), name(name), value(value), subtype(subtype)
{
}

InterpreterSymbol::InterpreterSymbol()
{
	this->data_type = NONE;
	this->subtype = NONE;
	this->name = "";
	this->value = "";
}
