#include "Interpreter.h"

const bool Interpreter::to_bool(std::string val) {
	if (val == "true" || val == "True" || val == "TRUE") {
		return true;
	}
	else if (val == "false" || val == "False" || val == "FALSE") {
		return false;
	}
	else {
		error("Error in bool value" , 2130);
	}
}

const std::string Interpreter::bool_string(bool val) {
	if (val == true) {
		return "True";
	}
	else {
		return "False";
	}
}


std::string Interpreter::get_var_value(std::string var_name, std::vector<std::tuple<Type, std::string, std::string>>* vars_table) {
	for (std::vector<std::tuple<Type, std::string, std::string>>::iterator var = vars_table->begin(); var != vars_table->end(); ++var) {
		if (var_name == std::get<1>(*var)) {
			return std::get<2>(*var);
		}
	}
	
	// if we reach this point, we have not found the variable
	this->error("No allocation found for variable '" + var_name + "'!", 3024);
}

void Interpreter::set_var_value(std::string var_name, std::string new_value, std::vector<std::tuple<Type, std::string, std::string>>* vars_table) {
	for (std::vector<std::tuple<Type, std::string, std::string>>::iterator var = vars_table->begin(); var != vars_table->end(); ++var) {
		if (var_name == std::get<1>(*var)) {
			std::get<2>(*var) = new_value;
			return;
		}
	}

	// if we reach this point, we have not found the variable
	this->error("No allocation found for variable '" + var_name + "'!", 3024);
}


// Our error function
const void Interpreter::error(std::string message, int code) {
	std::cerr << std::endl << "INTERPRETER ERROR:" << "\n\t" << message << std::endl;
	throw code;
}

// get the address of our variables table
std::vector<std::tuple<Type, std::string, std::string>>* Interpreter::get_vars_table() {
	return &this->vars_table;
}

// Evalute an allocation
void Interpreter::allocate_var(Allocation allocation, std::vector<std::tuple<Type, std::string, std::string>>* vars_table) {
	// Initialize the variable with its type, name, and no value / NULL value
	std::tuple<Type, std::string, std::string> var = std::make_tuple<Type, std::string, std::string>(allocation.getVarType(), allocation.getVarName(), "");

	// Push our newly-allocated variable to our variables vector
	vars_table->push_back(var);
}

void Interpreter::define_function(Definition definition) {
	Interpreter::functions_table.push_back(definition);
}



void Interpreter::execute_statements(StatementBlock prog, std::vector<std::tuple<Type, std::string, std::string>>* vars_table) {
	int i = 0;
	while (i < prog.StatementsList.size()) {
		Statement* statement = dynamic_cast<Statement*>(prog.StatementsList[i].get());
		this->evaluate_statement(statement, vars_table);
		i++;
	}
}



void Interpreter::evaluate_statement(Statement* statement, std::vector<std::tuple<Type, std::string, std::string>>* vars_table) {
	if (statement == NULL) {
		this->error("Expected a statement", 3411);
	}
	else {
		std::string stmt_type = statement->get_type();

		if (stmt_type == "alloc") {
			Allocation* alloc = dynamic_cast<Allocation*>(statement);
			Interpreter::allocate_var(*alloc, vars_table);
		}
		else if (stmt_type == "def") {
			Definition* def = dynamic_cast<Definition*>(statement);
			Interpreter::define_function(*def);
		}
		else if (stmt_type == "assign") {
			Assignment* assign = dynamic_cast<Assignment*>(statement);
			Interpreter::evaluate_assignment(*assign, vars_table);
		}
		else if (stmt_type == "ite") {
			IfThenElse* if_block = dynamic_cast<IfThenElse*>(statement);
			Expression* condition = dynamic_cast<Expression*>(if_block->get_condition().get());
			if (to_bool(std::get<1>(this->evaluate_expression(condition, vars_table)))) {
				StatementBlock* branch = dynamic_cast<StatementBlock*>(if_block->get_if_branch().get());
				this->execute_statements(*branch, vars_table);
			}
			else {
				StatementBlock* branch = dynamic_cast<StatementBlock*>(if_block->get_else_branch().get());
				if (branch) {
					this->execute_statements(*branch, vars_table);
				}
			}
		}
		else if (stmt_type == "while") {
			// parse a while loop
			WhileLoop* while_loop = dynamic_cast<WhileLoop*>(statement);
			Expression* condition = dynamic_cast<Expression*>(while_loop->get_condition().get());
			StatementBlock* branch = dynamic_cast<StatementBlock*>(while_loop->get_branch().get());
			bool loop = to_bool(std::get<1>(this->evaluate_expression(condition, vars_table)));
			while (loop) {
				this->execute_statements(*branch, vars_table);
				loop = to_bool(std::get<1>(this->evaluate_expression(condition, vars_table)));
			}
		}
		else if (stmt_type == "return") {
			// return statements are inappropriate here
			this->error("A return statement is inappopriate here", 3412);
		}
		else if (stmt_type == "call") {
			Call* call = dynamic_cast<Call*>(statement);
			// check to see if it is a pre-defined function
			if (call->get_func_name() == "print") {
				// do an of the print
				// there is probably a better way to call our predefined functions, but this is ok for now
				if (call->get_args_size() != 1) {
					this->error("'print' takes only one argument!'", 3140);
				}
				else {
					std::shared_ptr<Expression> _arg = call->get_arg(0);
					if (_arg->getExpType() == "literal") {
						Literal* arg = dynamic_cast<Literal*>(_arg.get());
						std::cout << arg->get_value() << std::endl;
					}
					else if (_arg->getExpType() == "LValue") {
						LValue* arg = dynamic_cast<LValue*>(_arg.get());
						std::cout << this->get_var_value(arg->getValue(), vars_table) << std::endl;
					}
					else if (_arg->getExpType() == "binary") {
						Binary* arg = dynamic_cast<Binary*>(_arg.get());
						std::cout << std::get<1>(this->evaluate_expression(arg, vars_table)) << std::endl;
					}
					else if (_arg->getExpType() == "value_returning") {
						ValueReturningFunctionCall* val_ret = dynamic_cast<ValueReturningFunctionCall*>(_arg.get());
						std::tuple<Type, std::string> _exp_tuple = this->evaluate_value_returning_function(*val_ret, vars_table);
						std::cout << std::get<1>(_exp_tuple) << std::endl;
					}
				}
			}
			// otherwise, if we defined it,
			else {
				// call the function normally
				this->evaluate_void_function(*call, vars_table);
			}
		}
	}
}



Definition Interpreter::get_definition(std::string func_to_find) {
	std::vector<Definition>::iterator func_it = this->functions_table.begin();
	bool found = false;
	while (func_it != this->functions_table.end() && !found) {
		// get our function's name as an LValue pointer -- because function names are considered LValues
		// maybe they should be considered literals or some other class, as they aren't really LValues, but that is to change later
		LValue* table_name = dynamic_cast<LValue*>(func_it->get_name().get());
		// if the two values are the same
		if (func_to_find == table_name->getValue()) {
			found = true;
		}
		// if they are not
		else {
			// increment func_it
			func_it++;
		}
	}
	if (found) {
		// We have found the function
		return *func_it;
	}
	else {
		this->error("Could not find a definition for the function referenced", 3034);
	}
}

void Interpreter::evaluate_void_function(Call func_to_evaluate, std::vector<std::tuple<Type, std::string, std::string>>* parent_vars_table) {
	// get the definition for the function we want to evaluate
	Definition func_def = this->get_definition(func_to_evaluate.get_func_name());

	std::vector<std::tuple<Type, std::string, std::string>> local_vars;

	// create instances of all our local function variables
	if (func_to_evaluate.get_args_size() == func_def.get_args().size()) {
		for (int i = 0; i < func_def.get_args().size(); i++) {
			Allocation* current_arg = dynamic_cast<Allocation*>(func_def.get_args()[i].get());
			Expression* arg = dynamic_cast<Expression*>(func_to_evaluate.get_arg(i).get());
			std::tuple<Type, std::string> argv;
			if (arg->getExpType() == "LValue") {
				argv = this->evaluate_expression(arg, parent_vars_table);	// if we pass a variable into a function, we need to be able to evaluate it
			}
			else {
				argv = this->evaluate_expression(arg, &local_vars);	// if it's not a variable, we don't need the table
			}
			if (std::get<0>(argv) == current_arg->getVarType()) {
				local_vars.push_back(std::make_tuple(current_arg->getVarType(), current_arg->getVarName(), std::get<1>(argv)));
			}
			else {
				this->error("Argument to function is of improper type, must be '" + get_string_from_type(current_arg->getVarType()) + "', not '" \
					+ get_string_from_type(std::get<0>(argv)) + "'!", 1141);
			}
		}
	}
	else {
		this->error("Number of arguments in function call is not equal to number in definition!", 3140);
	}

	this->execute_statements(*dynamic_cast<StatementBlock*>(func_def.get_procedure().get()), &local_vars);

	return;
}

std::tuple<Type, std::string> Interpreter::evaluate_value_returning_function(ValueReturningFunctionCall func_to_evaluate, std::vector<std::tuple<Type, std::string, std::string>>* parent_vars_table) {
	// Get the definition for the function we want to evaluate
	Definition func_def = this->get_definition(func_to_evaluate.get_func_name());

	std::vector<std::tuple<Type, std::string, std::string>> local_vars;

	// create instances of all our local function variables
	if (func_to_evaluate.get_args_size() == func_def.get_args().size()) {
		for (int i = 0; i < func_def.get_args().size(); i++) {
			Allocation* current_arg = dynamic_cast<Allocation*>(func_def.get_args()[i].get());
			Expression* arg = dynamic_cast<Expression*>(func_to_evaluate.get_arg(i).get());
			std::tuple<Type, std::string> argv;
			if (arg->getExpType() == "LValue") {
				argv = this->evaluate_expression(arg, parent_vars_table);
			}
			else {
				argv = this->evaluate_expression(arg, &local_vars);
			}
			if (std::get<0>(argv) == current_arg->getVarType()) {
				local_vars.push_back(std::make_tuple(current_arg->getVarType(), current_arg->getVarName(), std::get<1>(argv)));
			}
			else {
				this->error("Argument to function is of improper type, must be '" + get_string_from_type(current_arg->getVarType()) + "', not '" \
					+ get_string_from_type(std::get<0>(argv)) + "'!", 1141);
			}
		}
	}
	else {
		this->error("Number of arguments in function call is not equal to number in definition!", 3140);
	}

	int i = 0;
	StatementBlock* procedure = dynamic_cast<StatementBlock*>(func_def.get_procedure().get());
	Expression* return_exp = new Expression();
	while (i < procedure->StatementsList.size()) {
		Statement* statement = dynamic_cast<Statement*>(procedure->StatementsList[i].get());
		if (statement->get_type() != "return") {
			this->evaluate_statement(statement, &local_vars);
		}
		else {
			ReturnStatement* return_stmt = dynamic_cast<ReturnStatement*>(statement);
			return_exp = dynamic_cast<Expression*>(return_stmt->get_return_exp().get());
		}
		i++;
	}
	std::tuple<Type, std::string> evaluated_return_exp = this->evaluate_expression(return_exp, &local_vars);
	return evaluated_return_exp;
}



void Interpreter::evaluate_assignment(Assignment assign, std::vector<std::tuple<Type, std::string, std::string>>* vars_table) {
	LValue lvalue = assign.getLvalueName();
	Expression* RValue = dynamic_cast<Expression*>(assign.getRValue().get());

	this->set_var_value(lvalue.getValue(), std::get<1>(this->evaluate_expression(RValue, vars_table)), vars_table);
	return;
}

std::tuple<Type, std::string> Interpreter::evaluate_expression(Expression* expr, std::vector<std::tuple<Type, std::string, std::string>>* vars_table) {
	if (expr->getExpType() == "literal") {
		// evaluate a literal
		Literal* our_val = dynamic_cast<Literal*>(expr);
		return std::make_tuple(our_val->get_type(), our_val->get_value());
	}
	else if (expr->getExpType() == "LValue") {
		LValue* _var = dynamic_cast<LValue*>(expr);
		for (std::vector<std::tuple<Type, std::string, std::string>>::iterator var = vars_table->begin(); var != vars_table->end(); ++var) {
			if (_var->getValue() == std::get<1>(*var)) {
				return std::make_tuple(std::get<0>(*var), std::get<2>(*var));
			}
		}
	}
	else if (expr->getExpType() == "binary") {
		// cast the expression to a binary so we have access to our functions
		Binary* binary_exp = dynamic_cast<Binary*>(expr);

		// Now, create our left side, right side, and operator objects
		Expression* left_exp = dynamic_cast<Expression*>(binary_exp->get_left().get());
		Expression* right_exp = dynamic_cast<Expression*>(binary_exp->get_right().get());
		exp_operator op = binary_exp->get_operator();

		std::tuple<Type, std::string> left_exp_result = this->evaluate_expression(left_exp, vars_table);
		std::tuple<Type, std::string> right_exp_result = this->evaluate_expression(right_exp, vars_table);

		Type left_t = std::get<0>(left_exp_result);
		Type right_t = std::get<0>(right_exp_result);

		// First, make sure our expressions are of the same type
		if (left_t == right_t) {
			// Evaluate the result of our expression by discerning what operation we are actually trying to do
			if (op == PLUS) {
				// Since the types are the same, we only need to test one of them
				if (left_t == INT) {
					int sum = std::stoi(std::get<1>(left_exp_result)) + std::stoi(std::get<1>(right_exp_result));
					return std::make_tuple(INT, std::to_string(sum));
				}
				else if (left_t == FLOAT) {
					float sum = std::stof(std::get<1>(left_exp_result)) + std::stof(std::get<1>(right_exp_result));
					return std::make_tuple(FLOAT, std::to_string(sum));
				}
				else if (left_t == STRING) {
					std::string concatenated = std::get<1>(left_exp_result) + std::get<1>(right_exp_result);
					return std::make_tuple(STRING, concatenated);
				}
				else {
					this->error("Addition cannot be performed on expressions of this type", 1123);
				}
			}
			else if (op == MINUS) {
				if (left_t == INT) {
					int diff = std::stoi(std::get<1>(left_exp_result)) - std::stoi(std::get<1>(right_exp_result));
					return std::make_tuple(INT, std::to_string(diff));
				}
				else if (left_t == FLOAT) {
					float diff = std::stof(std::get<1>(left_exp_result)) - std::stof(std::get<1>(right_exp_result));
					return std::make_tuple(FLOAT, std::to_string(diff));
				}
				else {
					this->error("Subtraction cannot be performed on expressions of this type", 1123);
				}
			}
			else if (op == MULT) {
				if (left_t == INT) {
					int product = std::stoi(std::get<1>(left_exp_result)) * std::stoi(std::get<1>(right_exp_result));
					return std::make_tuple(INT, std::to_string(product));
				}
				else if (left_t == FLOAT) {
					float product = std::stof(std::get<1>(left_exp_result)) * std::stof(std::get<1>(right_exp_result));
					return std::make_tuple(FLOAT, std::to_string(product));
				}
				// Add support for string repitition?
				else {
					this->error("Multiplication cannot be performed on expressions of this type", 1123);
				}
			}
			else if (op == DIV) {
				if (left_t == INT) {
					int quotient = std::stoi(std::get<1>(left_exp_result)) / std::stoi(std::get<1>(right_exp_result));
					return std::make_tuple(INT, std::to_string(quotient));
				}
				else if (left_t == FLOAT) {
					float quotient = std::stof(std::get<1>(left_exp_result)) / std::stof(std::get<1>(right_exp_result));
					return std::make_tuple(FLOAT, std::to_string(quotient));
				}
				else {
					this->error("Division cannot be performed on expressions of this type", 1123);
				}
			}
			else if (op == GREATER) {
				if (left_t == INT) {
					int _left = std::stoi(std::get<1>(left_exp_result));
					int _right = std::stoi(std::get<1>(right_exp_result));
					bool greater = _left > _right;
					return std::make_tuple(BOOL, bool_string(greater));
				}
				else if (left_t == FLOAT) {
					float _left = std::stof(std::get<1>(left_exp_result));
					float _right = std::stof(std::get<1>(right_exp_result));
					bool greater = _left > _right;
					return std::make_tuple(BOOL, bool_string(greater));
				}
				else {
					this->error("Comparison cannot be performed on expressions of this type", 1123);
				}
			}
			else if (op == GREATER_OR_EQUAL) {
				if (left_t == INT) {
					int _left = std::stoi(std::get<1>(left_exp_result));
					int _right = std::stoi(std::get<1>(right_exp_result));
					bool greater = _left >= _right;
					return std::make_tuple(BOOL, bool_string(greater));
				}
				else if (left_t == FLOAT) {
					float _left = std::stof(std::get<1>(left_exp_result));
					float _right = std::stof(std::get<1>(right_exp_result));
					bool greater = _left >= _right;
					return std::make_tuple(BOOL, bool_string(greater));
				}
				else {
					this->error("Comparison cannot be performed on expressions of this type", 1123);
				}
			}
			else if (op == LESS) {
				if (left_t == INT) {
					int _left = std::stoi(std::get<1>(left_exp_result));
					int _right = std::stoi(std::get<1>(right_exp_result));
					bool greater = _left < _right;
					return std::make_tuple(BOOL, bool_string(greater));
				}
				else if (left_t == FLOAT) {
					float _left = std::stof(std::get<1>(left_exp_result));
					float _right = std::stof(std::get<1>(right_exp_result));
					bool greater = _left < _right;
					return std::make_tuple(BOOL, bool_string(greater));
				}
				else {
					this->error("Comparison cannot be performed on expressions of this type", 1123);
				}
			}
			else if (op == LESS_OR_EQUAL) {
				if (left_t == INT) {
					int _left = std::stoi(std::get<1>(left_exp_result));
					int _right = std::stoi(std::get<1>(right_exp_result));
					bool greater = _left <= _right;
					return std::make_tuple(BOOL, bool_string(greater));
				}
				else if (left_t == FLOAT) {
					float _left = std::stof(std::get<1>(left_exp_result));
					float _right = std::stof(std::get<1>(right_exp_result));
					bool greater = _left <= _right;
					return std::make_tuple(BOOL, bool_string(greater));
				}
				else {
					this->error("Comparison cannot be performed on expressions of this type", 1123);
				}
			}
			else if (op == EQUAL) {
				bool equals = std::get<1>(left_exp_result) == std::get<1>(right_exp_result);
				return std::make_tuple(BOOL, bool_string(equals));
			}
			else if (op == NOT_EQUAL) {
				bool not_equals = std::get<1>(left_exp_result) != std::get<1>(right_exp_result);
				return std::make_tuple(BOOL, bool_string(not_equals));
			}
			else if (op == AND) {
				bool and = to_bool(std::get<1>(left_exp_result)) && to_bool(std::get<1>(right_exp_result));
				return std::make_tuple(BOOL, bool_string(and));					
			}
			else if (op == OR) {
				bool or = to_bool(std::get<1>(left_exp_result)) || to_bool(std::get<1>(right_exp_result));
				return std::make_tuple(BOOL, bool_string(or));
			}
			else {
				this->error("Unrecognized operator in binary expression!", 3999);
			}
		}
		// Expressions are not of the same type
		else {
			this->error("Expressions in a binary expression must have the same type.", 3101);
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
				this->error("'input' only takes one argument!", 3140);
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
						this->error("Argument must be of type 'string'", 1141);
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
				this->error("'stoi' only takes one argument!", 3140);
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
							this->error("Cannot convert '" + to_convert + "' to type 'int'!", 2110);
						}
					}
				} 
				else if (_arg->getExpType() == "LValue") {
					LValue* var = dynamic_cast<LValue*>(_arg.get());
					std::tuple<Type, std::string> evaluated = this->evaluate_expression(var, vars_table);
					if (std::get<0>(evaluated)) {
						to_convert = std::get<1>(evaluated);
						try {
							converted = std::stoi(to_convert);
							return std::make_tuple(INT, std::to_string(converted));
						}
						catch (std::invalid_argument& ia) {
							std::cerr << "\n" << ia.what() << std::endl;
							this->error("Cannot convert '" + to_convert + "' to type 'int'!", 2110);
						}
					}
					else {
						this->error("Cannot convert expressions of this type with 'stoi'!", 2110);
					}
				}
				else {
					this->error("Cannot convert expressions of this type with 'stoi'!", 2110);
				}
			}
		}
		// then it is one we defined
		else {
			std::tuple<Type, std::string> evaluated_return_expression = this->evaluate_value_returning_function(*call, vars_table);
			return evaluated_return_expression;
		}
	}
	else if (expr->getExpType() == "var") {

	}
}


template<typename T> T Interpreter::eval_sum(T const& left, T const& right) {
	return T + T;
}

/***************	CONSTRUCTORS & DESTRUCTOR	***************/

Interpreter::Interpreter()
{
}


Interpreter::~Interpreter()
{
}
