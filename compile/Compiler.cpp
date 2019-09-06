/*

SIN Toolchain
Compiler.cpp
Copyright 2019 Riley Lannon

The implementation of the class defined in Compiler.h

*/


#include "Compiler.h"


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
				throw CompilerException("Compiled the include file '" + filename_no_extension + ".sina" + "', but could not open the compiled version for assembly.", 0, include_statement.get_line_number());
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
				for (std::vector<std::shared_ptr<Symbol>>::iterator it = include_compiler->symbol_table.symbols.begin(); it != include_compiler->symbol_table.symbols.end(); it++) {
					// if the symbol we found is a variable, insert a variable
					if (it->get()->symbol_type == VARIABLE) {
						Symbol variable = *it->get();
						this->symbol_table.insert(std::make_shared<Symbol>(variable), include_statement.get_line_number());
					}
					// otherwise, if it's a function, insert a symbol for a function definition
					else if (it->get()->symbol_type == FUNCTION_DEFINITION) {
						FunctionSymbol* function = dynamic_cast<FunctionSymbol*>(it->get());
						this->symbol_table.insert(std::make_shared<FunctionSymbol>(*function), include_statement.get_line_number());
					}
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
					throw CompilerException("Compiled the include file '" + filename_no_extension + ".sina" + "', but could not open the compiled version for assembly.", 0, include_statement.get_line_number());
				}

				included_sin_file.close();

				// we allocated memory for include_compiler, so we must now delete it
				delete include_compiler;
			}
			// if we cannot open the .sin file
			else {
				throw CompilerException("Could not open included file '" + to_include + "'!", 0, include_statement.get_line_number());
			}
		}

		// after including, add the filename (no extension) to our libraries list so we don't include it again
		this->library_names->push_back(filename_no_extension);
	}
}


void Compiler::handle_declaration(Declaration declaration_statement)
{
	/*
	
	Handles a declaration statement. Generates the symbol from 'declaration_statement' and adds it to the symbol table.
	This function generates no assembly, so this symbol will be added when it is linked with the implementation file. If there is no implementation file, the linker will generate errors because symbols will be referenced but undefined.
	
	Symbols generated with the 'decl' keyword are:
		- assumed to be in the global scope at level 0, as the 'decl' keyword cannot generate symbols that will exist on the stack;
		- isn't listed as defined unless a definition in an implementation file is found; if there is no definition, this will be caught by the linker

	*/

	// generate the symbol
	Symbol to_add(declaration_statement.get_var_name(), declaration_statement.get_type_information(), "global", 0);

	// if it's a function, we have to construct a FunctionSymbol from the Symbol; insert the proper symbol
	if (declaration_statement.is_function()) {
		FunctionSymbol function_definition(to_add, declaration_statement.get_formal_parameters());
		this->symbol_table.insert(std::make_shared<FunctionSymbol>(function_definition), declaration_statement.get_line_number());
	}
	else {
		this->symbol_table.insert(std::make_shared<Symbol>(to_add), declaration_statement.get_line_number());
	}

	// we are done; return
	return;
}


std::stringstream Compiler::ite(IfThenElse ite_statement, size_t max_offset)
{
	/*
	
	Produce SINASM16 code for an If/Then/Else branch.

	First, make sure everything in our branch is under a label so we can use relative labels -- it looks like:
		__<scope name>_<scope level>__ITE_<branch number>__:

	See the compiler conventions document for more information on naming schemes.

	*/

	// a stringstream for our generated asm
	std::stringstream ite_ss;

	std::string ite_label_name = "__" + this->current_scope_name + "_" + std::to_string(this->current_scope) + "__ITE_" + std::to_string(this->branch_number) + "__";
	ite_ss << ite_label_name << ":" << std::endl;

	// get the "if" condition
	// the simplest to do is a literal
	if ((ite_statement.get_condition()->get_expression_type() == LITERAL) || (ite_statement.get_condition()->get_expression_type() == LVALUE)) {
		/*

		Note that this branch will work for both literals AND lvalues as they only require the fetch_value function and a compare statement -- the LValue will be evaluated in fetch_value, and so the comparison will be done on a literal value held in the A register

		Literals are defined to be true/false in the following way:
			Literal value:		Evaluates to:
				true			True
				false			False
				0				False
				1 +				True
				-1 -			True
				0.0				False
				1.0 +			True
				-1.0 -			True
				""				False
				"..."			True	(strings only evaluate to "False" if they are empty)

		Essentially, the assembly performs a "cmp_ #$00 / breq .else" sequence

		*/

		// load the A register with the value
		ite_ss << this->fetch_value(ite_statement.get_condition(), ite_statement.get_line_number(), max_offset).str();
	}
	else if (ite_statement.get_condition()->get_expression_type() == UNARY) {
		
		// Unary expressions follow similar rules as literals -- see the above list for reference

		Unary* unary_condition = dynamic_cast<Unary*>(ite_statement.get_condition().get());	// cast the condition to the unary type
		ite_ss << this->evaluate_unary_tree(*unary_condition, ite_statement.get_line_number()).str();	// put the evaluated unary expression in A
	}
	else if (ite_statement.get_condition()->get_expression_type() == BINARY) {
		Binary* binary_condition = dynamic_cast<Binary*>(ite_statement.get_condition().get());	// cast to Binary statement
		ite_ss << this->evaluate_binary_tree(*binary_condition, ite_statement.get_line_number(), max_offset).str();
	}
	else {
		throw CompilerException("Invalid expression type in conditional statement!", 0, ite_statement.get_line_number());
	}

	// now that we have evaluated the condition, the result of said evaluation is in A; all we need to do is see whether the result was 0 or not -- if so, jump to the end of the 'if' branch
	ite_ss << "\t" << "cmpa #$00" << std::endl;
	ite_ss << "\t" << "breq " << ite_label_name << ".else" << std::endl;

	// Now that the condition has been evaluated, and the appropriate branch instructions have bene written, we can write the actual code for the branches

	// increment the scope level because we are within a branch (allows variables local to the scope); further, update the scope name to the branch we want
	this->current_scope += 1;
	//this->current_scope_name = parent_scope_name + "__ITE_" + std::to_string(this->branch_number);

	// increment the SP to the end of the stack frame
	ite_ss << this->move_sp_to_target_address(max_offset).str();

	// now, compile the branch using our compile method
	ite_ss << this->compile_to_sinasm(*ite_statement.get_if_branch().get(), this->current_scope, this->current_scope_name, max_offset).str();

	// unwind the stack and delete local variables
	for (size_t i = this->stack_offset; i > max_offset; i--) {
		this->stack_offset -= 1;
		ite_ss << "\t" << "incsp" << std::endl;
	}
	// we now need to delete all variables that were local to this if/else block -- iterate through the symbol table, removing the symbols in this local scope
	std::vector<std::shared_ptr<Symbol>>::iterator it = this->symbol_table.symbols.begin();
	while (it != this->symbol_table.symbols.end()) {
		// only delete variables that are in the scope with the same name AND of the same level
		if ((it->get()->scope_name == this->current_scope_name) && (it->get()->scope_level == this->current_scope)) {
			it = this->symbol_table.symbols.erase(it);	// delete the element at the iterator position, and continue without incrementing the iterator
		}
		else {
			it++;	// only increment the iterator if we do not have a deletion
		}
	}

	// now, jump to our .done label so we don't execute the "else" branch
	ite_ss << "\t" << "jmp " << ite_label_name << ".done" << std::endl;
	ite_ss << std::endl;

	// the else label
	ite_ss << ite_label_name << ".else:" << std::endl;

	// if we have an if_then (no else branch), then ignore this
	if (ite_statement.get_else_branch()) {
		// increment the scope level because we are within a branch (allows variables local to the scope)
		ite_ss << this->move_sp_to_target_address(max_offset).str();

		ite_ss << this->compile_to_sinasm(*ite_statement.get_else_branch().get(), this->current_scope, this->current_scope_name, max_offset).str();

		// unwind the stack and delete local variables
		for (size_t i = this->stack_offset; i > max_offset; i--) {
			this->stack_offset -= 1;
			ite_ss << "\t" << "incsp" << std::endl;
		}
		// we now need to delete all variables that were local to this if/else block -- iterate through the symbol table, removing the symbols in this local scope
		it = this->symbol_table.symbols.begin();	// use the same iterator as before
		while (it != this->symbol_table.symbols.end()) {
			// only delete variables that are in the scope with the same name AND of the same level
			if ((it->get()->scope_name == this->current_scope_name) && (it->get()->scope_level == this->current_scope)) {
				it = this->symbol_table.symbols.erase(it);	// delete the element at the iterator position, and continue without incrementing the iterator
			}
			else {
				it++;	// only increment the iterator if we do not have a deletion
			}
		}
	}

	ite_ss << "\t" << "jmp " << ite_label_name << ".done" << std::endl;
	ite_ss << std::endl;

	// finally, increment the branch number, decrement the scope level, write our ".done" label, and delete all local variables to the branches so we cannot reference them outside the ite
	this->branch_number += 1;
	this->current_scope -= 1;
	//this->current_scope_name = parent_scope_name;	// reset the scope name to the parent scope

	ite_ss << ite_label_name << ".done:" << std::endl;
	ite_ss << std::endl;
	
	// return our branch code
	return ite_ss;
}


std::stringstream Compiler::while_loop(WhileLoop while_statement, size_t max_offset)
{
	/*
	
	Compile the given code into a while loop. The algorithm for generating the code is as follows (note in the future we may use some register optimizations for incrementing counting variables)

	The algorithm essentially works as follows:
		First, we must evaluate the condition of the loop; if it is true, we continue; if false, we branch to the .done label
		Next, we must increment the stack pointer to the end of the current scope's stack frame so that we can allocate local variables
		Then, we run the code inside the loop, allocating any variables within the loop body in a new scope in our symbol table
		Once the execution is complete, before we jump back to the evaluation statement, we must:
			delete all variables from the symbol table that were allocated within the loop body;
			decrement the SP to the bottom of the previous stack frame
		Jump back to the evaluation statement and continue from step 1
		
	Note: See the ite( ... ) function for an explanation of how the compiler evaluates conditional expressions

	*/
	
	std::stringstream while_ss;

	std::string parent_scope_name = this->current_scope_name;
	std::string while_label_name = "__" + this->current_scope_name + "_" + std::to_string(this->current_scope) + "__WHILE_" + std::to_string(this->branch_number) + "__";

	// put the scope label at the top
	while_ss << while_label_name << ":" << std::endl;

	if ((while_statement.get_condition()->get_expression_type() == LITERAL) || (while_statement.get_condition()->get_expression_type() == LVALUE)) {
		while_ss << this->fetch_value(while_statement.get_condition(), while_statement.get_line_number(), max_offset).str();
	}
	else if (while_statement.get_condition()->get_expression_type() == UNARY) {
		Unary* unary_expression = dynamic_cast<Unary*>(while_statement.get_condition().get());
		while_ss << this->evaluate_unary_tree(*unary_expression, while_statement.get_line_number(), max_offset).str();
	}
	else if (while_statement.get_condition()->get_expression_type() == BINARY) {
		Binary* binary_expression = dynamic_cast<Binary*>(while_statement.get_condition().get());
		while_ss << this->evaluate_binary_tree(*binary_expression, while_statement.get_line_number(), max_offset).str();
	}
	else {
		throw CompilerException("Invalid expression type in conditional expression", 0, while_statement.get_line_number());
	}

	// now that A holds the result of the expression, use a compare statement; if the condition evaluates to false, we are done
	while_ss << "\t" << "cmpa #$00" << std::endl;
	while_ss << "\t" << "breq " << while_label_name << ".done" << std::endl;

	// increment our stack pointer to the end of the current stack frame
	while_ss << this->move_sp_to_target_address(max_offset).str();
	// increment the branch and scope numbers and update the scope name
	this->current_scope += 1;

	// put in the label for our loop
	while_ss << while_label_name << ".loop:" << std::endl;

	// compile the branch code
	while_ss << this->compile_to_sinasm(*while_statement.get_branch().get(), this->current_scope, this->current_scope_name, max_offset, max_offset).str();

	// unwind the stack and delete local variables
	while_ss << this->move_sp_to_target_address(max_offset).str();

	// we now need to delete all variables that were local to this if/else block -- iterate through the symbol table, removing the symbols in this local scope
	std::vector<std::shared_ptr<Symbol>>::iterator it = this->symbol_table.symbols.begin();
	while (it != this->symbol_table.symbols.end()) {
		// only delete variables that are in the current subscope
		if ((it->get()->scope_name == this->current_scope_name) && (it->get()->scope_level == this->current_scope)) {
			it = this->symbol_table.symbols.erase(it);	// delete the element at the iterator position, and continue without incrementing the iterator
		}
		else {
			it++;	// only increment the iterator if we do not have a deletion
		}
	}

	// now, jump back to the condition
	while_ss << "\t" << "jmp " << while_label_name << std::endl;

	// now that we are done, put the done label in and perform our clean-up
	while_ss << while_label_name << ".done:" << std::endl;
	this->branch_number += 1;
	this->current_scope_name = parent_scope_name;
	this->current_scope -= 1;

	return while_ss;
}


std::stringstream Compiler::compile_to_sinasm(StatementBlock AST, unsigned int local_scope_level, std::string local_scope_name, size_t max_offset, size_t stack_frame_base) {
	/*
	
	This function takes an AST and produces SINASM that will execute it, stored in a stringstream object. This can be converted into a .sina file, or passed directly to an assembler object.

	Parameters:
		AST - the abstract syntax tree we want to compile into assembly
		local_scope_level - the level of scope we are in
		local_scope_name - default to "global", only necessary if we are working with a function
		max_offset - the maximum offset for local function variables (increments by one per local function variable)
		stack_frame_base - the base offset for our current stack frame
	
	*/
	
	// set the scope level; this will allow us to use the proper memory addresses for local variables
	this->current_scope = local_scope_level;
	this->current_scope_name = local_scope_name;

	std::stringstream sinasm_ss;	// a stringstream object for our SINASM code
	
	for (std::vector<std::shared_ptr<Statement>>::iterator statement_iter = AST.statements_list.begin(); statement_iter != AST.statements_list.end(); statement_iter++) {

		// check to make sure our shared_ptr<Statement> is not a nullptr; this is equivalent to an empty statement
		if (*statement_iter == nullptr) {
			std::cout << "Empty statement found; skipping..." << std::endl;
		}
		else {
			Statement* current_statement = statement_iter->get();
			stmt_type statement_type = current_statement->get_statement_type();

			if (statement_type == INCLUDE) {
				// dynamic_cast to an Include type
				Include* include_statement = dynamic_cast<Include*>(current_statement);

				// "compile" an include statement
				this->include_file(*include_statement);

				// TODO: import symbol tables from included files
			}
			else if (statement_type == DECLARATION) {
				// cast to Declaration type and handle it
				Declaration* decl_statement = dynamic_cast<Declaration*>(current_statement);
				this->handle_declaration(*decl_statement);
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
					throw CompilerException("Inline ASM in file does not match compiler's ASM version", 0, asm_statement->get_line_number());
				}
			}
			else if (statement_type == FREE_MEMORY) {
				// dynamic cast to FreeMemory type
				FreeMemory* free_statement = dynamic_cast<FreeMemory*>(current_statement);

				// look for a symbol in the table with the same name as is indicated by the free statement
				std::shared_ptr<Symbol> fetched = this->symbol_table.lookup(free_statement->get_freed_memory().getValue(), this->current_scope_name,
					this->current_scope);
				Symbol* to_free = dynamic_cast<Symbol*>(fetched.get());	// todo: validate symbol_type of 'fetched' ?

				bool is_const = to_free->type_information.get_qualities().is_const();
				bool is_dynamic = to_free->type_information.get_qualities().is_dynamic();
				bool is_signed = to_free->type_information.get_qualities().is_signed();

				// we can only free dynamic memory that hasn't already been freed
				// however, do not throw this error for a plain pointer because it may point to dynamic memory
				if ( (!to_free->freed && (is_dynamic)) || to_free->type_information.get_primary() == PTR) {
					/*

					The SINASM method to free dynamic memory is simply loading the B register with the address where the variable is in the heap, and use the syscall instruction with syscall number 0x20.

					*/

					// fetch global and local variables differently
					if (to_free->scope_level == 0) {
						sinasm_ss << "\t" << "loadb " << to_free->name << std::endl;
					}
					else {
						sinasm_ss << "\t" << this->move_sp_to_target_address(to_free->stack_offset + 1).str();
						sinasm_ss << "\t" << "plb" << std::endl;
						this->stack_offset -= 1;
					}

					// make the syscall
					sinasm_ss << "\t" << "syscall #$20" << std::endl;

					// mark the variable as freed and undefined
					to_free->defined = false;
					to_free->freed = true;
				}
				else {
					throw CompilerException("Cannot free the variable specified; can only free dynamic memory that has not already been freed.", 0, current_statement->get_line_number());
				}
			}
			else if (statement_type == ALLOCATION) {
				// dynamic_cast to an Allocation type
				Allocation* alloc_statement = dynamic_cast<Allocation*>(current_statement);

				// compile an alloc statement
				sinasm_ss << this->allocate(*alloc_statement, &max_offset).str();
			}
			else if (statement_type == ASSIGNMENT) {
				// dynamic cast to an Assignment type and compile an assignment
				Assignment* assign_statement = dynamic_cast<Assignment*>(current_statement);
				sinasm_ss << this->assign(*assign_statement, max_offset).str();
			}
			else if (statement_type == RETURN_STATEMENT) {
				// if the current scope name is "global", throw an error
				if (this->current_scope_name == "global") {
					throw CompilerException("Cannot execute return statement outside of a function.", 0, current_statement->get_line_number());
				}
				else {
					// dynamic cast to a Return type
					ReturnStatement* return_statement = dynamic_cast<ReturnStatement*>(current_statement);

					sinasm_ss << this->return_value(*return_statement, stack_frame_base, return_statement->get_line_number()).str();

					// if the statement is not the last statement, display a warning stating the code is unreachable
					if (statement_iter + 1 != AST.statements_list.end()) {
						compiler_warning("Code after return statement is unreachable", return_statement->get_line_number());
					}
				}
			}
			else if (statement_type == IF_THEN_ELSE) {
				IfThenElse ite_statement = *dynamic_cast<IfThenElse*>(current_statement);
				sinasm_ss << this->ite(ite_statement, max_offset).str();
			}
			else if (statement_type == WHILE_LOOP) {
				WhileLoop* while_statement = dynamic_cast<WhileLoop*>(current_statement);
				sinasm_ss << this->while_loop(*while_statement, max_offset).str();
			}
			else if (statement_type == DEFINITION) {
				Definition* def_statement = dynamic_cast<Definition*>(current_statement);

				// write the definition to our stringstream containing our function definitions
				this->functions_ss << this->define(*def_statement).str();
			}
			else if (statement_type == CALL) {
				Call* call_statement = dynamic_cast<Call*>(current_statement);

				// compile a call to a function
				sinasm_ss << this->call(*call_statement, max_offset).str();
			}
			// if we have a STATEMENT_GENERAL, we had an explicit pass or some sort of parser error
			else if (statement_type == STATEMENT_GENERAL) {
				// generate a compiler warning stating we found an empty statement
				compiler_warning("Empty statement found; could be the result of a parser error or a 'pass' statement", current_statement->get_line_number());
			}
		}
	}

	// finally, return the assembly we generated for the statement block
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
		// if we are not suppressing builtins, invoke the builtins init subroutine
		if (include_builtins) {
			this->sina_file << "\t" << "jsr __builtins_init" << std::endl;
		}

		// write the body of the program
		this->sina_file << this->compile_to_sinasm(this->AST, current_scope, current_scope_name).str();

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
		throw CompilerException("Compiler could not open the target .sina file.");
	}
}

std::stringstream Compiler::compile_to_stringstream(bool include_builtins) {
	/*
	
	This function is to be used when we want to produce an object file from the .sin file. The stringstream this file creates can be passed directly into our assembler, as it takes an istream as input (as such, it can be a stringstream or a filestream). This should be what is called by default unless we specify that we want to create an assembly file during the compilation process.

	*/

	std::cout << "Beginning code generation..." << std::endl;

	// declare a stringstream to hold our generated ASM code
	std::stringstream generated_asm;

	// if we are not suppressing builtins, we must make a call to the builtins init function
	if (include_builtins) {
		generated_asm << "\t" << "jsr __builtins_init" << std::endl;
	}

	// generate our ASM code
	generated_asm << this->compile_to_sinasm(this->AST, current_scope, current_scope_name).str();

	// write a halt statement before our function definitions
	generated_asm << "\t" << "halt" << std::endl;

	// now, we need to write the stringstream containing all of our functions to the file
	generated_asm << this->functions_ss.str();
	
	// return our generated code
	return generated_asm;
}


/**********			CONSTRUCTOR / DESTRUCTOR		**********/


// If we initialize the compiler with a file, it will automatically lex and parse
Compiler::Compiler(std::istream& sin_file, uint8_t _wordsize, std::vector<std::string>* object_file_names, std::vector<std::string>* included_libraries, bool include_builtins) : _wordsize(_wordsize), object_file_names(object_file_names), library_names(included_libraries) {
	// create the parser and lexer objects
	Lexer lex(sin_file);
	Parser parser(lex);

	std::cout << "Beginning parse..." << std::endl;
	// get the AST from the parser
	this->AST = parser.create_ast();
	std::cout << "Done parsing." << std::endl;

	this->current_scope = 0;	// start at the global scope
	this->current_scope_name = "global";

	this->stack_offset = 0;

	this->strc_number = 0;
	this->branch_number = 0;

	this->AST_index = 0;
	
	symbol_table = SymbolTable();

	if (include_builtins) {
		Include builtins_include_statement("builtins.sin");
		this->include_file(builtins_include_statement);
	}
}

Compiler::Compiler() {
	this->library_names = {};
	this->AST_index = 0;
	this->_wordsize = 16;
	this->current_scope = 0;
	this->current_scope_name = "global";
	this->strc_number = 0;
	this->branch_number = 0;
	this->object_file_names = {};
	this->stack_offset = 0;
}

Compiler::~Compiler()
{
}
