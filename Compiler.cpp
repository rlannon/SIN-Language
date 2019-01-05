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
			Compiler include_compiler(included_sin_file, this->_wordsize, this->object_file_names);
			include_compiler.produce_sina_file(filename_no_extension + ".sina");

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
		}
		// if we cannot open the .sin file
		else {
			throw std::exception(("**** Could not open included file '" + to_include + "'!").c_str());
		}
	}
}



// allocate a variable
std::stringstream Compiler::allocate(Allocation allocation_statement, size_t* stack_offset) {
	// first, add the variable to the symbol table
	// next, create a macro for it and assign it to the next available address in the scope; local variables will use pages incrementally
	// next, every time a variable is referenced, make sure it is in the symbol table and within the appropriate scope
	// after that, simply use that variable's name as a macro in the code
	// the macro will reference its address and hold the proper value

	std::stringstream allocation_ss;

	std::string symbol_name = allocation_statement.getVarName();
	Type symbol_type = allocation_statement.getVarType();

	// the 'insert' function will throw an error if the given symbol is already in the table in the same scope
	// this allows two variables in separate scopes to have different names, but also allows local variables to shadow global ones
	// local variable names are ALWAYS used over global ones
	this->symbol_table.insert(symbol_name, symbol_type, current_scope_name, current_scope);

	// TODO: finish algorithm...

	// if the current scope is 0, we can use a "@rs" directive
	if (current_scope_name == "global") {
		// TODO: add support for constants in the Lexer/Parser so they can be added here; this will allow us to declare constants using @db instead of @rs
		allocation_ss << "@rs " << symbol_name << std::endl;	// syntax is "@rs macro_name"
	}
	// otherwise, we must use the next available memory address in the current scope
	else {
		// our local variables will use the stack; they will directly modify the list of variable names and the stack offset

		// first, make sure we have a valid pointer
		if (stack_offset != nullptr) {
			// add the variables to the symbol table and to the function's list of variable names

			// update the stack offset
			if (symbol_type == STRING) {
				// strings will increment the stack offset by two words; allocate the space for it on the stack by decrementing the stack pointer and also increment the stack_offset variable by two words
				allocation_ss << "\t" << "decsp" << std::endl;
				allocation_ss << "\t" << "decsp" << std::endl;
				(*stack_offset) += 2;
			}
			else {
				// all other types will increment the stack offset by one word; allocate space in the stack and increment the offset counter by one word
				allocation_ss << "\t" << "decsp" << std::endl;
				(*stack_offset) += 1;
			}
		}
		else {
			// if we forgot to supply the address of our counter, it will throw an exception
			throw std::exception("**** Cannot allocate memory for variable; expected pointer to stack offset counter, but found a nullptr instead.");
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

	// the offset from when we originally called the function
	size_t current_stack_offset = 0;

	// function definitions have to be in the global scope
	if (current_scope_name == "global" && current_scope == 0) {
		this->symbol_table.insert(func_name, return_type, "global", 0);
		// next, we will make sure that all of the function code gets written to a stringstream; once we have gone all the way through the AST, we will write our functions as subroutines to the end of the file

		// create a label for the function name
		function_asm << func_name << ":" << std::endl;

		std::vector<std::shared_ptr<Statement>> func_args = definition_statement.get_args();

		for (std::vector<std::shared_ptr<Statement>>::iterator arg_iter = func_args.begin(); arg_iter != func_args.end(); arg_iter++) {

			std::string arg_type = arg_iter->get()->get_type();	// get the argument type

			if (arg_type == "alloc") {
				// get the allocation statement for the argumnet
				Allocation* arg_alloc = dynamic_cast<Allocation*>(arg_iter->get());

				// add the variable to the symbol table, giving it the function's scope name at scope level 1
				this->symbol_table.insert(arg_alloc->getVarName(), arg_alloc->getVarType(), func_name, 1);	// this variable is only accessible inside this function's scope

				// note that the parameters will be pushed to the stack when called
				// so, they will be on the stack already when we jump to the label

				// however, we do need to increment the position in the stack according to what was pushed so we know how to navigate through our local variables, which will be on the stack for the function's lifetime
				if (arg_alloc->getVarType() == STRING) {
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
		function_asm << this->compile_to_sinasm(function_procedure, 1, func_name, &current_stack_offset).str();	// compile it

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
	std::string var_name = assignment_statement.getLvalueName();

	if (this->symbol_table.is_in_symbol_table(var_name, this->current_scope_name)) {
		// get the symbol information
		Symbol* fetched = this->symbol_table.lookup(var_name, this->current_scope_name);

		// make the assignment based on the type
		std::string RValueType = assignment_statement.getRValueType();
		
		if (RValueType == "binary") {
			// cast to binary object
			Binary* bin_exp = dynamic_cast<Binary*>(assignment_statement.getRValue().get());

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
		else if (RValueType == "literal") {
			// create an object for the literal rvalue
			Literal* literal_arg = dynamic_cast<Literal*>(assignment_statement.getRValue().get());
			
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
	}
	else {
		throw std::exception(("**** Error: Could not find '" + var_name + "' in symbol table").c_str());
	}

	// return the assignment statement
	return assignment_ss;
}



std::stringstream Compiler::call(Call call_statement) {
	// create a stream for the assembly generated by the function call
	std::stringstream call_ss;

	// if we don't have any arguments, just write a jsr
	if (call_statement.get_args_size() == 0) {
		call_ss << "\t" << "jsr " << call_statement.get_func_name() << std::endl;
	}
	else {

		// TODO: push function arguments to stack
	
	}

	// next, before we return, we need to increase the current scope and the scope's name
	this->current_scope += 1;
	this->current_scope_name = call_statement.get_func_name();

	// finally, return the call_ss
	return call_ss;
}



// the actual compilation routine
// it is separate from "Compile::compile()" so that we can call it recursively
std::stringstream Compiler::compile_to_sinasm(StatementBlock AST, unsigned int local_scope_level, std::string local_scope_name,size_t* stack_offset) {
	/*
	
	This function takes an AST and produces SINASM that will execute it, stored in a stringstream object.
	
	*/
	
	// set the scope level; this will allow us to use the proper memory addresses for local variables
	this->current_scope = local_scope_level;
	this->current_scope_name = local_scope_name;
	this->next_available_addr = 0;	// the next available address will always start at 0 when we compile a new AST

	std::stringstream sinasm_ss;	// a stringstream object for our SINASM code
	
	for (std::vector<std::shared_ptr<Statement>>::iterator statement_iter = AST.statements_list.begin(); statement_iter != AST.statements_list.end(); statement_iter++) {

		Statement* current_statement = statement_iter->get();
		std::string statement_type = current_statement->get_type();

		if (statement_type == "include") {
			// dynamic_cast to an Include type
			Include* include_statement = dynamic_cast<Include*>(current_statement);

			// "compile" an include statement
			this->include_file(*include_statement);
		}
		else if (statement_type == "alloc") {
			// dynamic_cast to an Allocation type
			Allocation* alloc_statement = dynamic_cast<Allocation*>(current_statement);

			// compile an alloc statement
			sinasm_ss << this->allocate(*alloc_statement, stack_offset).str();
		}
		else if (statement_type == "assign") {
			// dynamic cast to an Assignment type
			Assignment* assign_statement = dynamic_cast<Assignment*>(current_statement);

			if (stack_offset != nullptr) {
				sinasm_ss << this->assign(*assign_statement, stack_offset).str();
			}
			else {
				sinasm_ss << this->assign(*assign_statement).str();
			}
		}
		else if (statement_type == "return") {
			// dynamic cast to a Return type
			ReturnStatement* return_statement = dynamic_cast<ReturnStatement*>(current_statement);

			// TODO: write return routine
		}
		else if (statement_type == "ite") {
			IfThenElse* ite_statement = dynamic_cast<IfThenElse*>(current_statement);

			// TODO: write if/then/else routine
		}
		else if (statement_type == "while") {
			WhileLoop* while_statement = dynamic_cast<WhileLoop*>(current_statement);
		}
		else if (statement_type == "def") {
			Definition* def_statement = dynamic_cast<Definition*>(current_statement);

			// write the definition to our stringstream containing our function definitions
			this->functions_ss << this->define(*def_statement).str();
		}
		else if (statement_type == "call") {
			Call* call_statement = dynamic_cast<Call*>(current_statement);

			// write the call to the function into the file
			sinasm_ss << this->call(*call_statement).str();
		}
	}

	return sinasm_ss;
}



// our entry function
void Compiler::produce_sina_file(std::string sina_filename) {
	// first, open the file of our filename
	this->sina_file.open(sina_filename, std::ios::out);

	// check to make sure it opened correctly
	if (this->sina_file.is_open()) {

		// compile to ASM

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

std::stringstream Compiler::compile_to_stringstream() {
	// compile to ASM
	std::stringstream generated_asm;

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
Compiler::Compiler(std::istream& sin_file, uint8_t _wordsize, std::vector<std::string>* object_file_names) : _wordsize(_wordsize), object_file_names(object_file_names) {
	// create the parser and lexer objects
	Lexer lex(sin_file);
	Parser parser(lex);
	
	// get the AST from the parser
	this->AST = parser.createAST();

	this->current_scope = 0;	// start at the global scope
	this->current_scope_name = "global";

	this->_DATA_PTR = 0;	// our first address for variables is $00
	this->AST_index = -1;	// we use "get_next_statement()" every time, which increments before returning; as such, start at -1 so we fetch the 0th item, not the 1st, when we first call the compilation function
	symbol_table = SymbolTable();
}


Compiler::~Compiler()
{
}
