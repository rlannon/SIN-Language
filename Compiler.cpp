/*

SIN Toolchain
Compiler.cpp
Copyright 2019 Riley Lannon

The implementation of the class defined in Compiler.h

*/


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


Type Compiler::get_expression_data_type(std::shared_ptr<Expression> to_evaluate, bool get_subtype)
{
	/*
	
	This function takes an expression and returns its expected data type were it to be evaluated. For example, passing it an int literal would return INT, while a variable (string) 'myStr' will be looked up in the symbol table and evaluated before returning STRING.

	Note that this function does not evaluate binary trees or unary expressions; rather, it looks at the first literal or lvalue it can find and returns that value. It evaluates the type that the tree is /expected/ to return if it was constructed correctly; type match errors will be discovered once the compiler actually attempts to produce the tree in assembly.

	The function works by checking the expression type of to_evaluate and returning the value, and operates recursively if it sees another shared_ptr as an operand.

	*/

	// start with the two that we can do without recursion
	if (to_evaluate->get_expression_type() == LITERAL) {
		Literal* literal_exp = dynamic_cast<Literal*>(to_evaluate.get());

		return literal_exp->get_type();
	}
	else if (to_evaluate->get_expression_type() == LVALUE || to_evaluate->get_expression_type() == INDEXED) {
		LValue* lvalue_exp = dynamic_cast<LValue*>(to_evaluate.get());

		// make sure it's in the symbol table
		if (this->symbol_table.is_in_symbol_table(lvalue_exp->getValue(), this->current_scope_name)) {
			// get the variable's symbol and return the type
			Symbol* lvalue_symbol = this->symbol_table.lookup(lvalue_exp->getValue(), this->current_scope_name, this->current_scope);

			// if the expression is not indexed
			if (to_evaluate->get_expression_type() != INDEXED) {
				if (get_subtype && (lvalue_symbol->sub_type != NONE)) {
					return lvalue_symbol->sub_type;
				}
				else {
					return lvalue_symbol->type;
				}
			}
			// if we have an indexed expression, return its subtype
			else {
				return lvalue_symbol->sub_type;
			}
		}
		else {
			throw CompilerException("Cannot find '" + lvalue_exp->getValue() + "' in symbol table (perhaps it is out of scope?)");
		}
	}
	// the next ones will require recursion, as they have a shared_ptr as a class member
	else if (to_evaluate->get_expression_type() == ADDRESS_OF) {
		AddressOf* address_of_exp = dynamic_cast<AddressOf*>(to_evaluate.get());

		// The routine for an address_of expression is the same as for an lvalue one...so we can just use recursion
		return this->get_expression_data_type(std::make_shared<LValue>(address_of_exp->get_target()));
	}
	else if (to_evaluate->get_expression_type() == UNARY) {
		Unary* unary_exp = dynamic_cast<Unary*>(to_evaluate.get());

		return this->get_expression_data_type(unary_exp->get_operand());
	}
	else if (to_evaluate->get_expression_type() == BINARY) {
		Binary* binary_exp = dynamic_cast<Binary*>(to_evaluate.get());

		Type return_type = this->get_expression_data_type(binary_exp->get_left(), true);
		return return_type;
	}
	else if (to_evaluate->get_expression_type() == DEREFERENCED) {
		Dereferenced* dereferenced_exp = dynamic_cast<Dereferenced*>(to_evaluate.get());

		return this->get_expression_data_type(dereferenced_exp->get_ptr_shared(), true);
	}
	else if (to_evaluate->get_expression_type() == SIZE_OF) {
		return INT;	// sizeof(...) always returns an unsigned int
	}
	else {
		return NONE;
	}
}

std::stringstream Compiler::evaluate_binary_tree(Binary bin_exp, unsigned int line_number, size_t* stack_offset, size_t max_offset, Type right_type)
{
	std::stringstream binary_ss;

	Binary current_tree = bin_exp;
	Expression* left_exp = current_tree.get_left().get();
	Expression* right_exp = current_tree.get_right().get();

	/*
	
	The binary evaluation algorithm works as follows:
		1. Look at the right operand
			A. If the operand is another binary tree, call the function recursively on that tree (returning to step one)
		2. Now look at the left-hand operand
			A. If the operand is another binary tree, call the function recursively on that tree (returning to step one)
		3. Evaluate the expression
			A. If the result was the left_exp, keep it in A
			B. If the result was the right_exp, transfer it to B
	
	*/

	if (right_exp->get_expression_type() == BINARY) {
		// cast to Binary expression type and call this function recursively
		Binary* right_operand = dynamic_cast<Binary*>(right_exp);
		binary_ss << this->evaluate_binary_tree(*right_operand, line_number, stack_offset, max_offset, right_type).str();
		binary_ss << "\t" << "tab" << std::endl;
	}

	// once we reach this, the right expression is not binary
	if (right_exp->get_expression_type() == LVALUE || right_exp->get_expression_type() == INDEXED || right_exp->get_expression_type() == LITERAL || right_exp->get_expression_type() == DEREFERENCED) {
		// update right_type
		bool get_sub = right_exp->get_expression_type() == INDEXED;
		right_type = this->get_expression_data_type(current_tree.get_right(), get_sub);

		binary_ss << this->fetch_value(bin_exp.get_right(), line_number, stack_offset, max_offset).str();
		binary_ss << "\t" << "tab" << std::endl;
	}
	else if (right_exp->get_expression_type() == UNARY) {
		Unary* unary_operand = dynamic_cast<Unary*>(right_exp);

		right_type = get_expression_data_type(unary_operand->get_operand());

		// signed arithmetic will depend on the evaluation of a unary expression; if there are no unaries, there is no sign
		// if the quality is "minus", set the "N" flag
		if (unary_operand->get_operator() == MINUS) {
			binary_ss << "\t" << "tay" << std::endl;
			binary_ss << "\t" << "tstatusa" << std::endl;
			binary_ss << "\t" << "ora #%10000000" << std::endl;
			binary_ss << "\t" << "tastatus" << std::endl;
			binary_ss << "\t" << "tya" << std::endl;
		}

		binary_ss << this->evaluate_unary_tree(*unary_operand, line_number, stack_offset, max_offset).str();
		binary_ss << "\t" << "tab" << std::endl;
	}

	if (left_exp->get_expression_type() == BINARY) {
		// first, we must push the B register
		binary_ss << "\t" << "phb" << std::endl;

		// cast to Binary and call this function recursively
		Binary* left_operand = dynamic_cast<Binary*>(left_exp);
		binary_ss << this->evaluate_binary_tree(*left_operand, line_number, stack_offset, max_offset, right_type).str();

		// now we can pull the b register again
		binary_ss << "\t" << "plb" << std::endl;
	}
	else {
		// check to make sure the type matches
		Type left_type = this->get_expression_data_type(current_tree.get_left());

		if (left_type == right_type) {
			if (left_exp->get_expression_type() == LVALUE || left_exp->get_expression_type() == LITERAL || left_exp->get_expression_type() == DEREFERENCED) {
				binary_ss << this->fetch_value(bin_exp.get_left(), line_number, stack_offset, max_offset).str();
			}
			else if (right_exp->get_expression_type() == UNARY) {
				Unary* unary_operand = dynamic_cast<Unary*>(left_exp);
				binary_ss << this->evaluate_unary_tree(*unary_operand, line_number, stack_offset, max_offset).str();
			}
		}
		else {
			throw CompilerException("Types in binary expression do not match!", 0 , line_number);
		}
	} 

	// set STATUS bits accordingly
	if (right_type == FLOAT) {
		binary_ss << "\n\t" << "; Set 'float' bit in STATUS" << std::endl;
		binary_ss << "\t" << "tay" << std::endl;
		binary_ss << "\t" << "tstatusa" << std::endl;
		binary_ss << "\t" << "ora #%00000100" << std::endl;	// set the F bit in STATUS
		binary_ss << "\t" << "tastatus" << std::endl;
		binary_ss << "\t" << "tya" << std::endl << std::endl;
	}

	// now, we can perform the operation on the two operands
	if (bin_exp.get_operator() == PLUS) {
		binary_ss << "\t" << "addca b" << std::endl;
	}
	else if (bin_exp.get_operator() == MINUS) {
		binary_ss << "\t" << "subca b" << std::endl;
	}
	else if (bin_exp.get_operator() == MULT) {
		binary_ss << "\t" << "jsr __builtins_multiply" << std::endl;
	}
	else if (bin_exp.get_operator() == DIV) {
		binary_ss << "\t" << "jsr __builtins_divide" << std::endl;
	}
	else if (bin_exp.get_operator() == EQUAL) {
		binary_ss << "\t" << "jsr __builtins_equal" << std::endl;
	}
	else if (bin_exp.get_operator() == GREATER) {
		binary_ss << "\t" << "jsr __builtins_greater" << std::endl;
	}
	else if (bin_exp.get_operator() == GREATER_OR_EQUAL) {
		binary_ss << "\t" << "jsr __builtins_gt_equal" << std::endl;
	}
	else if (bin_exp.get_operator() == NOT_EQUAL) {
		binary_ss << "\t" << "jsr __builtins_equal" << std::endl;
		binary_ss << "\t" << "xora #$01" << std::endl;	// simply perform an equal operation and flip the bit
	}
	else if (bin_exp.get_operator() == LESS) {
		binary_ss << "\t" << "jsr __builtins_less" << std::endl;
	}
	else if (bin_exp.get_operator() == LESS_OR_EQUAL) {
		binary_ss << "\t" << "jsr __builtins_lt_equal" << std::endl;
	}
	// TODO: add AND/OR binary operators
	
	return binary_ss;
}

std::stringstream Compiler::evaluate_unary_tree(Unary unary_exp, unsigned int line_number, size_t* stack_offset, size_t max_offset)
{
	std::stringstream unary_ss;

	// TODO: evaluate unary

	/*
	
	The available unary operators are:
		+	PLUS	Does nothing
		-	MINUS	Negative value
		!	NOT		Not operator
	
	*/
	
	// First, we need the fetch the operand and load it into register A -- how we do this depends on the type of expression in our unary
	if (unary_exp.get_operand()->get_expression_type() == LITERAL) {
		// cast the operand to a literal type
		Literal* unary_operand = dynamic_cast<Literal*>(unary_exp.get_operand().get());

		// act according to its data type
		if (unary_operand->get_type() == BOOL) {
			if (unary_operand->get_value() == "True") {
				unary_ss << "\tloada #$01" << std::endl;
			}
			else if (unary_operand->get_value() == "False") {
				unary_ss << "\tloada #$00" << std::endl;
			}
			else {
				throw CompilerException("Invalid boolean literal value.", 0, line_number);
			}
		}
		else if (unary_operand->get_type() == INT) {
			unary_ss << "\t" << "loada #$" << std::hex << std::stoi(unary_operand->get_value()) << std::endl;
		}
		else if (unary_operand->get_type() == FLOAT) {
			// TODO: handle floats
		}
		else if (unary_operand->get_type() == STRING) {
			// define the string constant
			unary_ss << "@db __STRC__NUM_" << std::dec << this->strc_number << " (" << unary_operand->get_value() << ")" << std::endl;
			
			// load our registers with the appropriate values
			unary_ss << "\t" << "loada #$" << std::hex << unary_operand->get_value().length() << std::endl;
			unary_ss << "\t" << "loadb #" << "__STRC__NUM_" << std::dec << this->strc_number << std::endl;

			// increment strc_number so we can continue to define string literals
			this->strc_number++;
		}
	}
	else if (unary_exp.get_operand()->get_expression_type() == LVALUE) {
		// if we have an lvalue in the unary expression, simply fetch the lvalue
		unary_ss << this->fetch_value(unary_exp.get_operand(), line_number, stack_offset, max_offset).str();
	}
	else if (unary_exp.get_operand()->get_expression_type() == BINARY) {
		// TODO: parse binary tree to get the result in A
	}
	else if (unary_exp.get_operand()->get_expression_type() == UNARY) {
		// Our unary operand can be another unary expression -- if so, simply get the operand and call this function recursively
		Unary* unary_operand = dynamic_cast<Unary*>(unary_exp.get_operand().get());	// cast to appropriate type
		unary_ss << this->evaluate_unary_tree(*unary_operand, line_number, stack_offset, max_offset).str();	// add the produced code to our code here
	}

	// Now that the A register contains the value of the operand

	if (unary_exp.get_operator() == PLUS) {
		// the unary plus operator does nothing
	}
	else if (unary_exp.get_operator() == MINUS) {
		// the unary minus operator will flip every bit and then add 1 to it (to get two's complement) -- this works to convert both ways
		// the N and Z flags will automatically be set or cleared by the ADDCA instruction

		unary_ss << "\t" << "xora #$FFFF" << std::endl;	// using XOR on A with the value 0xFFFF will flip all bits (0110 XOR 1111 => 1001)
		unary_ss << "\t" << "addca #$01" << std::endl;	// adding 1 finishes two's complement
	}
	else if (unary_exp.get_operator() == NOT) {
		// 0 is the only value considered to be false; all other values are true -- as such, the not operator will set +/- values to 0, and 0 to 1
		unary_ss << "\t" << "cmpa #$00" << std::endl;	// compare A to 0
		unary_ss << "\t" << "breq .NOT__add" << std::endl;

		unary_ss << "\t" << "loada #$00" << std::endl;	// if not 0, set it to 0
		unary_ss << "\t" << "jmp .NOT__done" << std::endl;

		unary_ss << ".NOT__add:" << std::endl;
		unary_ss << "\t" << "loada #$01" << std::endl;	// if 0, load a with the value 1

		unary_ss << ".NOT__done:" << std::endl;	// our "done" label
	}
	else {
		// if it is not one of the aforementioned operators, it is an invalid unary operator
		throw CompilerException("Invalid operator in unary expression.", 0, line_number);
	}

	return unary_ss;
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
				for (std::vector<Symbol>::iterator it = include_compiler->symbol_table.symbols.begin(); it != include_compiler->symbol_table.symbols.end(); it++) {
					this->symbol_table.insert(it->name, it->type, it->scope_name, it->scope_level, it->sub_type, it->quality, it->defined, it->formal_parameters, include_statement.get_line_number());
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
			fetch_ss << "\t" << "loada #$" << std::hex << std::stoi(literal_expression->get_value()) << std::endl;
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
				throw CompilerException("Expected 'True' or 'False' as boolean literal value (case matters!)", 0, line_number);
			}

			fetch_ss << "\t" << "loada #$" << std::hex << bool_expression_as_int << std::endl;
		}
		else if (literal_expression->get_type() == FLOAT) {
			// floats are unique in SIN in that they are the only data type that is larger than the machine's word size; even in 16-bit SIN, floats are 32 bits
			// copy the bits of the float value into a uint32_t
			float literal_value = std::stof(literal_expression->get_value());
			uint32_t converted_value;
			memcpy(&converted_value, &literal_value, sizeof(uint32_t));

			// set the F bit in the status register using Y as our temp variable
			fetch_ss << "\t" << "tay" << std::endl;
			fetch_ss << "\t" << "tstatusa" << std::endl;
			fetch_ss << "\t" << "ora #%00000100" << std::endl;
			fetch_ss << "\t" << "tastatus" << std::endl;
			fetch_ss << "\t" << "tya" << std::endl;

			// TODO: continue implementing floating-point -- implement a 'half' type? Dynamically allocate floats?
		}
		else if (literal_expression->get_type() == STRING) {
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
	else if (to_fetch->get_expression_type() == LVALUE || to_fetch->get_expression_type() == INDEXED) {
		// get the lvalue's symbol data
		Symbol* variable_symbol;

		if (to_fetch->get_expression_type() == LVALUE) {
			LValue* variable_to_get = dynamic_cast<LValue*>(to_fetch.get());
			variable_symbol = this->symbol_table.lookup(variable_to_get->getValue(), this->current_scope_name, this->current_scope);
		}
		else {
			Indexed* variable_to_get = dynamic_cast<Indexed*>(to_fetch.get());
			variable_symbol = this->symbol_table.lookup(variable_to_get->getValue(), this->current_scope_name, this->current_scope);

			// now, use fetch_value to get the index value in the A register
			fetch_ss << this->fetch_value(variable_to_get->get_index_value(), line_number, stack_offset, max_offset).str();
		}

		// only fetch the value if it has been defined
		if (variable_symbol->defined) {
			
			// check the scope; we need to do different things for global and local scopes
			if ((variable_symbol->scope_name == "global") && (variable_symbol->scope_level == 0)) {
				
				// all we need to do is use the variable name for globals; however, we need to know the type
				if (variable_symbol->quality == DYNAMIC) {

					// ensure that the dynamic memory has not been freed
					if (variable_symbol->freed) {
						throw CompilerException("Cannot reference dynamic memory that has already been freed", 0, line_number);
					}
					else {
						// if we are indexing
						if (to_fetch->get_expression_type() == INDEXED) {
							// transfer the index offset from A to B
							fetch_ss << "\t" << "tab" << std::endl;

							// load a with the address
							fetch_ss << "\t" << "loada " << variable_symbol->name << std::endl;

							// add the index offset, transfer to b, and increment it by 2 (to skip the length)
							fetch_ss << "\t" << "addca b" << std::endl;
							fetch_ss << "\t" << "tab" << "\n\t" << "incb" << "\n\t" << "incb" << std::endl;

							// finally, load a with 1 -- indexing will give one character
							fetch_ss << "\t" << "loada #$01" << std::endl;

							// now, we are done; A and B contain the proper information
						}
						else {
							// first, we need to load the length of the string, which is the first word at the address of the string
							fetch_ss << "\t" << "loadx #$00" << std::endl;
							fetch_ss << "\t" << "loada (" << variable_symbol->name << "), x" << std::endl;

							// we need to get the address in B, but we must also increment B by two so we don't load the length as a character
							fetch_ss << "\t" << "loadb " << variable_symbol->name << std::endl;
							fetch_ss << "\t" << "incb" << "\n\t" << "incb" << std::endl;

							// Now we are done; A and B contain the proper information
						}
					}
				}
				else {
					// if we are loading an indexed value, index it
					if (to_fetch->get_expression_type() == INDEXED) {
						// multiply the index offset by 2 with lsl and transfer it from A to Y
						fetch_ss << "\t" << "lsl a" << std::endl;
						fetch_ss << "\t" << "tay" << std::endl;
						fetch_ss << "\t" << "loada " << variable_symbol->name << ", y" << std::endl;
					}
					else {
						fetch_ss << "\t" << "loada " << variable_symbol->name << std::endl;
					}
				}
			}
			else {
				fetch_ss << this->move_sp_to_target_address(stack_offset, variable_symbol->stack_offset + 1).str();
				
				// now, the offsets are the same; get the variable's value
				if (variable_symbol->quality == DYNAMIC) {
					if (variable_symbol->freed) {
						throw CompilerException("Cannot reference dynamic memory that has already been freed", 0, line_number);
					}
					else {
						// if we are indexing the string, get the value within the string in A
						if (to_fetch->get_expression_type() == INDEXED) {
							// TODO: index local string
						}
						else {
							// dynamic variables must use pointer dereferencing
							// so we pull the address of the string into B
							fetch_ss << "\t" << "plb" << std::endl;
							(*stack_offset) -= 1;

							// now, we must get the value at the address contained in B -- use the X register for this -- which is our string length
							fetch_ss << "\t" << "tba" << "\n\t" << "tax" << std::endl;	// simply index -- we _already dereferenced the pointer_, this address does not contain another address to dereference, but rather the data we want
							fetch_ss << "\t" << "loada $00, X" << std::endl;

							// now, increment B by 2 so that we point to the correct byte
							fetch_ss << "\t" << "incb" << "\n\t" << "incb" << std::endl;

							// Now, A and B contain the correct values
						}
					}
				}
				else {
					if (to_fetch->get_expression_type() == INDEXED) {
						/*
						
						Indexed local arrays must be accessed as follows:
							1) Get the index value in A
							2) Multiply by 2 with LSL (2 bytes per word, each index number is 1 word)
							3) Transfer to B
							4) Transfer the SP into A
							5) Subtract B from A (stack grows downward)
							6) Transfer back
							7) Pull the value
							8) Decrement the SP and add B back to it, so it is where it was before (so we don't screw up where the compiler thinks the SP is)
						
						*/
						
						// adjust the index
						fetch_ss << "\t" << "lsl a" << std::endl;	// index value ends up in A, so we can multiply right away
						fetch_ss << "\t" << "tab" << std::endl;

						// adjust the SP
						fetch_ss << "\t" << "tspa" << std::endl;
						fetch_ss << "\t" << "subca b" << std::endl;
						fetch_ss << "\t" << "tasp" << std::endl;

						// get the value
						fetch_ss << "\t" << "pla" << std::endl;
						fetch_ss << "\t" << "decsp" << std::endl;
						fetch_ss << "\t" << "tax" << std::endl;

						// re-adjust the SP
						fetch_ss << "\t" << "tspa" << std::endl;
						fetch_ss << "\t" << "addca b" << std::endl;
						fetch_ss << "\t" << "tasp" << std::endl;
						fetch_ss << "\t" << "txa" << std::endl;
					}
					else {
						// pull the value into the A register
						fetch_ss << "\t" << "pla" << std::endl;
						(*stack_offset) -= 1;	// we pulled from the stack, so we must adjust the stack offset
					}
				}
			}
		}
		else {
			throw CompilerException("Variable '" + variable_symbol->name + "' referenced before assignment", 0, line_number);
		}
	}
	else if (to_fetch->get_expression_type() == DEREFERENCED) {
		// TODO: get the value of a dereferenced variable
		Dereferenced* dereferenced_exp = dynamic_cast<Dereferenced*>(to_fetch.get());;
		
		if (dereferenced_exp->get_ptr_shared()->get_expression_type() == DEREFERENCED) {
			fetch_ss << this->fetch_value(dereferenced_exp->get_ptr_shared(), line_number, stack_offset, max_offset).str();
		}
		else {
			// check to make sure the variable is in the symbol table
			if (this->symbol_table.is_in_symbol_table(dereferenced_exp->get_ptr().getValue(), this->current_scope_name)) {
				// if the variable is in the symbol table, only use recursion if the type is still a pointer
				Symbol* referenced_var = this->symbol_table.lookup(dereferenced_exp->get_ptr().getValue(), this->current_scope_name, this->current_scope);
				fetch_ss << "\t" << "loady #$00" << std::endl;
				fetch_ss << "\t" << "loada (" << referenced_var->name << "), y" << std::endl;
			}
			else {
				throw CompilerException("Could not find '" + dereferenced_exp->get_ptr().getValue() + "' in symbol table; perhaps it is out of scope?", 0, line_number);
			}
		}
	}
	else if (to_fetch->get_expression_type() == ADDRESS_OF) {
		// TODO: get the address of a variable
		// dynamic cast to AddressOf and get the variable's symbol from the symbol table
		AddressOf* address_of_exp = dynamic_cast<AddressOf*>(to_fetch.get());	// the AddressOf expression
		LValue address_to_get = address_of_exp->get_target();	// the actual LValue of the address we want
		Symbol* variable_symbol = this->symbol_table.lookup(address_to_get.getValue(), this->current_scope_name, this->current_scope);

		// make sure the variable was defined
		if (variable_symbol->defined) {
			if (variable_symbol->quality == DYNAMIC) {
				// Getting the address of a dynamic variable is easy -- we simply move the stack pointer to the proper byte, pull the value into A

				if (variable_symbol->freed) {
					throw CompilerException("Cannot reference dynamic memory that has already been freed", 0, line_number);
				}
				else {
					fetch_ss << this->move_sp_to_target_address(stack_offset, variable_symbol->stack_offset + 1).str();

					// now that the stack pointer is in the right place, we can pull the value into A
					fetch_ss << "\t" << "pla" << std::endl;
					*stack_offset -= 1;
				}
			}
			else {
				if (variable_symbol->scope_name == "global") {
					fetch_ss << "\t" << "loada #" << variable_symbol->name << std::endl;	// using  "loada var" would mean "load the A register with the value at address 'var' " while "loada #var" means "load the A register with the address of 'var' "
				}
				else {
					fetch_ss << this->move_sp_to_target_address(stack_offset, variable_symbol->stack_offset + 1).str();

					// now that the stack pointer is in the proper place to pull the variable from, increment it by one place and transfer the pointer value to A; that is the address where the variable we want lives
					(*stack_offset) -= 1;
					fetch_ss << "\t" << "incsp" << std::endl;
					fetch_ss << "\t" << "tspa" << std::endl;
				}
			}
		}
		else {
			throw CompilerException("Variable '" + variable_symbol->name + "' referenced before assignment", 0, line_number);
		}
	}
	else if (to_fetch->get_expression_type() == UNARY) {
		Unary* unary_expression = dynamic_cast<Unary*>(to_fetch.get());
		fetch_ss << this->evaluate_unary_tree(*unary_expression, line_number, stack_offset, max_offset).str();
	}
	else if (to_fetch->get_expression_type() == BINARY) {
		Binary* binary_expression = dynamic_cast<Binary*>(to_fetch.get());
		fetch_ss << this->evaluate_binary_tree(*binary_expression, line_number, stack_offset, max_offset).str();
	}
	else if (to_fetch->get_expression_type() == VALUE_RETURNING_CALL) {
		ValueReturningFunctionCall* val_ret = dynamic_cast<ValueReturningFunctionCall*>(to_fetch.get());

		// now, search through the symbol table for the function so we can get its return type
		Symbol* function_symbol = this->symbol_table.lookup(val_ret->get_name()->getValue());

		// increment the stack pointer to the stack frame
		fetch_ss << this->move_sp_to_target_address(stack_offset, max_offset).str();

		// call the function
		Call to_call(val_ret->get_name(), val_ret->get_args());	// create a 'call' object from val_ret
		fetch_ss << this->call(to_call, stack_offset, max_offset).str();	// add that to the asm

		// now, the returned value will be on the stack; if the type is of variable length (array or struct), handle it separately
		if (function_symbol->type != ARRAY && function_symbol->type != STRUCT) {
			// the size of the return symbol is only one word; pull it from the stack
			fetch_ss << "\t" << "pla" << std::endl;
		}
		else if (function_symbol->type == STRUCT) {
			// TODO: implement structs
		}
		else {
			// TODO: implement arrays
		}
	}
	else if (to_fetch->get_expression_type() == SIZE_OF) {
		// cast to SizeOf type
		SizeOf* size_of = dynamic_cast<SizeOf*>(to_fetch.get());
		std::string to_check = size_of->get_type();

		// if it's an int, bool, float, string, raw, or pointer, we know the size
		if (to_check == "int" || to_check == "bool" || to_check == "float" || to_check == "string" || to_check == "ptr" || to_check == "raw") {
			fetch_ss << "loada #$02" << std::endl;
		}
		// otherwise, it's a struct, and we must check our structs to see what the size is
		else {
			// TODO: struct implementation
		}
	}
	else {
		throw CompilerException("Cannot fetch expression", 0, line_number);
	}

	return fetch_ss;
}

std::stringstream Compiler::move_sp_to_target_address(size_t * stack_offset, size_t target_offset, bool preserve_registers)
{
	/*
	
	Increment the stack pointer to the end of the current stack frame so that we can enter a new scope.

	If the register values do not need to be preserved, the function will 
	
	*/
	std::stringstream inc_ss;

	if (stack_offset) {
		// increment the stack pointer to the end of the current stack frame
		if (*stack_offset < target_offset) {
			// if we need to increment more than three times, use a transfer and add -- otherwise, it's efficient enough to use decsp; we will also elect not to use this method if we must preserve our register values
			if ((target_offset - *stack_offset > 3) && !preserve_registers) {
				// transfer the stack pointer to a, add the difference, and transfer it back
				size_t difference = target_offset - *stack_offset;
				inc_ss << "\t" << "tspa" << "\t; increment sp by using transfers and addca" << std::endl;
				inc_ss << "\t" << "addca #$" << std::hex << difference << std::endl;
				inc_ss << "\t" << "tasp" << std::endl;

				*stack_offset = target_offset;
			}
			else {
				while (*stack_offset < target_offset) {
					inc_ss << "\t" << "decsp" << std::endl;
					*stack_offset += 1;
				}
			}
		}
		else if (*stack_offset > target_offset) {
			// do the reverse here of what we did above
			if ((*stack_offset - target_offset > 3) && !preserve_registers) {
				// subtract the difference between the stack offset and the target offset from the stack offset
				size_t difference = *stack_offset - target_offset;
				inc_ss << "\t" << "tspa" << std::endl;
				inc_ss << "\t" << "subca #$" << std::hex << difference << std::endl;
				inc_ss << "\t" << "tspa" << std::endl;
			}
			else {
				while (*stack_offset > target_offset) {
					inc_ss << "\t" << "incsp" << std::endl;
					*stack_offset -= 1;
				}
			}
		}
		else {
			// the values must be equal, so do nothing
		}
	}
	else {
		std::cerr << "**** Warning: 'stack_offset' was nullptr in 'incsp_to_stack_frame'!" << std::endl;
	}

	return inc_ss;
}

std::stringstream Compiler::string_assignment(Symbol target_symbol, std::shared_ptr<Expression> rvalue, unsigned int line_number, size_t * stack_offset, size_t max_offset)
{
	std::stringstream string_assign_ss;

	// fetch the rvalue -- A will contain the length, B will contain the address
	string_assign_ss << this->fetch_value(rvalue, line_number, stack_offset, max_offset).str();

	// move the stack pointer to the end of the stack frame
	if (*stack_offset != max_offset) {
		// transfer A and B to X and Y before incrementing the stack pointer
		string_assign_ss << "\t" << "tax" << "\n\t" << "tba" << "\n\t" << "tay" << std::endl;

		// increment the stack pointer to the end of the stack frame so we can use the stack
		string_assign_ss << this->move_sp_to_target_address(stack_offset, max_offset).str();

		// move X and Y back into A and B
		string_assign_ss << "\t" << "tya" << "\n\t" << "tab" << "\n\t" << "txa" << std::endl;
	}

	string_assign_ss << "\t" << "phb" << std::endl;	// push the address of the string variable

	// add some padding to the string length
	string_assign_ss << "\t" << "pha" << std::endl;
	string_assign_ss << "\t" << "addca #$10" << std::endl;

	// next, create the system call to allocate the memory on the heap
	string_assign_ss << "\t" << "syscall #$21" << std::endl;

	// TODO: we need to ensure that we don't allocate new memory every time we have a string assignment, but rather only reallocate memory for the object when it is necessary

	// Global and local variables must be handled differently
	if (target_symbol.scope_level == 0) {
		string_assign_ss << "\t" << "storeb " << target_symbol.name << std::endl;	// store the address in our pointer variable

		// get the original value of A -- the actual string length -- back
		string_assign_ss << "\t" << "pla" << std::endl;

		// next, store the length in the heap
		string_assign_ss << "\t" << "loady #$00" << std::endl;
		string_assign_ss << "\t" << "storea (" << target_symbol.name << "), y" << std::endl;

		// next, get the value of our pointer and increment by 2 for memcpy
		string_assign_ss << "\t" << "loada " << target_symbol.name << std::endl;
		string_assign_ss << "\t" << "addca #$02" << std::endl;

		// the address of the string variable has already been pushed, so the next thing to push is the address of our destination
		string_assign_ss << "\t" << "pha" << std::endl;

		// next, we need to push the length of the string, which we will get from our pointer variable
		string_assign_ss << "\t" << "loada (" << target_symbol.name << "), y" << std::endl;
		string_assign_ss << "\t" << "pha" << std::endl;
	}
	else {
		*stack_offset += 2;	// increment our stack offset so we can decrement correctly
		size_t previous_offset = *stack_offset;	// we want to ensure that we know exactly where to return back to

		// now, we must move the stack pointer to the pointer variable; we don't need to set the retain registers flag because the addca method does not touch the B register and we have nothing valuable in A
		string_assign_ss << this->move_sp_to_target_address(stack_offset, target_symbol.stack_offset).str();

		// store the address of the dynamic memory in _LOCAL_DYNAMIC_POINTER in addition to putting it on the stack
		string_assign_ss << "\t" << "phb" << std::endl;
		*stack_offset += 1;	// increase the stack offset so the compiler navigates the stack properly

		string_assign_ss << "\t" << "storeb $" << std::hex << _LOCAL_DYNAMIC_POINTER << std::endl;

		// move the stack pointer back to where it was
		string_assign_ss << this->move_sp_to_target_address(stack_offset, previous_offset).str();

		// pull the length back into A and store it
		string_assign_ss << "\t" << "pla" << std::endl;

		string_assign_ss << "\t" << "loady #$00" << std::endl;
		string_assign_ss << "\t" << "storea ($" << std::hex << _LOCAL_DYNAMIC_POINTER << "), y" << std::endl;

		// get the address of the dynamic memory and increment it by 2, for memcpy
		string_assign_ss << "\t" << "loada $" << _LOCAL_DYNAMIC_POINTER << std::endl;
		string_assign_ss << "\t" << "addca #$02" << std::endl;

		// push the arguments to the stack
		string_assign_ss << "\t" << "pha" << std::endl;
		string_assign_ss << "\t" << "loada ($" << std::hex << _LOCAL_DYNAMIC_POINTER << "), y" << std::endl;
		string_assign_ss << "\t" << "pha" << std::endl;

		*stack_offset -= 2;
	}

	// finally, invoke the subroutine
	string_assign_ss << "\t" << "jsr __builtins_memcpy" << std::endl;

	// the memory has been successfully copied over; our assignment is done, so we can return the assembly we generated
	return string_assign_ss;
}


// allocate a variable
std::stringstream Compiler::allocate(Allocation allocation_statement, size_t* stack_offset, size_t* max_offset) {
	// first, add the variable to the symbol table
	// next, create a macro for it and assign it to the next available address in the scope; local variables will use pages incrementally
	// next, every time a variable is referenced, make sure it is in the symbol table and within the appropriate scope
	// after that, simply use that variable's name as a macro in the code
	// the macro will reference its address and hold the proper value

	std::stringstream allocation_ss;

	std::shared_ptr<Expression> initial_value = allocation_statement.get_initial_value();
	Symbol to_allocate(allocation_statement.get_var_name(), allocation_statement.get_var_type(), current_scope_name, current_scope, allocation_statement.get_var_subtype(), allocation_statement.get_quality(), allocation_statement.was_initialized(), {}, allocation_statement.get_array_length());

	// if we have a const, we can use the "@db" directive
	if (to_allocate.quality == CONSTANT) {
		this->symbol_table.insert(to_allocate, allocation_statement.get_line_number());	// constants have no need for the stack; we can add the symbol right away

		// constants must initialized when they are allocated (i.e. they must use alloc-assign syntax)
		if (to_allocate.defined) {
			// get the initial value's expression type and handle it accordingly
			if (initial_value->get_expression_type() == LITERAL) {
				// literal values are easy
				Literal* const_literal = dynamic_cast<Literal*>(initial_value.get());

				// make sure the types match
				if (to_allocate.type == const_literal->get_type()) {
					std::string const_value = const_literal->get_value();

					// use "@db"
					allocation_ss << "@db " << to_allocate.name << " (" << const_value << ")" << std::endl;
				}
				else {
					throw CompilerException("Types do not match", 0, allocation_statement.get_line_number());
				}
			}
			else if (initial_value->get_expression_type() == LVALUE) {
				// dynamic cast to lvalue
				LValue* initializer_lvalue = dynamic_cast<LValue*>(initial_value.get());

				// look through the symbol table to get the symbol
				Symbol* initializer_symbol = this->symbol_table.lookup(initializer_lvalue->getValue(), this->current_scope_name, this->current_scope);	// this will throw an exception if the object isn't in the symbol table

				// verify the symbol is defined
				if (initializer_symbol->defined) {
					// verify the types match -- we don't need to worry about subtypes just yet
					if (initializer_symbol->type == to_allocate.type) {
						// define the constant using a dummy value
						allocation_ss << "@db " << to_allocate.name << " (0)" << std::endl;

						// different data types must be treated differently
						if (to_allocate.type == STRING) {
							/*
							
							To allocate a string constant, we must fetch the value and then utilize memcpy to copy the data from our old location to the new one.

							Usage for memcpy is as follows:
								- Push source
								- Push destination
								- Push number of bytes to copy

							After fetch_value is called, A will contain the number of _bytes_, B will contain the address of the string

							*/

							allocation_ss << this->fetch_value(initial_value, allocation_statement.get_line_number(), stack_offset, *max_offset).str();
							allocation_ss << this->move_sp_to_target_address(stack_offset, *max_offset, true).str();	// increment the stack pointer to our stack frame, but preserve our register values

							// push our parameters and invoke our memcpy subroutine
							allocation_ss << "\t" << "phb" << "\n\t" << "loadb #" << to_allocate.name << "\n\t" << "phb" << "\n\t" << "pha" << std::endl;
							allocation_ss << "\t" << "jsr __builtins_memcpy" << std::endl;
						}
						// TODO: add the array type here as well, once that is implemented in SIN
						else {
							// now, use fetch_value to get the value of our lvalue and store the value in A at the constant we have defined
							allocation_ss << this->fetch_value(initial_value, allocation_statement.get_line_number(), stack_offset, *max_offset).str();
							allocation_ss << "storea " << to_allocate.name << std::endl;
						}
					}
				}
				else {
					throw CompilerException("'" + initializer_symbol->name + "' was referenced before assignment.", 0, allocation_statement.get_line_number());
				}
			}
			else if (initial_value->get_expression_type() == DEREFERENCED) {
				Dereferenced* initializer_deref = dynamic_cast<Dereferenced*>(initial_value.get());
				// TODO: implement dereferenced constant initializer
			}
			else if (initial_value->get_expression_type() == ADDRESS_OF) {
				// TODO: implement address_of constant initializer
			}
			else if (initial_value->get_expression_type() == UNARY) {
				Unary* initializer_unary = dynamic_cast<Unary*>(initial_value.get());
				// if the types match
				if (this->get_expression_data_type(initial_value) == to_allocate.type) {
					allocation_ss << "@db " << to_allocate.name << " (0)" << std::endl;
					// get the evaluated unary
					this->evaluate_unary_tree(*initializer_unary, allocation_statement.get_line_number(), stack_offset, *max_offset);	// evaluate the unary expression
					allocation_ss << "storea " << to_allocate.name << std::endl;
				}
				else {
					throw CompilerException("Types do not match", 0, allocation_statement.get_line_number());
				}
			}
			else if (initial_value->get_expression_type() == BINARY) {
				Binary* initializer_binary = dynamic_cast<Binary*>(initial_value.get());
				// if the types match
				if (this->get_expression_data_type(initial_value) == to_allocate.type) {
					allocation_ss << "@db " << to_allocate.name << " (0)";
					this->evaluate_binary_tree(*initializer_binary, allocation_statement.get_line_number(), stack_offset, *max_offset);
					allocation_ss << "storea " << to_allocate.name << std::endl;
				}
				else {
					throw CompilerException("Types do not match", 0, allocation_statement.get_line_number());
				}
			}
			// TODO: add other const initializer expressions
		}
		else {
			// this error should have been caught by the parser, but just to be safe...
			throw CompilerException("Const-qualified variables must be initialized in allocation", 0, allocation_statement.get_line_number());
		}
	}

	// handle all non-const global variables
	else if (current_scope_name == "global" && current_scope == 0) {
		this->symbol_table.insert(to_allocate, allocation_statement.get_line_number());	// global variables do not need the stack, so add to the symbol table right away

		// we must specify how many bytes to reserve depending on the data type; structs and arrays will use different sizes, but all other data types will use one word
		if (to_allocate.type == ARRAY) {
			// arrays may only contain the integral types; you can have an array of pointers to arrays or structs, but not of arrays or structs themselves (at this time)
			if (to_allocate.sub_type == ARRAY || to_allocate.sub_type == STRUCT) {
				throw CompilerException("Arrays with subtype 'array' or 'struct' are not supported in SIN at this time", 0, allocation_statement.get_line_number());
			}
			else {
				// reserve one word for each element
				size_t num_bytes = to_allocate.array_length * 2;
				allocation_ss << "@rs " << std::dec << num_bytes << " " << to_allocate.name << std::endl;	// since we have been using std::hex, switch back to decimal mode here to be safe
			}
		}
		else if (to_allocate.type == STRUCT) {
			// TODO: implement structs
		}
		else {
			// reserve the variable itself first -- it may be a pointer to the variable if we need to dynamically allocate it
			allocation_ss << "@rs 2 " << to_allocate.name << std::endl;	// syntax is "@rs macro_name"

			// if the variable type is anything with a variable length, we need a different mechanism for handling them
			if (to_allocate.type == STRING) {
				// We can only allocate space dynamically if we have an initial value; we shouldn't guess on a size
				if (to_allocate.defined) {
					allocation_ss << this->string_assignment(to_allocate, initial_value, allocation_statement.get_line_number(), stack_offset, *max_offset).str();
				}
			}
			else {
				// check to see if we have alloc-assign syntax for our other data types
				if (to_allocate.defined) {
					allocation_ss << this->fetch_value(allocation_statement.get_initial_value(), allocation_statement.get_line_number(), stack_offset, *max_offset).str();
					allocation_ss << "\t" << "storea " << to_allocate.name << std::endl;
				}
			}
		}
	}

	// handle all non-const local variables
	else {
		// make sure we have valid pointers
		if ((stack_offset != nullptr) && (max_offset != nullptr)) {
			// our local variables will use the stack; they will directly modify the list of variable names and the stack offset
			allocation_ss << this->move_sp_to_target_address(stack_offset, *max_offset).str();	// make sure the stack pointer is at the end of the stack frame before allocating a local variable (so we don't overwrite anything)
			
			to_allocate.stack_offset = *stack_offset;	// the stack offset for the symbol will now be the current stack offset
			this->symbol_table.insert(to_allocate, allocation_statement.get_line_number());	// now that the symbol's stack offset has been determined, we can add the symbol to the table

			// If the variable is defined, we can push its initial value
			if (to_allocate.defined) {
				// we must handle dynamic memory differently
				if (to_allocate.quality == DYNAMIC) {
					// we don't need to check if the variable has been freed when we are allocating it
					if (to_allocate.type == STRING) {
						// allocate a word on the stack for the pointer
						*stack_offset += 1;
						*max_offset += 1;
						allocation_ss << "\t" << "decsp" << std::endl;

						// strings will use the member function for string assignment
						allocation_ss << this->string_assignment(to_allocate, initial_value, allocation_statement.get_line_number(), stack_offset, *max_offset).str();
					}
					// TODO: implement more dynamic memory types
				}
				else {
					// get the initial value
					allocation_ss << this->fetch_value(initial_value, allocation_statement.get_line_number(), stack_offset, *max_offset).str();
					// push the A register and increment our counters
					allocation_ss << "\t" << "pha" << std::endl;
					(*stack_offset) += 1;
					(*max_offset) += 1;
				}
			}
			else {
				// update the stack offset -- all types will increment the stack offset by one word; allocate space in the stack and increase our offset
				// Note that we push 0x00 onto the stack -- this is so that if we try to dereference the pointer there, it's truly a null pointer (the memory is not guaranteed to be initialized to 0x00 except at location 0x00)
				if (to_allocate.type == ARRAY) {
					allocation_ss << "\t" << "loada #$00" << std::endl;
					allocation_ss << "\t" << "loadx #$" << std::hex << to_allocate.array_length << std::endl;
					allocation_ss << ".__ALLOC_ARRAY_LOOP__BR_" << this->branch_number << "__:" << std::endl;
					allocation_ss << "\t" << "pha" << std::endl;
					allocation_ss << "\t" << "decx" << std::endl;
					allocation_ss << "\t" << "cmpx #$00" << std::endl;
					allocation_ss << "\t" << "brne .__ALLOC_ARRAY_LOOP__BR_" << this->branch_number << "__" << std::endl;

					this->branch_number += 1;
					(*stack_offset) += to_allocate.array_length;
					(*max_offset) += to_allocate.array_length;
				}
				else {
					allocation_ss << "\t" << "loada #$00" << std::endl;
					allocation_ss << "\t" << "pha" << std::endl;
					(*stack_offset) += 1;
					(*max_offset) += 1;
				}
			}
				
		}
		else {
			// if we forgot to supply the address of our counter, it will throw an exception
			throw CompilerException("Cannot allocate memory for variable; expected pointer to stack offset counter, but found 'nullptr' instead.");
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
		this->symbol_table.insert(func_name, return_type, "global", 0, NONE, NO_QUALITY, true, definition_statement.get_args(), definition_statement.get_line_number());
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
				this->symbol_table.insert(arg_alloc->get_var_name(), arg_alloc->get_var_type(), func_name, 1, arg_alloc->get_var_subtype(), arg_alloc->get_quality(), arg_alloc->was_initialized(), {}, definition_statement.get_line_number());	// this variable is only accessible inside this function's scope

				// TODO: add default parameters

				// note that the parameters will be pushed to the stack when called
				// so, they will be on the stack already when we jump to the label

				// however, we do need to increment the position in the stack according to what was pushed so we know how to navigate through our local variables, which will be on the stack for the function's lifetime; note we are not actually incrementing the pointer, we are just incrementing the variable that the compiler itself uses to track where we are in the stack
				if (arg_alloc->get_var_type() == ARRAY) {
					// TODO: implement arrays
				}
				else if (arg_alloc->get_var_type() == STRUCT) {
					// TODO: implement structs
				}
				else if (arg_alloc->get_var_type() == STRING) {
					// One word for length, one for address
					current_stack_offset += 2;
				}
				else {
					// all other types are one word
					current_stack_offset += 1;
				}
			}
			else {
				// currently, only alloc statements are allowed in function definitions
				throw CompilerException("Only allocation statements are allowed in function parameter definitions.", 0, definition_statement.get_line_number());
			}
		}
		
		// by this point, all of our function parameters are on the stack, in our symbol table, and our stack offset pointer tells us how many; as such, we can call the compile routine on our AST, as all of the functions for compilation will be able to handle scopes other than global
		StatementBlock function_procedure = *definition_statement.get_procedure().get();	// get the AST
		function_asm << this->compile_to_sinasm(function_procedure, 1, func_name, &current_stack_offset, max_stack_offset).str();	// compile it

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
		throw CompilerException("Function definitions must be in the global scope.", 0, definition_statement.get_line_number());
	}
}



std::stringstream Compiler::assign(Assignment assignment_statement, size_t* stack_offset, size_t max_offset) {

	exp_type lvalue_exp_type = assignment_statement.get_lvalue()->get_expression_type();

	// TODO: add support for double/triple/quadruple/etc. ref pointers

	// create a stringstream to which we will write write our generated code
	std::stringstream assignment_ss;
	LValue* assignment_lvalue;
	std::shared_ptr<Expression> assignment_index;	// if we have an index in our assignment
	std::string var_name = "";

	if (lvalue_exp_type == LVALUE) {
		assignment_lvalue = dynamic_cast<LValue*>(assignment_statement.get_lvalue().get());
		var_name = assignment_lvalue->getValue();
	}
	else if (lvalue_exp_type == INDEXED) {
		Indexed* indexed_lvalue = dynamic_cast<Indexed*>(assignment_statement.get_lvalue().get());
		var_name = indexed_lvalue->getValue();
		assignment_index = indexed_lvalue->get_index_value();
	}
	else if (lvalue_exp_type == DEREFERENCED) {
		std::shared_ptr<Expression> assign_lvalue = assignment_statement.get_lvalue();
		while (assign_lvalue->get_expression_type() == DEREFERENCED) {
			Dereferenced* deref = dynamic_cast<Dereferenced*>(assign_lvalue.get());
			assign_lvalue = deref->get_ptr_shared();
		}
		if (assign_lvalue->get_expression_type() == LVALUE) {
			assignment_lvalue = dynamic_cast<LValue*>(assign_lvalue.get());
			var_name = assignment_lvalue->getValue();
		}
		else {
			throw CompilerException("Error in parsing deref tree!", 0, assignment_statement.get_line_number());
		}
	}
	else {
		throw CompilerException("Cannot use expression of this type in lvalue!", 0, assignment_statement.get_line_number());
	}

	if (this->symbol_table.is_in_symbol_table(var_name, this->current_scope_name)) {
		// get the symbol information
		Symbol* fetched = this->symbol_table.lookup(var_name, this->current_scope_name, this->current_scope);
		
		// if the lvalue_exp_type is 'indexed', make sure 'fetched->type' is either a string or an array -- those are the only data types we can index
		if (lvalue_exp_type == INDEXED && ((fetched->type != STRING) && (fetched->type != ARRAY))) {
			throw CompilerException("Cannot index variables of this type", 0, assignment_statement.get_line_number());
		}

		// if the quality is "const" throw an error; we cannot make assignments to const-qualified variables
		if (fetched->quality == CONSTANT) {
			throw CompilerException("Cannot make an assignment to a const-qualified variable!", 0, assignment_statement.get_line_number());
		}
		// otherwise, make the assignment
		else {
			// get the anticipated type of the rvalue
			Type rvalue_data_type = this->get_expression_data_type(assignment_statement.get_rvalue());

			// if the types match, continue with the assignment
			if ((fetched->type == rvalue_data_type) || (fetched->sub_type == rvalue_data_type)) {

				// dynamic memory must be handled a little differently than automatic memory because under the hood, it is implemented through pointers
				if (fetched->quality == DYNAMIC) {
					// we don't need to check if the memory has been freed here -- we do that in string assignment
					if (fetched->type == STRING) {
						// check to make sure the type isn't indexed; if it is, throw an exception -- string index assignment is disallowed
						if (lvalue_exp_type == INDEXED) {
							throw CompilerException("Index assignment on strings is forbidden", 0, assignment_statement.get_line_number());
						}
						else {
							// set the symbol to "defined" and call our string_assignment function
							fetched->defined = true;
							fetched->freed = false;
							assignment_ss << this->string_assignment(*fetched, assignment_statement.get_rvalue(), assignment_statement.get_line_number(), stack_offset, max_offset).str();
						}
					}
					// TODO: add support for other dynamic types
				}
				// automatic memory is easier to handle than dynamic
				else {
					/*
					
					first, evaluate the right-hand statement
						(fetch_value will write assembly such the value currently in A (the value we fetched) is the evaluated rvalue)
					next, check to see whether the variable is /local/ or /global/; local variables will not use symbol names, but globals will

					*/

					if (fetched->scope_level == 0) {
						// global variables
						
						// get the rvalue
						if (lvalue_exp_type == INDEXED) {
							// get the value of the index and push it onto the stack
							assignment_ss << this->fetch_value(assignment_index, assignment_statement.get_line_number(), stack_offset, max_offset).str();
							assignment_ss << "\t" << "pha" << std::endl;
							
							// get the rvalue
							assignment_ss << this->fetch_value(assignment_statement.get_rvalue(), assignment_statement.get_line_number(), stack_offset, max_offset).str() << std::endl;
							assignment_ss << "\t" << "tax" << std::endl;
							assignment_ss << "\t" << "pla" << std::endl;

							// we have to use an LSL because we need to multiply by 2 (the size of a word), but we want to shift in 0
							assignment_ss << "\t" << "lsl a" << std::endl;
							
							// finish getting the index
							assignment_ss << "\t" << "tay" << std::endl;
							assignment_ss << "\t" << "txa" << std::endl;
						}
						else {
							assignment_ss << this->fetch_value(assignment_statement.get_rvalue(), assignment_statement.get_line_number(), stack_offset, max_offset).str() << std::endl;	// TODO: add max_offset in the fetch in assignment?
							assignment_ss << "\t" << "loady #$00" << std::endl;
						}

						// now, make the assignment
						if (lvalue_exp_type != DEREFERENCED) {
							assignment_ss << "\t" << "storea " << var_name << ", y" << std::endl;	// y will be zero if we have no index
						}
						else {
							// y register will be zero if lvalue_exp_type is 'DEREFERENCED'
							Dereferenced* dereferenced_value = dynamic_cast<Dereferenced*>(assignment_statement.get_lvalue().get());
							assignment_ss << this->fetch_value(dereferenced_value->get_ptr_shared(), assignment_statement.get_line_number(), stack_offset).str();
							assignment_ss << "\t" << "storea (" << var_name << ", y)" << std::endl;
						}
					}
					else {
						// get the value
						if (lvalue_exp_type == INDEXED) {
							// first, fetch the index value and push it to the stack
							assignment_ss << this->fetch_value(assignment_index, assignment_statement.get_line_number(), stack_offset, max_offset).str();
							assignment_ss << "\t" << "tay" << std::endl;	// transfer A to Y so we don't have to set the preserve_registers flag
							assignment_ss << this->move_sp_to_target_address(stack_offset, max_offset).str();	// move the stack pointer to the stack frame
							assignment_ss << "\t" << "tya" << "\n\t" << "pha" << std::endl;	// push the index
							(*stack_offset) += 1;

							// now, get the value we want
							assignment_ss << this->fetch_value(assignment_statement.get_rvalue(), assignment_statement.get_line_number(), stack_offset).str() << std::endl;

							// move the stack pointer back to where it was, but preserve our registers
							assignment_ss << "\t" << "tax" << std::endl;
							assignment_ss << this->move_sp_to_target_address(stack_offset, max_offset + 1).str();

							/*

							At this point, we have to add the appropriate index value to the stack pointer, and subtract it again once we make the assignment so the compiler's tracker doesn't get messed up. The algorithm is as follows:

							1) Multiply the index value by 2, as there are 2 bytes in a word
							2) Subtract the value of the index to the stack pointer (use SUBCA because the stack grows _downwards_)
							3) Make the push
							4) Increment the stack pointer back to where it was before we pushed the value
							5) Add the value of the index from the stack pointer (use ADDCA because the stack grows _downwards_)

							The stack pointer will then be where it was before the indexed value was assigned

							*/

							// update the index
							// since we moved A to X before the move, the value to assign is still safe in register X, and A is free to use
							assignment_ss << "\t" << "pla" << std::endl;
							(*stack_offset) -= 1;
							assignment_ss << "\t" << "lsl a" << std::endl;	// multiply the index by 2 with lsl
							assignment_ss << "\t" << "tay" << std::endl;	// move the index into Y so it's safe to move the SP

							assignment_ss << this->move_sp_to_target_address(stack_offset, fetched->stack_offset).str();

							// move the index back into the B register
							assignment_ss << "\t" << "tya" << std::endl;
							assignment_ss << "\t" << "tab" << std::endl;

							// update the stack pointer value
							assignment_ss << "\t" << "tspa" << std::endl;
							assignment_ss << "\t" << "subca b" << std::endl;	// the B register holds the index
							assignment_ss << "\t" << "tasp" << std::endl;

							// push the value
							assignment_ss << "\t" << "txa" << std::endl;	// the value to assign was in X
							assignment_ss << "\t" << "pha" << std::endl;

							// move the stack pointer back to where it was (for the compiler's sake)
							assignment_ss << "\t" << "incsp" << std::endl;	// move it back to where it was before the push
							assignment_ss << "\t" << "tspa" << std::endl;
							assignment_ss << "\t" << "addca b" << std::endl;	// subtract the index from it
							assignment_ss << "\t" << "tasp" << std::endl;	// move the pointer value back
						}
						else {
							assignment_ss << this->fetch_value(assignment_statement.get_rvalue(), assignment_statement.get_line_number(), stack_offset).str() << std::endl;

							// move the SP to the target address, but store A and B in X and Y, respectively, before we move it
							assignment_ss << "\t" << "tax" << "\n\t" << "tba" << "\n\t" << "tay" << std::endl;
							assignment_ss << this->move_sp_to_target_address(stack_offset, fetched->stack_offset).str();
							assignment_ss << "\t" << "tya" << "\n\t" << "tab" << "\n\t" << "txa" << std::endl;	// move the register values back


							// make the assignment
							if (lvalue_exp_type != DEREFERENCED) {
								assignment_ss << "\t" << "pha" << std::endl;
								*stack_offset += 1;	// when we push a variable, we need to update our stack offset
							}
							else {
								// we can use indexed indirect addressing with the x register to determine where the data should be stored
								assignment_ss << "\t" << "decsp" << std::endl;
								assignment_ss << "\t" << "tab" << std::endl;
								assignment_ss << "\t" << "pla" << std::endl;
								assignment_ss << "\t" << "tax" << std::endl;
								assignment_ss << "\t" << "tba" << std::endl;
								assignment_ss << "\t" << "storea ($00, X)" << std::endl;
							}
						}
					}

					// update the symbol's "defined" status
					fetched->defined = true;
				}
			}
			// if the types do not match, we must throw an exception
			else {
				throw CompilerException("Cannot match '" + get_string_from_type(fetched->type) + "' and '" + get_string_from_type(rvalue_data_type) + "'", 0, assignment_statement.get_line_number());
			}
		}
	}
	else {
		throw CompilerException("Could not find '" + var_name + "' in symbol table", 0, assignment_statement.get_line_number());
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
		throw CompilerException("Cannot locate function in symbol table (perhaps you didn't include the right file?)", 0, call_statement.get_line_number());
	}

	std::vector<std::shared_ptr<Statement>> formal_parameters = func_to_call_symbol.formal_parameters;

	// if we don't have any arguments, just write a jsr
	if (call_statement.get_args_size() == 0) {
		call_ss << "\t" << "jsr " << call_statement.get_func_name() << std::endl;
	}
	else {
		// add a push statement for each argument
		for (int i = 0; i < call_statement.get_args_size(); i++) {
			// get the expression for the argument
			std::shared_ptr<Expression> argument = call_statement.get_arg(i);

			// and the expression for the formal parameter
			Allocation* formal_parameter = dynamic_cast<Allocation*>(formal_parameters[i].get());
			Type formal_type = formal_parameter->get_var_type();

			// we can be passed a literal value, an lvalue, a unary, or a binary expression
			if (argument->get_expression_type() == LITERAL) {
				Literal* literal_argument = dynamic_cast<Literal*>(argument.get());
				Type argument_type = literal_argument->get_type();
				
				// make sure the type passed to the function matches what is expected
				if (argument_type == formal_type) {
					call_ss << "\t";
					if (argument_type == INT) {
						/*
						the INT type is easy to handle; simply write the number
						*/
						call_ss << "loada #$" << std::hex << std::stoi(literal_argument->get_value()) << std::endl;
					}
					else if (argument_type == STRING) {
						// add a loada statement for the string length
						call_ss << "loada #$" << std::hex << literal_argument->get_value().length() << std::endl;
						// add a loadb statement for the string litereal
						call_ss << "\t" << "@db __ARGC_STRC_" << call_statement.get_func_name() << "__" << std::dec << this->strc_number << " (" << literal_argument->get_value() << ")" << std::endl;
						call_ss << "\t" << "loadb #" << "__ARGC_STRC_" << call_statement.get_func_name() << "__" << std::dec << this->strc_number << std::endl;

						// increment the string constant number
						this->strc_number += 1;

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
							throw CompilerException("Expected a value of 'True' or 'False' in boolean literal; instead got '" + literal_argument->get_value() + "'", 0, call_statement.get_line_number());
						}
					}
					else if (argument_type == FLOAT) {
						// TODO: establish writing of float type...
					}
				}
				// otherwise, throw an exception
				else {
					throw CompilerException("Expected argument of type '" + get_string_from_type(formal_type) + "', but got '" +  get_string_from_type(argument_type) + "' instead", 0, call_statement.get_line_number());
				}
			}
			else if (argument->get_expression_type() == LVALUE) {
				LValue* var_arg_exp = dynamic_cast<LValue*>(argument.get());

				// if the variable is in the symbol table
				if (this->symbol_table.is_in_symbol_table(var_arg_exp->getValue(), this->current_scope_name)) {
					// if the lvalue we are passing is accessible
					if (this->symbol_table.is_in_symbol_table(var_arg_exp->getValue(), this->current_scope_name)) {
						// then, get the variable
						Symbol argument_symbol_data = *this->symbol_table.lookup(var_arg_exp->getValue(), this->current_scope_name, this->current_scope);

						// dynamic and static/auto memory must be treated differently
						if (argument_symbol_data.quality == DYNAMIC) {
							// DYNAMIC MEMORY

							// make sure we haven't freed the memory before we reference it
							if (argument_symbol_data.freed) {
								throw CompilerException("Cannot reference dynamic memory that has already been freed", 0, call_statement.get_line_number());
							}
							else if (argument_symbol_data.type == STRING) {
								// if the symbol is in the global scope
								if (argument_symbol_data.scope_level == 0) {
									// get the length of the string
									call_ss << "\t" << "loady #$00" << std::endl;
									call_ss << "\t" << "loada (" << argument_symbol_data.name << "), y" << std::endl;

									// now, get the address of the string (one word ahead)
									call_ss << "\t" << "loadb " << argument_symbol_data.name << std::endl;
									call_ss << "\t" << "incb" << "\n\t" << "incb" << std::endl;
								}
								else {
									// get the value at the address of our pointer
									call_ss << this->fetch_value(argument, call_statement.get_line_number(), stack_offset, max_offset).str();
									// now, A contains the length and B contains the address of the string -- we are ready to push
								}

								// now, push the length and then the address
								call_ss << "\t" << "pha" << "\n\t" << "phb" << std::endl;
							}
							else {
								// TODO: implement other dynamic types
							}
						}
						else {
							// STATIC MEMORY

							// if the symbol is not within a scope, then we can just use its name
							if (argument_symbol_data.scope_level == 0) {
								call_ss << "\t" << "loada " << argument_symbol_data.name << std::endl;
							}
							// otherwise, we must navigate through the stack
							else {
								// use fetch_value to get the argument
								call_ss << this->fetch_value(argument, call_statement.get_line_number(), stack_offset, max_offset).str();
							}

							// now that the value is in A, push it -- but first, move to the end of the stack frame
							call_ss << "\t" << "tab" << std::endl;
							call_ss << this->move_sp_to_target_address(stack_offset, max_offset).str();
							call_ss << "\t" << "phb" << std::endl;
						}
					}
					else {
						// if the variable is not found, throw an exception
						throw CompilerException("The variable you wish to pass into the function is either undefined or inaccessible", 0, call_statement.get_line_number());
					}
				}
				else {
					throw CompilerException("Could not find '" + var_arg_exp->getValue() + "' in symbol table", 0, call_statement.get_line_number());
				}
			}
			else if (argument->get_expression_type() == UNARY) {
				// TODO: evaluate the unary expression and pass the result to the function
			}
			else if (argument->get_expression_type() == BINARY) {
				Binary* binary_argument = dynamic_cast<Binary*>(argument.get());
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

	// a stringstream for our generated asm
	std::stringstream ite_ss;

	std::string parent_scope_name = this->current_scope_name;	// the parent scope name must be restored once we exit the scope

	/*
	First, make sure everything in our branch is under a label so we can use relative labels -- it looks like:
		__<scope name>_<scope level>__ITE_<branch number>__:
	*/

	std::string ite_label_name = "__" + this->current_scope_name + "_" + std::to_string(this->current_scope) + "__ITE_" + std::to_string(this->branch_number) + "__";
	ite_ss << ite_label_name << ":" << std::endl;

	// get the "if" condition
	// the simplest to do is a literal
	if ((ite_statement.get_condition()->get_expression_type() == LITERAL) || (ite_statement.get_condition()->get_expression_type() == LVALUE)) {
		/*

		Note that this branch will work for both literals AND lvalues as they only require the fetch_value function and a compare statement -- the LValue will be evaluated in fetch_value, and so the comparison will be done on a literal value held in the A register

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

		Essentially, the assembly performs a "cmp_ #$00 / breq .else" sequence

		*/

		// load the A register with the value
		ite_ss << this->fetch_value(ite_statement.get_condition(), ite_statement.get_line_number(), stack_offset, max_offset).str();
	}
	else if (ite_statement.get_condition()->get_expression_type() == UNARY) {
		
		// Unary expressions follow similar rules as literals -- see the above list for reference

		Unary* unary_condition = dynamic_cast<Unary*>(ite_statement.get_condition().get());	// cast the condition to the unary type
		ite_ss << this->evaluate_unary_tree(*unary_condition, ite_statement.get_line_number()).str();	// put the evaluated unary expression in A
	}
	else if (ite_statement.get_condition()->get_expression_type() == BINARY) {
		// TODO: evaluate binary conditions
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
	this->current_scope_name = parent_scope_name + "__ITE_" + std::to_string(this->branch_number);

	// increment the SP to the end of the stack frame
	ite_ss << this->move_sp_to_target_address(stack_offset, max_offset).str();

	// now, compile the branch using our compile method
	ite_ss << this->compile_to_sinasm(*ite_statement.get_if_branch().get(), this->current_scope, this->current_scope_name, stack_offset, max_offset).str();

	// unwind the stack and delete local variables
	for (size_t i = *stack_offset; i > max_offset; i--) {
		*stack_offset -= 1;
		ite_ss << "\t" << "incsp" << std::endl;
	}
	// we now need to delete all variables that were local to this if/else block -- iterate through the symbol table, removing the symbols in this local scope
	std::vector<Symbol>::iterator it = this->symbol_table.symbols.begin();
	while (it != this->symbol_table.symbols.end()) {
		// only delete variables that are in the scope with the same name AND of the same level
		if ((it->scope_name == this->current_scope_name) && (it->scope_level == this->current_scope)) {
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
		ite_ss << this->move_sp_to_target_address(stack_offset, max_offset).str();

		ite_ss << this->compile_to_sinasm(*ite_statement.get_else_branch().get(), this->current_scope, this->current_scope_name, stack_offset, max_offset).str();

		// unwind the stack and delete local variables
		for (size_t i = *stack_offset; i > max_offset; i--) {
			*stack_offset -= 1;
			ite_ss << "\t" << "incsp" << std::endl;
		}
		// we now need to delete all variables that were local to this if/else block -- iterate through the symbol table, removing the symbols in this local scope
		it = this->symbol_table.symbols.begin();	// use the same iterator as before
		while (it != this->symbol_table.symbols.end()) {
			// only delete variables that are in the scope with the same name AND of the same level
			if ((it->scope_name == this->current_scope_name) && (it->scope_level == this->current_scope)) {
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
	this->current_scope_name = parent_scope_name;	// reset the scope name to the parent scope

	ite_ss << ite_label_name << ".done:" << std::endl;
	ite_ss << std::endl;
	
	// return our branch code
	return ite_ss;
}

std::stringstream Compiler::while_loop(WhileLoop while_statement, size_t * stack_offset, size_t max_offset)
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
		while_ss << this->fetch_value(while_statement.get_condition(), while_statement.get_line_number(), stack_offset, max_offset).str();
	}
	else if (while_statement.get_condition()->get_expression_type() == UNARY) {
		Unary* unary_expression = dynamic_cast<Unary*>(while_statement.get_condition().get());
		while_ss << this->evaluate_unary_tree(*unary_expression, while_statement.get_line_number(), stack_offset, max_offset).str();
	}
	else if (while_statement.get_condition()->get_expression_type() == BINARY) {
		// TODO: produce binary tree
	}
	else {
		throw CompilerException("Invalid expression type in conditional expression", 0, while_statement.get_line_number());
	}

	// now that A holds the result of the expression, use a compare statement; if the condition evaluates to false, we are done
	while_ss << "\t" << "cmpa #$00" << std::endl;
	while_ss << "\t" << "breq " << while_label_name << ".done" << std::endl;

	// increment our stack pointer to the end of the current stack frame
	while_ss << this->move_sp_to_target_address(stack_offset, max_offset).str();
	// increment the branch and scope numbers and update the scope name
	this->current_scope += 1;
	this->current_scope_name = parent_scope_name + "__WHILE_" + std::to_string(branch_number);

	// put in the label for our loop
	while_ss << while_label_name << ".loop:" << std::endl;

	// compile the branch code
	while_ss << this->compile_to_sinasm(*while_statement.get_branch().get(), this->current_scope, this->current_scope_name, stack_offset, max_offset).str();

	// unwind the stack and delete local variables
	for (size_t i = *stack_offset; i > max_offset; i--) {
		*stack_offset -= 1;
		while_ss << "\t" << "incsp" << std::endl;
	}
	// we now need to delete all variables that were local to this if/else block -- iterate through the symbol table, removing the symbols in this local scope
	std::vector<Symbol>::iterator it = this->symbol_table.symbols.begin();
	while (it != this->symbol_table.symbols.end()) {
		// only delete variables that are in the scope with the same name AND of the same level
		if ((it->scope_name == this->current_scope_name) && (it->scope_level == this->current_scope)) {
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



// the actual compilation routine -- it is separate from our entry functions so that we can call it recursively (it is called by our entry functions)
std::stringstream Compiler::compile_to_sinasm(StatementBlock AST, unsigned int local_scope_level, std::string local_scope_name, size_t* stack_offset, size_t max_offset) {
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
				throw CompilerException("Inline ASM in file does not match compiler's ASM version", 0, asm_statement->get_line_number());
			}
		}
		else if (statement_type == FREE_MEMORY) {
			// dynamic cast to FreeMemory type
			FreeMemory* free_statement = dynamic_cast<FreeMemory*>(current_statement);

			// look for a symbol in the table with the same name as is indicated by the free statement
			Symbol* to_free = this->symbol_table.lookup(free_statement->get_freed_memory().getValue(), this->current_scope_name, this->current_scope);
			// we can only free dynamic memory that hasn't already been freed
			if (!to_free->freed && (to_free->quality == DYNAMIC)) {
				/*
				
				The SINASM method to free dynamic memory is simply loading the B register with the address where the variable is in the heap, and use the syscall instruction with syscall number 0x20.
				
				*/

				// fetch global and local variables differently
				if (to_free->scope_level == 0) {
					sinasm_ss << "\t" << "loadb " << to_free->name << std::endl;
				}
				else {
					sinasm_ss << "\t" << this->move_sp_to_target_address(stack_offset, to_free->stack_offset + 1).str();
					sinasm_ss << "\t" << "plb" << std::endl;
				}

				// make the syscall
				sinasm_ss << "\t" << "syscall #$20" << to_free->name << std::endl;

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
			sinasm_ss << this->allocate(*alloc_statement, stack_offset, &max_offset).str();
		}
		else if (statement_type == ASSIGNMENT) {
			// dynamic cast to an Assignment type
			Assignment* assign_statement = dynamic_cast<Assignment*>(current_statement);

			if (stack_offset) {
				sinasm_ss << this->assign(*assign_statement, stack_offset, max_offset).str();
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
			IfThenElse ite_statement = *dynamic_cast<IfThenElse*>(current_statement);

			if (stack_offset && max_offset) {
				sinasm_ss << this->ite(ite_statement, stack_offset, max_offset).str();
			}
			else if (stack_offset) {
				sinasm_ss << this->ite(ite_statement, stack_offset).str();
			}
			else {
				sinasm_ss << this->ite(ite_statement).str();
			}
		}
		else if (statement_type == WHILE_LOOP) {
			WhileLoop* while_statement = dynamic_cast<WhileLoop*>(current_statement);

			if (stack_offset && max_offset) {
				sinasm_ss << this->while_loop(*while_statement, stack_offset, max_offset).str();
			}
			else if (stack_offset) {
				sinasm_ss << this->while_loop(*while_statement, stack_offset).str();
			}
			else {
				sinasm_ss << this->while_loop(*while_statement).str();
			}
		}
		else if (statement_type == DEFINITION) {
			Definition* def_statement = dynamic_cast<Definition*>(current_statement);

			// write the definition to our stringstream containing our function definitions
			this->functions_ss << this->define(*def_statement).str();
		}
		else if (statement_type == CALL) {
			Call* call_statement = dynamic_cast<Call*>(current_statement);

			// write the call to the function into the file
			if (stack_offset) {
				sinasm_ss << this->call(*call_statement, stack_offset, max_offset).str();
			}
			else {
				sinasm_ss << this->call(*call_statement).str();
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
		// write the body of the program
		this->sina_file << this->compile_to_sinasm(this->AST, current_scope, current_scope_name, &this->stack_offset).str();

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
	
	// declare a stringstream to hold our generated ASM code
	std::stringstream generated_asm;

	// all programs include a header to define a few things
	//if (include_builtins) {
	//	generated_asm << "@include builtins.sina" << std::endl;	// include 'builtins', to allow us to use SIN's built-in functions in the VM
	//	this->object_file_names->push_back("builtins.sinc");
	//}
	// any other common header information will go here

	// write the body of the program
	generated_asm << this->compile_to_sinasm(this->AST, current_scope, current_scope_name, &this->stack_offset).str();

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
	this->AST = parser.create_ast();

	this->current_scope = 0;	// start at the global scope
	this->current_scope_name = "global";

	this->stack_offset = 0;

	this->strc_number = 0;
	this->branch_number = 0;

	this->_DATA_PTR = 0;	// our first address for variables is $00
	this->AST_index = 0;	// we use "get_next_statement()" every time, which increments before returning; as such, start at -1 so we fetch the 0th item, not the 1st, when we first call the compilation function
	
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
	this->library_names = {};
	this->AST_index = 0;
	this->_wordsize = 16;
	this->current_scope = 0;
	this->current_scope_name = "global";
	this->strc_number = 0;
	this->branch_number = 0;
	this->_DATA_PTR = 0;
	this->object_file_names = {};
}

Compiler::~Compiler()
{
}
