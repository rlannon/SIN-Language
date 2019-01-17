#include "Compiler.h"


// given a StatementBlock object, get statements from it
std::shared_ptr<Statement> Compiler::get_next_statement(StatementBlock AST)
{
	this->AST_index += 1;	// increment AST_index by one
	std::shared_ptr<Statement> stmt_ptr = AST.statements_list[AST_index];	// get the shared_ptr<Statement> at the correct position
	return stmt_ptr; // return the shared_ptr<Statement>
}

std::shared_ptr<Statement> Compiler::get_current_statement(StatementBlock AST)
{
	return AST.statements_list[AST_index];	// return the shared_ptr<Statement> at the current position of the AST index
}



void Compiler::produce_binary_tree(Binary bin_exp) {
	Binary current_tree = bin_exp;
	Expression* left_exp = current_tree.get_left().get();
	Expression* right_exp = current_tree.get_right().get();

	// TODO: devise binary tree algorithm
}



void Compiler::include_file(Include include_statement)
{
	/*
	
	Include statements take the included file and add its code in the reserved area in memory
		- If the file is a .sina file, its assembly is added
		- If the file is a .sinc file, it is disassembled before being added
		- If the file is a .sin file, it is compiled and its assembly added

	Handling included files and how they relate to the main executable is all handled by the linker. The include statement does not go into the assembler; it adds the object files to a vector to pass to the linker.

	*/

	std::string to_include = include_statement.get_filename();
	size_t extension_position = to_include.find(".");
	std::string extension = to_include.substr(extension_position);
	std::string filename_no_extension = to_include.substr(0, extension_position);

	// first, check to make sure the filename is NOT in our list of dependencies already
	bool already_included = false;
	std::vector<std::string>::iterator included_libraries_iter = this->library_names->begin();

	while (!already_included && (included_libraries_iter != this->library_names->end())) {
		if (filename_no_extension == *included_libraries_iter) {
			already_included = true;
		}
		else {
			included_libraries_iter++;
		}
	}

	if (already_included) {
		// write a warning, but don't throw an exception
		std::cerr << "Warning: duplicate include found! Skipping..." << std::endl;
		return;
	}
	else {
		// if the extension is '.sinc', we will simply add that; we don't need to compile or assemble
		if (extension == ".sinc") {
			// push back the file name
			object_file_names->push_back(to_include);
		}
		else if (extension == ".sina") {
			// now, open the compiled include file as an istream object
			std::ifstream compiled_include;
			compiled_include.open(filename_no_extension + ".sina", std::ios::in);
			if (compiled_include.is_open()) {
				// then turn it into an object file
				Assembler assemble(compiled_include, this->_wordsize);
				compiled_include.close();

				// finally, add the object file's name to our vector
				this->object_file_names->push_back(filename_no_extension + ".sinc");
			}
			// if we cannot open the .sina file
			else {
				throw std::exception(("**** Compiled the include file '" + filename_no_extension + ".sina" + "', but could not open the compiled version for assembly.").c_str());
			}
		}
		// if the have another SIN file, we need to compile it and then push it back
		else if (extension == ".sin") {
			// open the included file
			std::ifstream included_sin_file;
			included_sin_file.open(to_include, std::ios::in);

			if (included_sin_file.is_open()) {
				// compile the file
				Compiler* include_compiler;

				// if we are compiling "builtins", don't include "builtins"
				if (filename_no_extension == "builtins") {
					include_compiler = new Compiler(included_sin_file, this->_wordsize, this->object_file_names, this->library_names, false);
				}
				else {
					include_compiler = new Compiler(included_sin_file, this->_wordsize, this->object_file_names, this->library_names);
				}

				include_compiler->produce_sina_file(filename_no_extension + ".sina");

				// iterate through the symbols in that file and add them to our symbol table
				for (std::vector<Symbol>::iterator it = include_compiler->symbol_table.symbols.begin(); it != include_compiler->symbol_table.symbols.end(); it++) {
					this->symbol_table.insert(it->name, it->type, it->scope_name, it->scope_level, it->quality, it->defined, it->formal_parameters);
				}

				// now, open the compiled include file as an istream object
				std::ifstream compiled_include;
				compiled_include.open(filename_no_extension + ".sina", std::ios::in);
				if (compiled_include.is_open()) {
					// then turn it into an object file
					Assembler assemble(compiled_include, this->_wordsize);
					compiled_include.close();

					// finally, add the object file's name to our vector
					this->object_file_names->push_back(filename_no_extension + ".sinc");
				}
				// if we cannot open the .sina file to read
				else {
					throw std::exception(("**** Compiled the include file '" + filename_no_extension + ".sina" + "', but could not open the compiled version for assembly.").c_str());
				}

				included_sin_file.close();

				// we allocated memory for include_compiler, so we must now delete it
				delete include_compiler;
			}
			// if we cannot open the .sin file
			else {
				throw std::exception(("**** Could not open included file '" + to_include + "'!").c_str());
			}
		}

		// after including, add the filename (no extension) to our libraries list so we don't include it again
		this->library_names->push_back(filename_no_extension);
	}
}

std::stringstream Compiler::fetch_value(std::shared_ptr<Expression> to_fetch, unsigned int line_number, size_t * stack_offset, size_t max_offset)
{
	/*
	
	Given an expression, this function will produce ASM that, when run, will get the desired value in the A register. For example, if we have the statement
		let myVar = anotherVar;
	we can use this function to fetch the contents of 'anotherVar', producing assembly that will load its value in the A register; we can then finish compiling the statement, in our respective function. This method simply allows us to hide away the details of how this is done, as well as removing code from functions and making the compiler more maintainable.
	
	*/

	// the stringstream that will contain our assembly code
	std::stringstream fetch_ss;

	// test the expression type to determine how to write the assembly for fetching it

	// the simplest one is a literal
	if (to_fetch->get_expression_type() == LITERAL) {
		Literal* literal_expression = dynamic_cast<Literal*>(to_fetch.get());

		// different data types will require slightly different methods for loading
		if (literal_expression->get_type() == INT) {
			// int types just need to write a loada instruction
			fetch_ss << "\t" << "loada #$" << literal_expression->get_value() << std::endl;
		}
		else if (literal_expression->get_type() == BOOL) {
			// bool types are also easy to write; any nonzero integer is true
			int bool_expression_as_int;
			if (literal_expression->get_value() == "True") {
				bool_expression_as_int = 1;
			}
			else if (literal_expression->get_value() == "False") {
				bool_expression_as_int = 0;
			}
			else {
				// if it isn't "True" or "False", throw an error
				throw std::exception(("Expected 'True' or 'False' as boolean literal value (case matter!) (line " + literal_expression->get_value() + ")").c_str());
			}

			fetch_ss << "\t" << "loada #$" << std::hex << bool_expression_as_int << std::endl;
		}
		else if (literal_expression->get_expression_type() == FLOAT) {
			// copy the bits of the float value into an integer
			float literal_value = std::stof(literal_expression->get_value());
			int converted_value;
			memcpy(&converted_value, &literal_value, sizeof(int));

			// write the integer value (in hex notation) into a
			fetch_ss << "\t" << "loada #$" << std::hex << converted_value << std::endl;
		}
		else if (literal_expression->get_expression_type() == STRING) {
			// first, define a constant for the string using our naming convention
			std::string string_constant_name;
			string_constant_name = "__STRC__NUM_" + std::to_string(this->strc_number);	// define the constant name
			this->strc_number++;	// increment the strc number by one

			// define the constant
			fetch_ss << "@db " << string_constant_name << " (" << literal_expression->get_value() << ")" << std::endl;

			// load the A and B registers appropriately
			fetch_ss << "\t" << "loadb #" << string_constant_name << std::endl;
			fetch_ss << "\t" << "loada #$" << std::hex << literal_expression->get_value().length() << std::endl;
		}
	}
	// TODO: fetch other types like lvalues, dereferenced values, etc.
	else if (to_fetch->get_expression_type() == LVALUE) {
		// get the lvalue's symbol data
		LValue* variable_to_get = dynamic_cast<LValue*>(to_fetch.get());
		Symbol* variable_symbol = this->symbol_table.lookup(variable_to_get->getValue(), this->current_scope_name);

		// only fetch the value if it has been defined
		if (variable_symbol->defined) {
			// check the scope; we need to do different things for global and local scopes
			if ((variable_symbol->scope_name == "global") && (variable_symbol->scope_level == 0)) {
				// TODO: fetch variable from global scope
				// all we need to do is use the variable name for globals; however, we need to know the type
				if ((variable_symbol->type == INT) || (variable_symbol->type == BOOL) || (variable_symbol->type == FLOAT)) {
					fetch_ss << "\t" << "loada " << variable_symbol->name << std::endl;
				}
				else if (variable_symbol->type == STRING) {
					// TODO: what do we load into A when we are loading a global string?
					// TODO: add support for data types with variable lengths (like strings)
				}
			}
			else {
				// TODO: fetch variable from local scope
			}
		}
		else {
			throw std::exception(("Variable '" + variable_symbol->name + "' referenced before assignment (line " + std::to_string(line_number) + ")").c_str());
		}
	}

	return fetch_ss;
}



// allocate a variable
std::stringstream Compiler::allocate(Allocation allocation_statement, size_t* stack_offset, size_t* max_offset) {
	// first, add the variable to the symbol table
	// next, create a macro for it and assign it to the next available address in the scope; local variables will use pages incrementally
	// next, every time a variable is referenced, make sure it is in the symbol table and within the appropriate scope
	// after that, simply use that variable's name as a macro in the code
	// the macro will reference its address and hold the proper value

	std::stringstream allocation_ss;

	std::string symbol_name = allocation_statement.get_var_name();
	Type symbol_type = allocation_statement.get_var_type();
	std::string symbol_quality = allocation_statement.get_quality();
	bool initialized = allocation_statement.was_initialized();
	std::shared_ptr<Expression> initial_value = allocation_statement.get_initial_value();

	// the 'insert' function will throw an error if the given symbol is already in the table in the same scope
	// this allows two variables in separate scopes to have different names, but also allows local variables to shadow global ones
	// local variable names are ALWAYS used over global ones
	this->symbol_table.insert(symbol_name, symbol_type, current_scope_name, current_scope, symbol_quality, initialized);

	// if we have a const, we can use the "@db" directive
	if (symbol_quality == "const") {
		// make sure it is initialized
		if (initialized) {

			// TODO: we seem to be getting the values of expressions and comparing them to symbols a lot before writing assembly -- could we make this process more modular so we can call a function and hide the dirty details of it away? It would make for easier to read and more maintainable code if it's possible

			// get the initial value's expression type and handle it accordingly
			if (initial_value->get_expression_type() == LITERAL) {
				// literal values are easy
				Literal* const_literal = dynamic_cast<Literal*>(initial_value.get());

				// make sure the types match
				if (symbol_type == const_literal->get_type()) {
					std::string const_value = const_literal->get_value();

					// use "@db"
					allocation_ss << "@db " << symbol_name << " (" << const_value << ")" << std::endl;
				}
				else {
					throw std::exception(("Types do not match (line " + std::to_string(allocation_statement.get_line_number()) + ")").c_str());
				}
			}
			else if (initial_value->get_expression_type() == UNARY) {
				Unary* const_unary = dynamic_cast<Unary*>(initial_value.get());
				
				// TODO: write unary scheme for constants
				// TODO: allow constants to use the value of an address ?
			}
			else if (initial_value->get_expression_type() == BINARY) {
				// produce a binary tree; the result will be in A
				// TODO: allow constants to use the value of an address
			}
			else if (initial_value->get_expression_type() == LVALUE) {
				// TODO: write lvalue scheme for constants
			}
			else if (initial_value->get_expression_type() == DEREFERENCED) {
				// TODO: write dereferenced scheme for constants
			}
		}
		else {
			// this error should have been caught by the parser, but just to be safe...
			throw std::exception(("**** Const-qualified variables must be initialized in allocation (error occurred on line " + std::to_string(allocation_statement.get_line_number()) + ")").c_str());
		}
	}
	// if the current scope is 0, we can use a "@rs" directive
	else if (current_scope_name == "global") {
		// if the variable type is anything with a variable length, we need a different mechanism for handling them
		if (symbol_type == STRING) {
			// TODO: add support for global strings, arrays, etc.
		}
		else {
			// reserve the variable itself
			allocation_ss << "@rs " << symbol_name << std::endl;	// syntax is "@rs macro_name"
		}
	}
	// otherwise, we must use the next available memory address in the current scope
	else {
		// our local variables will use the stack; they will directly modify the list of variable names and the stack offset

		// first, make sure we have valid pointers
		if ((stack_offset != nullptr) && (max_offset != nullptr)) {
			// add the variables to the symbol table and to the function's list of variable names

			// update the stack offset
			if (symbol_type == STRING) {
				// strings will increment the stack offset by two words; allocate the space for it on the stack by decrementing the stack pointer and also increment the stack_offset variable by two words
				allocation_ss << "\t" << "decsp" << std::endl;
				allocation_ss << "\t" << "decsp" << std::endl;
				(*stack_offset) += 2;
				(*max_offset) += 2;
			}
			else {
				// all other types will increment the stack offset by one word; allocate space in the stack and increment the offset counter by one word
				allocation_ss << "\t" << "decsp" << std::endl;
				(*stack_offset) += 1;
				(*max_offset) += 1;
			}
		}
		else {
			// if we forgot to supply the address of our counter, it will throw an exception
			throw std::exception("**** Cannot allocate memory for variable; expected pointer to stack offset counter, but found 'nullptr' instead.");
		}
	}

	// return our allocation statement
	return allocation_ss;
}

// create a function definition
std::stringstream Compiler::define(Definition definition_statement) {
	// first, add the function to the symbol table
	std::shared_ptr<Expression> func_name_expr = definition_statement.get_name();
	LValue* lvalue_ptr = dynamic_cast<LValue*>(func_name_expr.get());
	std::string func_name = lvalue_ptr->getValue();
	Type return_type = definition_statement.get_return_type();

	// create a stringstream for our function data
	std::stringstream function_asm;

	// stack offsets
	size_t current_stack_offset = 0;	// the offset from when we originally called the function
	size_t max_stack_offset = 0;	// the maximum stack offset for the function (increment by one per local variable)

	// function definitions have to be in the global scope
	if (current_scope_name == "global" && current_scope == 0) {
		this->symbol_table.insert(func_name, return_type, "global", 0, "none", true, definition_statement.get_args());
		// next, we will make sure that all of the function code gets written to a stringstream; once we have gone all the way through the AST, we will write our functions as subroutines to the end of the file

		// create a label for the function name
		function_asm << func_name << ":" << std::endl;

		std::vector<std::shared_ptr<Statement>> func_args = definition_statement.get_args();

		for (std::vector<std::shared_ptr<Statement>>::iterator arg_iter = func_args.begin(); arg_iter != func_args.end(); arg_iter++) {

			stmt_type arg_type = arg_iter->get()->get_statement_type();	// get the argument type

			if (arg_type == ALLOCATION) {
				// get the allocation statement for the argumnet
				Allocation* arg_alloc = dynamic_cast<Allocation*>(arg_iter->get());
				
				// add the variable to the symbol table, giving it the function's scope name at scope level 1
				this->symbol_table.insert(arg_alloc->get_var_name(), arg_alloc->get_var_type(), func_name, 1, arg_alloc->get_quality(), arg_alloc->was_initialized());	// this variable is only accessible inside this function's scope

				// TODO: add default parameters

				// note that the parameters will be pushed to the stack when called
				// so, they will be on the stack already when we jump to the label

				// however, we do need to increment the position in the stack according to what was pushed so we know how to navigate through our local variables, which will be on the stack for the function's lifetime; note we are not actually incrementing the pointer, we are just incrementing the variable that the compiler itself uses to track where we are in the stack
				if (arg_alloc->get_var_type() == STRING) {
					// strings take two words; one for the length, one for the start address
					current_stack_offset += 2;
				}
				else {
					// all other types are one word
					current_stack_offset += 1;
				}
			}
			else {
				// currently, only alloc statements are allowed in function definitions
				throw std::exception(("**** Semantic error: only allocation statements are allowed in function parameter definitions (error occurred on line " + std::to_string(definition_statement.get_line_number()) + ").").c_str());
			}
		}
		
		// by this point, all of our function parameters are on the stack, in our symbol table, and our stack offset pointer tells us how many; as such, we can call the compile routine on our AST, as all of the functions for compilation will be able to handle scopes other than global
		StatementBlock function_procedure = *definition_statement.get_procedure().get();	// get the AST
		function_asm << this->compile_to_sinasm(function_procedure, 1, func_name, &current_stack_offset, &max_stack_offset).str();	// compile it

		// unwind the stack pointer back to its original position, thereby freeing the memory we allocated for the function
		for (int i = current_stack_offset; i > 0; i--) {
			function_asm << "\t" << "incsp" << std::endl;
		}

		// return from the subroutine
		function_asm << "\t" << "rts" << std::endl;

		// return our scope name to "global", the current scope to 0, and return the function assembly
		this->current_scope_name = "global";
		this->current_scope = 0;
		return function_asm;
	}
	else {
		throw std::exception(("**** Compiler Error: Function definitions must be in the global scope (error occurred on line " + std::to_string(definition_statement.get_line_number()) + ").").c_str());
	}
}



std::stringstream Compiler::assign(Assignment assignment_statement, size_t* stack_offset) {
	// create a stringstream to which we will write write our generated code
	std::stringstream assignment_ss;

	// first, check to see if the variable is in the symbol table
	std::string var_name = assignment_statement.get_lvalue_name();

	if (this->symbol_table.is_in_symbol_table(var_name, this->current_scope_name)) {
		// get the symbol information
		Symbol* fetched = this->symbol_table.lookup(var_name, this->current_scope_name);

		// if the quality is "const" throw an error; we cannot make assignments to const-qualified variables
		if (fetched->quality == "const") {
			throw std::exception(("**** Cannot make an assignment to a const-qualified variable! (error occurred on line " + std::to_string(assignment_statement.get_line_number()) + ")").c_str());
		}
		else {
			// make the assignment based on the type
			exp_type RValueType = assignment_statement.get_rvalue_expression_type();

			// we can have a binary expression
			if (RValueType == BINARY) {
				// cast to binary object
				Binary* bin_exp = dynamic_cast<Binary*>(assignment_statement.get_rvalue().get());

				// TODO: make sure types match before writing out a binary tree...

				// write out the binary tree information
				this->produce_binary_tree(*bin_exp);

				// the result of the binary expression will now be in the A register
				// now, if the variable is global, we will simply use the macro for it; if not, we have to manipulate the stack
				if (fetched->scope_name == "global") {
					assignment_ss << "storea " << var_name << std::endl;
				}
				else {
					// make sure our vector of variable names isn't empty and that our offset is not 0
					// these parameters can be left blank IF we are making an assignment to a global variable, but here, we are not
					if (*stack_offset == 0) {
						throw std::exception("**** Tried to make assignment in function definition, but was passed empty vector of variables and/or no stack offset.");
					}
					else {

						// TODO: make assignment in stack variable appropriately

					}
				}

				// if the symbol is not marked as defined, define it
				if (!this->symbol_table.lookup(var_name, this->current_scope_name)->defined) {
					this->symbol_table.define(var_name, this->current_scope_name);
				}
			}
			// we can have a literal as an rvalue
			else if (RValueType == LITERAL) {
				// create an object for the literal rvalue
				Literal* literal_arg = dynamic_cast<Literal*>(assignment_statement.get_rvalue().get());

				// we must first make sure the lvalue and rvalue types match
				if (fetched->type == literal_arg->get_type()) {
					// now, the compiler must write different code depending on whether we have a global variable or a local one
					if (fetched->scope_name == "global") {
						assignment_ss << "\t" << "loada #$" << std::hex << literal_arg->get_value() << std::endl;
						assignment_ss << "\t" << "storea " << fetched->name << std::endl;
					}
					else {
						// first, we must get the placement of the variable in the stack so we know where to write
						// we will do this by iterating through the symbol table, stopping at the symbols in our scope and adding to our target offset if we haven't hit our target symbol
						size_t target_offset = 0;
						std::vector<Symbol>::iterator symbol_iter = this->symbol_table.symbols.begin();
						bool found = false;
						while ((symbol_iter != this->symbol_table.symbols.end()) && !found) {

							/*

							TODO: devise more effective scope checks; consider a situation like this:

								def int myFunc(alloc int a){
									if (a < 10) {
										alloc int b;
									}
									else {
										alloc int c;
										let b = 5;
									}
									return 1;
								}

							In the above situation, given the algorithm we have now, the function name is the scope and the scope level is given by the indentation level; both b and c are then considered to be in the same scope according to this algorithm.
							While the assignment statement is bogus for a number of reasons, the compiler may not catch it correctly using the algorithm we have now. As such, a better approach must be devised for a future version of the compiler. For now, however, we will leave it as is.

							*/

							// we will only consider variables in the same scope whose scope level is equal to or less than our current scope level
							if ((symbol_iter->scope_name == fetched->scope_name) && (symbol_iter->scope_level <= fetched->scope_level)) {
								// if the symbol name is there, we are done
								if (symbol_iter->name == fetched->name) {
									// set our found variable
									found = true;

									// our target offset is where we want to go; adjust the current offset and increment/decrement the SP accordingly
									if (*stack_offset > target_offset) {
										// if the stack_offset is greater than the target offset, we need to increment the stack pointer but decrement the stack offset (they grow in opposite directions)
										for (; *stack_offset > target_offset; (*stack_offset) -= 1) {
											assignment_ss << "\t" << "incsp" << std::endl;
										}
									}
									else if (target_offset > *stack_offset) {
										// if the stack_offset is less than the target offset, we need to decrement the stack pointer but increment the stack offset (they grow in opposite directions)
										for (; *stack_offset < target_offset; (*stack_offset) += 1) {
											assignment_ss << "\t" << "decsp" << std::endl;
										}
									}
									else if (target_offset == *stack_offset) {
										// otherwise, we do nothing; the SP must be where we want it
									}
								}
								// otherwise, increase the target offset, increment the iterator, and continue
								else {
									// if the type is a string
									if (symbol_iter->type == STRING) {
										// increment by two words
										target_offset += 2;
									}
									else {
										// only increment by one word if it is not a string type
										target_offset++;
									}
									// increment the iterator so we don't get stuck in an infinite loop
									symbol_iter++;
								}
							}
							// if it is out of scope, increment the iterator; don't consider any out of scope variables
							else {
								symbol_iter++;
							}
						}

						// check to make sure we got the variable
						if (found) {
							// now that we have the position in the stack, we can make the assignment
							// if it is a string,
							if (literal_arg->get_type() == STRING) {
								// define the string as a constant
								/*

								Literal argument - string constant naming convention:
									__ARG__STRC_SP_X
								where X is the stack_offset in hex

								*/
								assignment_ss << "@db " << "__ARG__STRC_SP_" << std::hex << target_offset << " (" << literal_arg->get_value() << ")" << std::endl;
								size_t string_length = literal_arg->get_value().length();

								assignment_ss << "\t" << "loada #$" << std::hex << string_length << std::endl;
								assignment_ss << "\t" << "loadb " << "#__ARG__STRC_SP_" << std::hex << target_offset << std::endl;

								assignment_ss << "\t" << "pha" << std::endl;
								assignment_ss << "\t" << "phb" << std::endl;

								// modify the stack offset so we know where we are in the stack
								(*stack_offset) += 2;
							}
							// if it is not a string,
							else {
								// if it is an int, load the A register with that value and push it to the stack
								assignment_ss << "\t" << "loada #" << literal_arg->get_value() << std::endl;
								assignment_ss << "\t" << "pha" << std::endl;

								// increment the stack offset by one word
								(*stack_offset) += 1;
							}

							// finally, mark it as defined if it is not already
							if (!this->symbol_table.lookup(var_name, this->current_scope_name)->defined) {
								this->symbol_table.define(var_name, this->current_scope_name);
							}
						}
						else {
							// if we could not locate the variable, throw an exception
							throw std::exception(("**** Error: Could not locate local variable '" + fetched->name + "' for stack address (error occurred on line " + std::to_string(assignment_statement.get_line_number()) + ").").c_str());
						}
					}
				}
				// if they do not match, throw an exception
				else {
					throw std::exception(("**** Type Error: Cannot make assignment of '" + literal_arg->get_value() + "' to '" + fetched->name + "' because the types are incompatible (cannot pair '" + get_string_from_type(literal_arg->get_type()) + "' with '" + get_string_from_type(fetched->type) + "'). Error occurred on line " + std::to_string(assignment_statement.get_line_number()) + ".").c_str());
				}
			}
			// we can also have a variable as our rvalue
			else if (RValueType == LVALUE) {
				// get our variable
				LValue variable_to_get = *dynamic_cast<LValue*>(assignment_statement.get_rvalue().get());

				// check to sure that the lvalue is in our symbol table in our current scope or in the global scope
				if ((this->symbol_table.is_in_symbol_table(variable_to_get.getValue(), this->current_scope_name)) || (this->symbol_table.is_in_symbol_table(variable_to_get.getValue(), "global"))) {
					// TODO: compile for lvalue
				}
				else {
					throw std::exception(("**** Could not find '" + variable_to_get.getValue() + "' in symbol table (at least, not in an appropriate scope). Error occurred on line " + std::to_string(assignment_statement.get_line_number())).c_str());
				}
			}
			else if (RValueType == UNARY) {
				// TODO: compile assignment for unary
			}
		}
	}
	else {
		throw std::exception(("**** Error: Could not find '" + var_name + "' in symbol table").c_str());
	}

	// return the assignment statement
	return assignment_ss;
}



std::stringstream Compiler::call(Call call_statement, size_t* stack_offset, size_t max_offset) {
	/*
	
	Compile a function call, held in 'call_statement'. We will use stack_offset and max_offset to hold the stack offset at the time the function is called and the maximum offset for local variables, respectively. stack_offset and max_offset are both default parameters, set to nullptr and 0, respectively, and serve to track stack locations for using local variables. If no local variables have been allocated at call time, they will both be 0
	
	*/
	
	// create a stream for the assembly generated by the function call
	std::stringstream call_ss;
	Symbol func_to_call_symbol;

	// get the symbol of our function

	// TODO: import symbol tables from included files!

	if (this->symbol_table.is_in_symbol_table(call_statement.get_func_name(), "global")) {
		func_to_call_symbol = *(this->symbol_table.lookup(call_statement.get_func_name(), "global"));
	}
	else {
		throw std::exception(("Cannot locate function in symbol table (perhaps you didn't include the right file?) (line " + std::to_string(call_statement.get_line_number()) + ")").c_str());
	}

	std::vector<std::shared_ptr<Statement>> formal_parameters = func_to_call_symbol.formal_parameters;

	// if we don't have any arguments, just write a jsr
	if (call_statement.get_args_size() == 0) {
		call_ss << "\t" << "jsr " << call_statement.get_func_name() << std::endl;
	}
	else {

		// TODO: push function arguments to stack

		// add a push statement for each argument
		for (int i = 0; i < call_statement.get_args_size(); i++) {
			// get the expression for the argument
			Expression* argument = call_statement.get_arg(i).get();

			// and the expression for the formal parameter
			Allocation* formal_parameter = dynamic_cast<Allocation*>(formal_parameters[i].get());
			Type formal_type = formal_parameter->get_var_type();

			// TODO: create a literal expression from an lvalue (or other argument type) and update 'argument' to point to the result; this would allow us to use the folllowing code for all values
			// TODO: figure out how to handle negatives in assembly generation

			// we can be passed a literal value, an lvalue, a unary, or a binary expression
			if (argument->get_expression_type() == LITERAL) {
				Literal* literal_argument = dynamic_cast<Literal*>(argument);
				Type argument_type = literal_argument->get_type();
				
				// make sure the type passed to the function matches what is expected
				if (argument_type == formal_type) {
					call_ss << "\t";
					if (argument_type == INT) {
						/*
						the INT type is easy to handle; simply write the number
						*/
						call_ss << "loada #$" << std::hex << literal_argument->get_value() << std::endl;
					}
					else if (argument_type == STRING) {
						// add a loada statement for the string length
						call_ss << "loada #$" << std::hex << literal_argument->get_value().length() << std::endl;
						// add a loadb statement for the string litereal
						call_ss << "\t" << "@db __ARGC_STR_" << call_statement.get_func_name() << "__" << i << " (" << literal_argument->get_value() << ")" << std::endl;
						call_ss << "\t" << "loadb #" << "__ARGC_STR_" << call_statement.get_func_name() << "__" << i << std::endl;

						// push the length, then the address of the string
						call_ss << "\t" << "pha" << std::endl;
						call_ss << "\t" << "phb" << std::endl;
					}
					else if (argument_type == BOOL) {
						/*
						a bool will be handled as follows:
							1) if the value is False, write a 0
							2) if the value is True, write a 1
						however,
							NB: Any nonzero value will be interpreted as True in SIN
						*/

						if (literal_argument->get_value() == "True") {
							call_ss << "loada #$01" << std::endl;
							call_ss << "\t" << "pha" << std::endl;
						}
						else if (literal_argument->get_value() == "False") {
							call_ss << "loada #$00" << std::endl;
							call_ss << "\t" << "pha" << std::endl;
						}
						else {
							// if the boolean value is not 'True' or 'False', throw an exception
							throw std::exception(("**** Expected a value of 'True' or 'False' in boolean literal; instead got '" + literal_argument->get_value() + "' (error occurred on line " + std::to_string(call_statement.get_line_number()) + ")").c_str());
						}
					}
					else if (argument_type == FLOAT) {
						// TODO: establish writing of float type...
					}
				}
				// otherwise, throw an exception
				else {
					throw std::exception(("Type error: expected argument of type '" + get_string_from_type(formal_type) + "', but got '" +  get_string_from_type(argument_type) + "' instead (error occurred on line " + std::to_string(call_statement.get_line_number()) + ")").c_str());
				}
			}
			else if (argument->get_expression_type() == LVALUE) {
				LValue* var_arg_exp = dynamic_cast<LValue*>(argument);

				// if the variable is in the symbol table
				if (this->symbol_table.is_in_symbol_table(var_arg_exp->getValue(), this->current_scope_name)) {
					// if the lvalue we are passing is accessible
					if (this->symbol_table.is_in_symbol_table(var_arg_exp->getValue(), this->current_scope_name)) {
						// then, get the variable
						Symbol argument_symbol_data = *this->symbol_table.lookup(var_arg_exp->getValue(), this->current_scope_name);

						// if the scope is global, the task is easy
						if (argument_symbol_data.scope_name == "global") {
							call_ss << "\t" << "loada " << argument_symbol_data.name << std::endl;

							// if we are not in a subscope of any kind, then we can just push the variable onto the stack
							if ((this->current_scope == 0) && (this->current_scope_name == "global")) {
								call_ss << "\t" << "pha" << std::endl;
							}
							// if we are currently in some sort of scope besides global, we need to make sure we don't overwrite our local variables
							else {
								// TODO: navigate the current stack position forward to the end of the local variables and push the argument
							}
						}
						// if it is not, our task is a little more difficult
						else {
							if (stack_offset != nullptr) {
								// compare the current stack offset to the stack offset of our variable -- it must be (variable.stack_offset - 1), as it must point to one word /below/ the variable so when we pull from the stack, we get the correct one (as pla/plb pulls the word /before/ the one to which the SP points)
								// if we are too far forward in the stack, move backward
								if (*stack_offset > (argument_symbol_data.stack_offset - 1)) {
									// decrement stack_offset until it matches up with our symbol's stack offset - 1
									for (; *stack_offset > (argument_symbol_data.stack_offset + 1); (*stack_offset)--) {
										call_ss << "\t" << "decsp" << std::endl;	// decrement the SP by a word
									}
								}
								else if (*stack_offset < (argument_symbol_data.stack_offset + 1)) {
									// increment stack_offset until it matches with our symbol's offset - 1
									for (; *stack_offset < (argument_symbol_data.stack_offset - 1); (*stack_offset)++) {
										call_ss << "\t" << "incsp" << std::endl;	// increment the SP by a word
									}
								}
								
								// this condition should always evaluate to true if we make it this far
								if (*stack_offset == (argument_symbol_data.stack_offset - 1)) {
									// now, we can pull the variable into the A register
									call_ss << "\t" << "pla" << std::endl;

									// TODO: figure out how far we need to advance the SP
									// now, we must decrement the stack pointer to the end of the scope's local memory so we don't overwrite current local variables with data for the next scope
								}
								else {
									throw std::exception("**** Mismatched stack offsets");	// we should never reach this, but just to be safe...
								}
							}
							else {
								// if our stack_offset is nullptr, but we needed a value, throw an exception; this will happen if the function is not called properly
								throw std::exception("**** Required 'stack_offset', but it was 'nullptr'");
							}
						}
					}
					else {
						// if the variable is not found, throw an exception
						throw std::exception(("**** The variable you wish to pass into the function is either undefined or inaccessible (error occurred on line " + std::to_string(call_statement.get_line_number()) + ")").c_str());
					}
				}
				else {
					throw std::exception(("**** Could not find '" + var_arg_exp->getValue() + "' in symbol table (error occurred on line " + std::to_string(call_statement.get_line_number()) + ")").c_str());
				}
			}
			else if (argument->get_expression_type() == UNARY) {
				// TODO: evaluate the unary expression and pass the result to the function
			}
			else if (argument->get_expression_type() == BINARY) {
				Binary* binary_argument = dynamic_cast<Binary*>(argument);
				// TODO: produce binary tree
			}
		}

		// finally, use jsr
		call_ss << "\t" << "jsr " << call_statement.get_func_name() << std::endl;
	
	}

	// finally, return the call_ss
	return call_ss;
}

std::stringstream Compiler::ite(IfThenElse ite_statement, size_t * stack_offset, size_t max_offset)
{
	/*
	
	Produce SINASM code for an If/Then/Else branch.

	*/

	// get the "if" condition
	// the simplest to do is a literal
	if (ite_statement.get_condition()->get_expression_type() == LITERAL) {
		Literal* if_condition = dynamic_cast<Literal*>(ite_statement.get_condition().get());

		/*

		Literals are defined to be true/false in the following way:
			Literal value:		Evaluates to:
				True			True
				False			False
				0				False
				1 +				True
				-1 -			True
				0.0				False
				1.0 +			True
				-1.0 -			True
				""				False
				"..."			True	(strings only evaluate to "False" if they are empty)

		Essentially, the assembly performs a "cmp_ #$00 / brne .__" sequence

		*/

		// TODO: use "fetch_value" to put the value in the A register before doing our compare and branch instructions

	}
	else if (ite_statement.get_condition()->get_expression_type() == UNARY) {

	}
	else if (ite_statement.get_condition()->get_expression_type() == BINARY) {

	}
	
	return std::stringstream();
}



// the actual compilation routine
// it is separate from "Compile::compile()" so that we can call it recursively
std::stringstream Compiler::compile_to_sinasm(StatementBlock AST, unsigned int local_scope_level, std::string local_scope_name, size_t* stack_offset, size_t* max_offset) {
	/*
	
	This function takes an AST and produces SINASM that will execute it, stored in a stringstream object.

	Parameters:
		AST - the abstract syntax tree we want to compile into assembly
		local_scope_level - the level of scope we are in
		local_scope_name - default to "global", only necessary if we are working with a function
		size_t* stack_offset - the current stack offset (for local variables to a function)
		size_t* max_offset - the maximum offset for local function variables (increments by one per local function variable)
	
	*/
	
	// set the scope level; this will allow us to use the proper memory addresses for local variables
	this->current_scope = local_scope_level;
	this->current_scope_name = local_scope_name;
	this->next_available_addr = 0;	// the next available address will always start at 0 when we compile a new AST

	std::stringstream sinasm_ss;	// a stringstream object for our SINASM code
	
	for (std::vector<std::shared_ptr<Statement>>::iterator statement_iter = AST.statements_list.begin(); statement_iter != AST.statements_list.end(); statement_iter++) {

		Statement* current_statement = statement_iter->get();
		stmt_type statement_type = current_statement->get_statement_type();

		if (statement_type == INCLUDE) {
			// dynamic_cast to an Include type
			Include* include_statement = dynamic_cast<Include*>(current_statement);

			// "compile" an include statement
			this->include_file(*include_statement);

			// TODO: import symbol tables from included files
		}
		else if (statement_type == INLINE_ASM) {
			// dynamic cast to an InlineAssembly type
			InlineAssembly* asm_statement = dynamic_cast<InlineAssembly*>(current_statement);

			// check to make sure the asm type is valid
			if (asm_statement->get_asm_type() == this->asm_type) {
				// simply copy the asm into our file with comments denoting where it begins/ends
				sinasm_ss << ";; BEGIN ASM FROM .SIN FILE" << std::endl;
				sinasm_ss << asm_statement->asm_code;
				sinasm_ss << ";; END ASM FROM .SIN FILE" << std::endl;
			}
			else {
				// if the types do not match, throw an exception
				throw std::exception(("**** Inline ASM in file does not match compiler's ASM version (line " + std::to_string(asm_statement->get_line_number()) + ")").c_str());
			}
		}
		else if (statement_type == ALLOCATION) {
			// dynamic_cast to an Allocation type
			Allocation* alloc_statement = dynamic_cast<Allocation*>(current_statement);

			// compile an alloc statement
			sinasm_ss << this->allocate(*alloc_statement, stack_offset, max_offset).str();
		}
		else if (statement_type == ASSIGNMENT) {
			// dynamic cast to an Assignment type
			Assignment* assign_statement = dynamic_cast<Assignment*>(current_statement);

			if (stack_offset != nullptr) {
				sinasm_ss << this->assign(*assign_statement, stack_offset).str();
			}
			else {
				sinasm_ss << this->assign(*assign_statement).str();
			}
		}
		else if (statement_type == RETURN_STATEMENT) {
			// dynamic cast to a Return type
			ReturnStatement* return_statement = dynamic_cast<ReturnStatement*>(current_statement);

			// TODO: write return routine
		}
		else if (statement_type == IF_THEN_ELSE) {
			IfThenElse* ite_statement = dynamic_cast<IfThenElse*>(current_statement);

			// TODO: write if/then/else routine
		}
		else if (statement_type == WHILE_LOOP) {
			WhileLoop* while_statement = dynamic_cast<WhileLoop*>(current_statement);
		}
		else if (statement_type == DEFINITION) {
			Definition* def_statement = dynamic_cast<Definition*>(current_statement);

			// write the definition to our stringstream containing our function definitions
			this->functions_ss << this->define(*def_statement).str();
		}
		else if (statement_type == CALL) {
			Call* call_statement = dynamic_cast<Call*>(current_statement);

			// write the call to the function into the file
			if (stack_offset != nullptr) {
				sinasm_ss << this->call(*call_statement, stack_offset).str();
			}
			else {
				sinasm_ss << this->call(*call_statement).str();
			}
		}
	}

	return sinasm_ss;
}


// our entry functions
void Compiler::produce_sina_file(std::string sina_filename, bool include_builtins) {
	/*
	
	This function is to be used if we wish to produce a SINASM (.sina) file from our code. Useful for debugging, optimizations, etc.

	*/
	
	// first, open the file of our filename
	this->sina_file.open(sina_filename, std::ios::out);

	// check to make sure it opened correctly
	if (this->sina_file.is_open()) {

		// compile to ASM

		// all programs include a header to define a few things
		//if (include_builtins) {
		//	this->sina_file << "@include builtins.sina" << std::endl;	// include 'builtins', to allow us to use SIN's built-in functions in the VM
		//	this->object_file_names->push_back("builtins.sinc");
		//}
		// any other common header information will go here

		// write the body of the program
		this->sina_file << this->compile_to_sinasm(this->AST, current_scope).str();

		// write a halt statement before our function definitions
		this->sina_file << "\t" << "halt" << std::endl;

		// now, we need to write the stringstream containing all of our functions to the file
		this->sina_file << this->functions_ss.str();

		// close our output file and return to caller
		this->sina_file.close();
		return;
	}
	// otherwise, throw an exception
	else {
		throw std::exception("**** Fatal Error: Compiler could not open the target .sina file.");
	}
}

std::stringstream Compiler::compile_to_stringstream(bool include_builtins) {
	/*
	
	This function is to be used when we want to produce an object file from the .sin file. The stringstream this file creates can be passed directly into our assembler, as it takes an istream as input (as such, it can be a stringstream or a filestream). This should be what is called by default unless we specify that we want to create an assembly file during the compilation process.

	*/
	
	// declare a stringstream to hold our generated ASM code
	std::stringstream generated_asm;

	// all programs include a header to define a few things
	//if (include_builtins) {
	//	generated_asm << "@include builtins.sina" << std::endl;	// include 'builtins', to allow us to use SIN's built-in functions in the VM
	//	this->object_file_names->push_back("builtins.sinc");
	//}
	// any other common header information will go here

	// write the body of the program
	generated_asm << this->compile_to_sinasm(this->AST, current_scope).str();

	// write a halt statement before our function definitions
	generated_asm << "\t" << "halt" << std::endl;

	// now, we need to write the stringstream containing all of our functions to the file
	generated_asm << this->functions_ss.str();
	
	// return our generated code
	return generated_asm;
}


// If we initialize the compiler with a file, it will automatically lex and parse
Compiler::Compiler(std::istream& sin_file, uint8_t _wordsize, std::vector<std::string>* object_file_names, std::vector<std::string>* included_libraries, bool include_builtins) : _wordsize(_wordsize), object_file_names(object_file_names), library_names(included_libraries) {
	// create the parser and lexer objects
	Lexer lex(sin_file);
	Parser parser(lex);
	
	// get the AST from the parser
	this->AST = parser.createAST();

	this->current_scope = 0;	// start at the global scope
	this->current_scope_name = "global";

	this->next_available_addr = 0;	// start our next available address at 0

	this->strc_number = 0;

	this->_DATA_PTR = 0;	// our first address for variables is $00
	this->AST_index = -1;	// we use "get_next_statement()" every time, which increments before returning; as such, start at -1 so we fetch the 0th item, not the 1st, when we first call the compilation function
	symbol_table = SymbolTable();

	if (include_builtins) {
		Include builtins_include_statement("builtins.sin");
		this->include_file(builtins_include_statement);
	}
	else {
		// do nothing
	}
}

Compiler::Compiler() {
	// TODO: give default values for compiler initialization
}

Compiler::~Compiler()
{
}
