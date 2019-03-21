/*

SIN Toolchain
Functions.cpp
Copyright 2019 Riley Lannon

This file contains the implementation of Compiler::define(...), Compiler::call(...), and Compiler::return_value(...) the methods that exclusively relate to functions.

*/

#include "Compiler.h"


// define a function
std::stringstream Compiler::define(Definition definition_statement) {
	/*

	Creates a definition for a function found in 'definition_statement'

	*/

	size_t stack_frame_base_offset = this->stack_offset;	// keeps track of where the stack offset was when we started compiling the definition; this is where we must return to in the return statement

	// first, add the function to the symbol table
	std::shared_ptr<Expression> func_name_expr = definition_statement.get_name();
	LValue* lvalue_ptr = dynamic_cast<LValue*>(func_name_expr.get());
	std::string func_name = lvalue_ptr->getValue();
	Type return_type = definition_statement.get_return_type();

	// create a stringstream for our function data
	std::stringstream function_asm;

	// function definitions have to be in the global scope
	if (current_scope_name == "global" && current_scope == 0) {
		this->symbol_table.insert(func_name, return_type, "global", 0, NONE, {}, true, definition_statement.get_args(), definition_statement.get_line_number());
		// next, we will make sure that all of the function code gets written to a stringstream; once we have gone all the way through the AST, we will write our functions as subroutines to the end of the file

		// create a label for the function name
		function_asm << func_name << ":" << std::endl;

		std::vector<std::shared_ptr<Statement>> func_args = definition_statement.get_args();
		bool must_be_default = false;	// as soon as we have one default argument, the rest must also be default

		for (std::vector<std::shared_ptr<Statement>>::iterator arg_iter = func_args.begin(); arg_iter != func_args.end(); arg_iter++) {

			stmt_type arg_type = arg_iter->get()->get_statement_type();	// get the argument type

			if (arg_type == ALLOCATION) {
				// get the allocation statement for the argumnet
				Allocation* arg_alloc = dynamic_cast<Allocation*>(arg_iter->get());

				// check to see if the symbol is a default argument
				if (arg_alloc->get_initial_value()->get_expression_type() != NONE) {	// if it is a default, make sure we set must_be_default
					must_be_default = true;
				}
				else if (must_be_default) {	// if it's not default, but must_be_default is set, throw an exception
					throw CompilerException("Default arguments must be declared last in an argument list", 0, definition_statement.get_line_number());
				}

				// add the variable to the symbol table, giving it the function's scope name at scope level 1
				// make sure we set "was_initialized" in the definition so that when the function is compiled, we don't get a "referenced before assignment" error; the definition assumes all arguments will be passed, because the call will throw an error if they aren't
				Symbol argument_symbol(arg_alloc->get_var_name(), arg_alloc->get_var_type(), func_name, 1, arg_alloc->get_var_subtype(), arg_alloc->get_qualities(), true, {}, arg_alloc->get_array_length());
				argument_symbol.stack_offset = this->stack_offset;	// update the stack offset before pushing
				this->symbol_table.insert(argument_symbol, definition_statement.get_line_number());	// this variable is only accessible inside this function's scope

				/*

				Note the parameters will already be pushed when the function is called, so we don't need to worry about that here

				However, we _do_ need to increment the position of the the stack offset counter according to what was pushed so we know how to navigate through our local variables, which will be on the stack for the function's lifetime
				The only reason we need to know these offsets is so the compiler knows where its data members are during the compilation of the function. It cannot access local variables above its stack frame base.

				Note we are not actually incrementing the pointer, we are just incrementing the variable that the compiler itself uses to track where we are in the stack

				*/

				if (arg_alloc->get_var_type() == ARRAY) {
					// TODO: implement arrays
				}
				else if (arg_alloc->get_var_type() == STRUCT) {
					// TODO: implement structs
				}
				else if (arg_alloc->get_var_type() == STRING) {
					// One word for length, one for address
					this->stack_offset += 2;
				}
				else {
					// all other types are one word
					this->stack_offset += 1;
				}
			}
			else {
				// currently, only alloc statements are allowed in function definitions
				throw CompilerException("Only allocation statements are allowed in function parameter definitions.", 0, definition_statement.get_line_number());
			}
		}

		// by this point, all of our function parameters are on the stack, in our symbol table, and our stack offset pointer tells us how many; as such, we can call the compile routine on our AST, as all of the functions for compilation will be able to handle scopes other than global
		StatementBlock function_procedure = *definition_statement.get_procedure().get();	// get the AST

		// update the current scope
		this->current_scope_name = func_name;
		this->current_scope = 1;

		// if we don't have an empty procedure, compile it
		if (function_procedure.statements_list.size() > 0) {
			function_asm << this->compile_to_sinasm(function_procedure, 1, func_name, this->stack_offset, stack_frame_base_offset).str();	// compile it; be sure to pass in the previous offset so the return statement unwinds the stack correctly
		}
		else {
			compiler_warning("Empty function definition", definition_statement.get_line_number());
		}

		// The 'return' statement at the end of the function is responsible for unwinding the stack, so we don't need to do that here

		// as such, we can now return from the subroutine
		function_asm << "\t" << "rts" << std::endl;

		// return our scope name to "global", the current scope to 0, and return the function assembly
		this->current_scope_name = "global";
		this->current_scope = 0;
		return function_asm;
	}
	else {
		throw CompilerException("Function definitions must be in the global scope.", 0, definition_statement.get_line_number());
	}
}


// call a function
std::stringstream Compiler::call(Call call_statement, size_t max_offset) {
	/*

	Compile a function call, held in 'call_statement'. We will use stack_offset and max_offset to hold the stack offset at the time the function is called and the maximum offset for local variables, respectively. stack_offset and max_offset are both default parameters, set to nullptr and 0, respectively, and serve to track stack locations for using local variables. If no local variables have been allocated at call time, they will both be 0

	*/

	// create a stream for the assembly generated by the function call
	std::stringstream call_ss;
	Symbol func_to_call_symbol;

	// get the symbol of our function
	if (this->symbol_table.is_in_symbol_table(call_statement.get_func_name(), "global")) {
		func_to_call_symbol = *(this->symbol_table.lookup(call_statement.get_func_name(), "global"));
	}
	else {
		throw CompilerException("Cannot locate function in symbol table (perhaps you didn't include the right file?)", 0, call_statement.get_line_number());
	}

	std::vector<std::shared_ptr<Statement>> formal_parameters = func_to_call_symbol.formal_parameters;

	call_ss << this->move_sp_to_target_address(max_offset).str();
	size_t function_stack_frame_base = this->stack_offset;

	// if we don't have any arguments, just write a jsr
	if (call_statement.get_args_size() == 0 && func_to_call_symbol.formal_parameters.size() == 0) {
		call_ss << "\t" << "jsr " << call_statement.get_func_name() << std::endl;
	}
	else {
		// check to make sure the number of args in the call is not greater than the number expected
		if (call_statement.get_args_size() > func_to_call_symbol.formal_parameters.size()) {
			// if we have too many, throw an exception
			throw CompilerException("Too many arguments in function call; expected " + std::to_string(func_to_call_symbol.formal_parameters.size()) + ", got " + std::to_string(call_statement.get_args_size()), 0, call_statement.get_line_number());
		}

		// add a push statement for each argument we do have
		for (size_t i = 0; i < call_statement.get_args_size(); i++) {
			// get the expression for the argument
			std::shared_ptr<Expression> argument = call_statement.get_arg(i);
			Type argument_type = this->get_expression_data_type(argument);

			// and the expression for the formal parameter
			Allocation* formal_parameter = dynamic_cast<Allocation*>(formal_parameters[i].get());
			Type formal_type = formal_parameter->get_var_type();

			// now, ensure the types match
			if (argument_type == formal_type) {
				// fetch the value of the argument we are currently on
				call_ss << this->fetch_value(argument, call_statement.get_line_number(), max_offset).str();

				// we have fetched the appropriate value; push based on its type
				if (formal_type == INT || formal_type == FLOAT || formal_type == BOOL || formal_type == PTR) {
					call_ss << "\t" << "tax" << std::endl;
					call_ss << this->move_sp_to_target_address(max_offset).str();
					call_ss << "\t" << "pha" << std::endl;
					this->stack_offset += 1;	// for some reason, this line breaks it -- but it works fine without it
					max_offset += 1;
				}
				else if (formal_type == STRING) {
					call_ss << "\t" << "tax" << "\n\t" << "tba" << "\n\t" << "tay" << std::endl;
					call_ss << this->move_sp_to_target_address(max_offset).str();
					call_ss << "\t" << "tya" << "\n\t" << "tab" << "\n\t" << "txa" << std::endl;
					// strings push length (A), then address (B)
					call_ss << "\t" << "pha" << std::endl;
					call_ss << "\t" << "phb" << std::endl;
					this->stack_offset += 2;	// breaks it
					max_offset += 2;
				}
				else if (formal_type == ARRAY) {
					// todo: implement arrays in function calls (as arguments)
				}
				else if (formal_type == STRUCT) {
					// todo: implement structs in function calls (as arguments)
				}
				else {
					throw CompilerException("Could not resolve function parameter data type", 0, call_statement.get_line_number());
				}
			}
			else {
				// if the don't, throw an exception
				throw CompilerException("Type match error: argument supplied does not match the type of the formal parameter", 0, call_statement.get_line_number());
			}
		}

		// Check to see whether the number of arguments passed into the function match the number of arguments we expected
		if (call_statement.get_args_size() < func_to_call_symbol.formal_parameters.size()) {
			// start pushing the default values, starting at the index "call_statement.get_args_size
			for (size_t i = call_statement.get_args_size(); i < func_to_call_symbol.formal_parameters.size(); i++) {
				if (func_to_call_symbol.formal_parameters[i]->get_statement_type() != ALLOCATION) {
					throw CompilerException("Expected allocation statement in parameter list", 0, call_statement.get_line_number());
				}
				else {
					Allocation* arg_allocation = dynamic_cast<Allocation*>(func_to_call_symbol.formal_parameters[i].get());

					// make sure we actually have a value -- if the user simply hasn't supplied enough values, we must throw an exception
					if (arg_allocation->get_initial_value()->get_expression_type() == EXPRESSION_GENERAL) {	// it is 'EXPRESSION_GENERAL' if it wasn't initialized to anything, which it won't be if there wasn't a default value given
						throw CompilerException("Not enough arguments supplied in call to '" + call_statement.get_func_name() +
							"'; expected '" + arg_allocation->get_var_name() + "'", 0, call_statement.get_line_number());
					}
					// if we have a default value, proceed
					else {
						std::shared_ptr<Expression> arg_to_push = arg_allocation->get_initial_value();
						Type var_type = this->get_expression_data_type(arg_to_push);

						// fetch the value
						call_ss << this->fetch_value(arg_to_push, call_statement.get_line_number(), max_offset).str();

						// now, push the value -- but it depends on type!
						if (var_type == INT || var_type == FLOAT || var_type == BOOL || var_type == PTR) {
							call_ss << "\t" << "pha" << std::endl;
							//this->stack_offset += 1;
						}
						else if (var_type == STRING) {
							// strings push length (A) then address (B)
							call_ss << "\t" << "pha" << "\n\t" << "phb" << std::endl;
							//this->stack_offset += 2;
						}
						else if (var_type == ARRAY) {
							// todo: implement arrays as function parameters
						}
						else if (var_type == STRUCT) {
							// todo: implement structs
						}
						else {
							throw CompilerException("Could not resolve function parameter data type", 0, call_statement.get_line_number());
						}
					}
				}
			}
		}

		// finally, use jsr
		call_ss << "\t" << "jsr " << call_statement.get_func_name() << std::endl;
	}

	/*

	Move the stack pointer to where it will be based on the return type; if this is INT, FLOAT, BOOL, STRING, PTR, VOID, or NONE, we can move it back to the base of the stack frame because these types can be stored in registers.
	If the return type is ARRAY or STRUCT, we will move it to (base - sizeof(return value)) so that it can be popped off the stack properly
	Note that because of this, calling a function that returns an object on the stack can be risky, as the function will not return to the base of the stack frame; this may result in your stack frame increasing by the size of an object you aren't using (e.g., if you immediately allocate a local variable)

	*/

	if (func_to_call_symbol.type == INT || func_to_call_symbol.type == FLOAT || func_to_call_symbol.type == BOOL || func_to_call_symbol.type == PTR || func_to_call_symbol.type == STRING || func_to_call_symbol.type == VOID || func_to_call_symbol.type == NONE) {
		this->stack_offset = function_stack_frame_base;
	}
	else if (func_to_call_symbol.type == ARRAY) {
		// get the array size; note that only integral types are allowed (structs and arrays are forbidden; arrays of pointers to such types are allowed)
		size_t subtype_size;
		if (func_to_call_symbol.sub_type == STRING) {
			subtype_size = 2;
		}
		else {
			subtype_size = 1;
		}

		// get the size of the object we are returning; this is the array size (number of elements) multiplied by the size of each element
		this->stack_offset = (function_stack_frame_base - (func_to_call_symbol.array_length * subtype_size));
	}

	// finally, return the call_ss
	return call_ss;
}


// return a value from a function
std::stringstream Compiler::return_value(ReturnStatement return_statement, size_t previous_offset, unsigned int line_number)
{
	/*

	If the value can be stored in registers, it will be; otherwise, it will be pushed to the stack (arrays and structs)
	The return function is responsible for unwinding the stack

	*/

	std::stringstream return_ss;	// the stringstream that contains our compiled code

	// get the return type
	Type return_type = this->get_expression_data_type(return_statement.get_return_exp());

	// Some types can be loaded into registers
	if (return_type == INT || return_type == STRING || return_type == BOOL || return_type == FLOAT) {
		return_ss << this->fetch_value(return_statement.get_return_exp()).str();	// get the expression to return

		return_ss << "\t" << "tax" << "\n\t" << "tba" << "\n\t" << "tay" << std::endl;	// move A and B into X and Y to preserve their values
		return_ss << this->move_sp_to_target_address(previous_offset).str();	// this allows us to use A to unwind the stack, if we need
		return_ss << "\t" << "tya" << "\n\t" << "tab" << "\n\t" << "txa" << std::endl;
	}
	else if (return_type == VOID) {
		return_ss << this->move_sp_to_target_address(previous_offset).str();
	}
	else if (return_type == ARRAY) {
		// TODO: use stack to return an array
	}
	else if (return_type == STRUCT) {
		// TODO: use stack to return struct type once structs are implemented
	}
	else {
		throw CompilerException("Cannot return an expression of the specified type", 0, line_number);
	}

	return return_ss;
}
