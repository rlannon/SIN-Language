/*

SIN Toolchain
CompilerUtilities.cpp
Copyright 2019 Riley Lannon

Contains the implementation of various compiler utility functions, including:
	- Compiler::get_next_statement(...)	- gets the next statement in a given AST, using the compiler object's AST index
	- Compiler::get_current_statement(...)	- gets the statement currently being evaluated in an AST, using the compiler object's AST index
	- Compiler::get_expression_data_type(...)	- gets the anticipated data type of a given expression
	- Compiler::is_signed(...)	- determines whether the result of a given expression will be signed or not
	- Compiler::fetch_value(...)	- get the value of a given expression in the appropriate registers or at the appropriate position; this depends on the value's type
	- Compiler::move_sp_to_target_address(...)	- increments or decrements the stack pointer to the specified position

*/

#include "Compiler.h"


std::shared_ptr<Statement> Compiler::get_next_statement(StatementBlock AST)
{
	// Given a StatementBlock object, get statements from it
	this->AST_index += 1;	// increment AST_index by one
	std::shared_ptr<Statement> stmt_ptr = AST.statements_list[AST_index];	// get the shared_ptr<Statement> at the correct position
	return stmt_ptr; // return the shared_ptr<Statement>
}

std::shared_ptr<Statement> Compiler::get_current_statement(StatementBlock AST)
{
	// Gets the current statement from the AST
	return AST.statements_list[AST_index];	// return the shared_ptr<Statement> at the current position of the AST index
}


// todo: add line_number as parameter because this function may throw an exception
DataType Compiler::get_expression_data_type(std::shared_ptr<Expression> to_evaluate, unsigned int line_number)
{
	/*

	This function takes an expression and returns its expected data type were it to be evaluated. For example, passing it an int literal would return INT, while a variable (string) 'myStr' will be looked up in the symbol table and evaluated before returning STRING.

	Note that this function does not evaluate binary trees or unary expressions; rather, it looks at the first literal or lvalue it can find and returns that value. It evaluates the type that the tree is /expected/ to return if it was constructed correctly; type match errors will be discovered once the compiler actually attempts to produce the tree in assembly.

	The function works by checking the expression type of to_evaluate and returning the value, and operates recursively if it sees another shared_ptr as an operand.

	*/

	// start with the two that we can do without recursion
	if (to_evaluate->get_expression_type() == LITERAL) {
		Literal* literal_exp = dynamic_cast<Literal*>(to_evaluate.get());

		return DataType(literal_exp->get_data_type());
	}
	else if (to_evaluate->get_expression_type() == LVALUE || to_evaluate->get_expression_type() == INDEXED) {
		LValue* lvalue_exp = dynamic_cast<LValue*>(to_evaluate.get());

		// make sure it's in the symbol table
		if (this->symbol_table.is_in_symbol_table(lvalue_exp->getValue(), this->current_scope_name)) {
			// get the variable's symbol and return the type
			std::shared_ptr<Symbol> fetched = this->symbol_table.lookup(lvalue_exp->getValue(), this->current_scope_name, this->current_scope);

			// ensure we got a Symbol, and not something else
			if (fetched->symbol_type == VARIABLE) {
				Symbol* lvalue_symbol = dynamic_cast<Symbol*>(fetched.get());

				// return a DataType object with the type and subtype of lvalue_symbol
				return lvalue_symbol->type_information;
			}
			else {
				throw CompilerException("Expected modifiable-lvalue");
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
		DataType address_of_type = this->get_expression_data_type(std::make_shared<LValue>(address_of_exp->get_target()), line_number);
		return DataType(PTR, address_of_type.get_primary());
	}
	else if (to_evaluate->get_expression_type() == UNARY) {
		Unary* unary_exp = dynamic_cast<Unary*>(to_evaluate.get());

		return this->get_expression_data_type(unary_exp->get_operand(), line_number);
	}
	else if (to_evaluate->get_expression_type() == BINARY) {
		Binary* binary_exp = dynamic_cast<Binary*>(to_evaluate.get());

		return this->get_expression_data_type(binary_exp->get_left(), line_number);
	}
	else if (to_evaluate->get_expression_type() == DEREFERENCED) {
		Dereferenced* dereferenced_exp = dynamic_cast<Dereferenced*>(to_evaluate.get());

		return this->get_expression_data_type(dereferenced_exp->get_ptr_shared(), line_number);
	}
	else if (to_evaluate->get_expression_type() == VALUE_RETURNING_CALL) {
		ValueReturningFunctionCall* val_ret_exp = dynamic_cast<ValueReturningFunctionCall*>(to_evaluate.get());

		// get the symbol and return its type
		std::shared_ptr<Symbol> fetched = this->symbol_table.lookup(val_ret_exp->get_func_name());
		FunctionSymbol* func_symbol = nullptr;

		if (fetched->symbol_type == FUNCTION_DEFINITION) {
			func_symbol = dynamic_cast<FunctionSymbol*>(fetched.get());
		}
		else {
			throw CompilerException("Expected function definition symbol");
		}

		return func_symbol->type_information;
	}
	else if (to_evaluate->get_expression_type() == LIST) {
		ListExpression* list_exp = dynamic_cast<ListExpression*>(to_evaluate.get());

		std::vector<std::shared_ptr<Expression>> list_members = list_exp->get_list();

		if (list_members.size() > 0) {
			// get the type we expect for the list; the first member determines it
			DataType list_data_type = this->get_expression_data_type(list_members[0], line_number);

			// iterate through the list and check to see if the types are homogenous
			size_t list_item = 1;
			bool abort = false;
			while (list_item < list_members.size() && !abort) {
				DataType current_item_type = this->get_expression_data_type(list_members[list_item], line_number);
				abort = current_item_type != list_data_type;
				list_item++;
			}

			if (abort) {
				throw CompilerException("Lists must be homogenous in SIN");
			}
			else {
				return list_data_type;
			}
		}
		else {
			compiler_warning("Empty list found");
			return VOID;	// an empty list has a type of VOID
		}
	}
	else if (to_evaluate->get_expression_type() == SIZE_OF) {
		return DataType(INT, NONE);		// sizeof(...) always returns an unsigned int
	}
	else {
		return DataType(NONE, NONE);
	}
}

bool Compiler::is_signed(std::shared_ptr<Expression> to_evaluate, unsigned int line_number)
{
	/*

	Determines whether the result of a given expression should be signed or not

	*/

	if (to_evaluate->get_expression_type() == LITERAL) {
		Literal* literal_exp = dynamic_cast<Literal*>(to_evaluate.get());

		// only INT and FLOAT can be signed
		if (literal_exp->get_data_type() == INT) {
			// an int may be signed or unsigned; we must get the value to determine whether to return true or false
			int value = std::stoi(literal_exp->get_value());

			// if it's negative, it's signed; otherwise, it's unsigned
			if (value < 0) {
				return true;
			}
			else {
				return false;
			}
		}
		else if (literal_exp->get_data_type() == FLOAT) {
			// floats are _always_ signed
			return true;
		}
		else {
			return false;
		}
	}
	else if (to_evaluate->get_expression_type() == LVALUE || to_evaluate->get_expression_type() == INDEXED) {
		LValue* lvalue_exp = dynamic_cast<LValue*>(to_evaluate.get());
		std::shared_ptr<Symbol> fetched = this->symbol_table.lookup(lvalue_exp->getValue(), this->current_scope_name, this->current_scope);
		Symbol* to_check = dynamic_cast<Symbol*>(fetched.get());	// todo: check symbol_type? or is this unnecessary?

		// only ints and floats can be signed
		if (to_check->type_information.get_primary() == INT) {
			// check the int's quality
			return to_check->type_information.get_qualities().is_signed();
		}
		else if (to_check->type_information.get_primary() == FLOAT) {
			return true;	// floats are always signed
		}
		else {
			return false;
		}
	}
	else if (to_evaluate->get_expression_type() == ADDRESS_OF) {
		return false;	// addresses are unsigned
	}
	else if (to_evaluate->get_expression_type() == DEREFERENCED) {
		Dereferenced* deref_exp = dynamic_cast<Dereferenced*>(to_evaluate.get());
		return this->is_signed(deref_exp->get_ptr_shared(), line_number);	// call this function on the thing the pointer is pointing to
	}
	else if (to_evaluate->get_expression_type() == UNARY) {
		Unary* unary_exp = dynamic_cast<Unary*>(to_evaluate.get());

		// check to see if the unary operand is signed
		bool unary_arg_is_signed = this->is_signed(unary_exp->get_operand());

		// if the unary operand is signed, or if the unary operator is MINUS, we have a signed expression
		if (unary_arg_is_signed || unary_exp->get_operator() == MINUS) {
			return true;
		}
		else {
			return false;
		}
	}
	else if (to_evaluate->get_expression_type() == BINARY) {
		Binary* bin_exp = dynamic_cast<Binary*>(to_evaluate.get());

		// if either one of the operands is signed, return true
		bool left_is_signed = this->is_signed(bin_exp->get_left());
		bool right_is_signed = this->is_signed(bin_exp->get_right());

		if (left_is_signed || right_is_signed) {
			// however, if only one is signed, generate an error
			if (!(left_is_signed && right_is_signed)) {
				compiler_warning("Signed/unsigned mismatch", line_number);
			}

			return true;
		}
		else {
			return false;
		}
	}
	else {
		return false;
	}
}

bool Compiler::types_are_compatible(std::shared_ptr<Expression> left, std::shared_ptr<Expression> right, unsigned int line_number) {
	/*
	
	Checks whether two types are compatible with one another.
	Simply get the expression data type from each expression, create DataTypes from them, and run DataType::is_compatible(...)
	
	*/

	DataType left_type = this->get_expression_data_type(left, line_number);
	DataType right_type = this->get_expression_data_type(right, line_number);

	return left_type.is_compatible(right_type);
}


std::stringstream Compiler::fetch_value(std::shared_ptr<Expression> to_fetch, unsigned int line_number, size_t max_offset)
{
	/*

	Given an expression, this function will produce ASM that, when run, will get the desired value in the A register. For example, if we have the statement
		let myVar = anotherVar;
	we can use this function to fetch the contents of 'anotherVar', producing assembly that will load its value in the A register; we can then finish compiling the statement that uses the fetched value.

	*/

	// the stringstream that will contain our assembly code
	std::stringstream fetch_ss;

	// test the expression type to determine how to write the assembly for fetching it

	// the simplest one is a literal
	if (to_fetch->get_expression_type() == LITERAL) {
		Literal* literal_expression = dynamic_cast<Literal*>(to_fetch.get());

		// different data types will require slightly different methods for loading
		if (literal_expression->get_data_type() == INT) {
			// int types just need to write a loada instruction
			fetch_ss << "\t" << "loada #$" << std::hex << std::stoi(literal_expression->get_value()) << std::endl;
		}
		else if (literal_expression->get_data_type() == BOOL) {
			// bool types are also easy to write; any nonzero integer is true
			int bool_expression_as_int;
			if (literal_expression->get_value() == "true") {
				bool_expression_as_int = 1;
			}
			else if (literal_expression->get_value() == "false") {
				bool_expression_as_int = 0;
			}
			else {
				// if it isn't "true" or "false", throw an error
				throw CompilerException("Expected 'true' or 'false' as boolean literal value (case matters!)", 0, line_number);
			}

			fetch_ss << "\t" << "loada #$" << std::hex << bool_expression_as_int << std::endl;
		}
		else if (literal_expression->get_data_type() == FLOAT) {
			// for now, we will only deal with a half-precision float type, though a single-precision could be implemented as well
			// copy the bits of the float value into a uint32_t
			float literal_value = std::stof(literal_expression->get_value());
			uint32_t converted_value = *reinterpret_cast<uint32_t*>(&literal_value);

			// now, pack that 32-bit float into a 16-bit float and load reg_a with it
			uint16_t literal_float = pack_32(converted_value);
			fetch_ss << "\t" << "loada #$" << std::hex << static_cast<int>(literal_float) << std::endl;
		}
		else if (literal_expression->get_data_type() == STRING) {
			// first, define a constant for the string using our naming convention
			std::string string_constant_name;
			string_constant_name = "__STRC__NUM_" + std::to_string(this->strc_number);	// define the constant name
			this->strc_number++;	// increment the strc number by one

			// define the constant
			fetch_ss << "@db " << string_constant_name << " (" << literal_expression->get_value() << ")" << std::endl;

			// load the A and B registers appropriately
			fetch_ss << "\t" << "loada " << string_constant_name << std::endl;
			fetch_ss << "\t" << "loadb #" << string_constant_name << std::endl;
			fetch_ss << "\t" << "incb" << "\n\t" << "incb" << std::endl;
		}
	}
	else if (to_fetch->get_expression_type() == LVALUE || to_fetch->get_expression_type() == INDEXED) {
		// get the lvalue's symbol data
		Symbol* variable_symbol;

		if (to_fetch->get_expression_type() == LVALUE) {
			LValue* variable_to_get = dynamic_cast<LValue*>(to_fetch.get());
			std::shared_ptr<Symbol> fetched = this->symbol_table.lookup(variable_to_get->getValue(), this->current_scope_name, this->current_scope);
			
			// ensure we got a variable symbol and not the symbol for a function definition, or something else
			if (fetched->symbol_type == VARIABLE) {
				variable_symbol = dynamic_cast<Symbol*>(fetched.get());
			}
			else {
				throw CompilerException("Expected modifiable-lvalue", 0, line_number);
			}
		}
		else {
			Indexed* variable_to_get = dynamic_cast<Indexed*>(to_fetch.get());
			std::shared_ptr<Symbol> fetched = this->symbol_table.lookup(variable_to_get->getValue(), this->current_scope_name, this->current_scope);

			// ensure we got a variable symbol and not the symbol for a function definition, or something else
			if (fetched->symbol_type == VARIABLE) {
				variable_symbol = dynamic_cast<Symbol*>(fetched.get());
			}
			else {
				throw CompilerException("Expected modifiable-lvalue", 0, line_number);
			}

			// now, use fetch_value to get the index value in the A register
			fetch_ss << this->fetch_value(variable_to_get->get_index_value(), line_number, max_offset).str();
			// move the value into the Y register to preserve it if we need the A register
			fetch_ss << "\t" << "tay" << std::endl;
		}

		// now that the variable has been fetched, check its qualities

		bool is_const = variable_symbol->type_information.get_qualities().is_const();
		bool is_dynamic = variable_symbol->type_information.get_qualities().is_dynamic();
		bool is_signed = variable_symbol->type_information.get_qualities().is_signed();

		// only fetch the value if it has been defined
		if (variable_symbol->defined) {

			// check the scope; we need to do different things for global and local scopes
			if ((variable_symbol->scope_name == "global") && (variable_symbol->scope_level == 0)) {
				// const strings operate a little differently than regular strings; they do not use indirect addressing
				if (is_const && variable_symbol->type_information.get_primary() == STRING) {
					/*
					
					Const strings can use direct addressing -- 
						- load the value at the address of the symbol into A
						- load the address of the symbol into B
						- increment B by two, to skip the address of the length
					
					*/

					fetch_ss << "\t" << "loada " << variable_symbol->name << std::endl;
					fetch_ss << "\t" << "loadb #" << variable_symbol->name << std::endl;
					fetch_ss << "\t" << "incb" << "\n\t" << "incb" << std::endl;

					// now we are done; A and B contain the proper information
				}
				// all we need to do is use the variable name for globals; however, we need to know the type
				else if (is_dynamic) {

					// ensure that the dynamic memory has not been freed
					if (variable_symbol->freed) {
						throw CompilerException("Cannot reference dynamic memory that has already been freed", 0, line_number);
					}
					else {
						// if we are indexing a string
						if (to_fetch->get_expression_type() == INDEXED) {
							// transfer the index offset from A to B
							fetch_ss << "\t" << "tab" << std::endl;

							// load a with the address
							fetch_ss << "\t" << "loada " << variable_symbol->name << std::endl;

							// add the index offset, transfer to b, and increment it by 2 (to skip the length)
							fetch_ss << "\t" << "clc" << std::endl;
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
						// check to see if we have an array of strings or of something else
						if (variable_symbol->type_information.get_subtype() == STRING) {
							// multiply the index offset by 2 with lsl and transfer to Y
							fetch_ss << "\t" << "lsl a" << std::endl;
							fetch_ss << "\t" << "tay" << std::endl;

							// use indexed indirect addressing to get the length
							fetch_ss << "\t" << "loada (" << variable_symbol->name << ", y)" << std::endl;

							// get the value at "name, y" -- this is the address of the dynamic object; increment it by 2 (to skip the length)
							fetch_ss << "\t" << "loadb " << variable_symbol->name << ", y" << std::endl;
							fetch_ss << "\t" << "incb" << "\n\t" << "incb" << std::endl;
						}
						else {
							// multiply the index offset by 2 with lsl and transfer it from A to Y
							fetch_ss << "\t" << "lsl a" << std::endl;
							fetch_ss << "\t" << "tay" << std::endl;
							fetch_ss << "\t" << "loada " << variable_symbol->name << ", y" << std::endl;
						}
					}
					else {
						fetch_ss << "\t" << "loada " << variable_symbol->name << std::endl;
					}
				}
			}
			// if it's a local variable, use the stack
			else {
				fetch_ss << this->move_sp_to_target_address(variable_symbol->stack_offset + 1).str();

				if (to_fetch->get_expression_type() == INDEXED) {
					fetch_ss << "\t" << "tya" << std::endl;	// move the index value back into A
				}

				// now, the offsets are the same; get the variable's value
				if (is_dynamic) {
					if (variable_symbol->freed) {
						throw CompilerException("Cannot reference dynamic memory that has already been freed", 0, line_number);
					}
					else {
						// if we are indexing the string, get the value within the string in A
						if (to_fetch->get_expression_type() == INDEXED) {
							// Note that when we index a local string, we do _not_ use the lsl instruction; since characters are a single byte, the index number and the character number will be 
							// the index value is in the A register; we need to add two to it to skip the address of the string
							fetch_ss << "\t" << "clc" << std::endl;
							fetch_ss << "\t" << "addca #$02" << std::endl;

							// now, we need to pull the address of the string into B
							fetch_ss << "\t" << "plb" << std::endl;
							this->stack_offset -= 1;

							// now, add that address to our index offset
							fetch_ss << "\t" << "clc" << std::endl;
							fetch_ss << "\t" << "addca B" << std::endl;

							// now, transfer A into B and load A with 1 -- indexing a string gives a string consisting of one character
							fetch_ss << "\t" << "tab" << std::endl;
							fetch_ss << "\t" << "loada #$01" << std::endl;

							// A now contains the length (1) and B contains the address (string address + length offset + index offset)
						}
						else {
							// dynamic variables must use pointer dereferencing
							// so we pull the address of the string into B
							fetch_ss << "\t" << "plb" << std::endl;
							this->stack_offset -= 1;

							// now, we must get the value at the address contained in B -- use the X register for this -- which is our string length
							fetch_ss << "\t" << "tbx" << std::endl;	// simply index -- we _already dereferenced the pointer_, this address does not contain another address to dereference, but rather the data we want
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
						fetch_ss << "\t" << "sec" << std::endl;
						fetch_ss << "\t" << "subca b" << std::endl;
						fetch_ss << "\t" << "tasp" << std::endl;

						// get the value
						fetch_ss << "\t" << "pla" << std::endl;
						this->stack_offset -= 1;
						fetch_ss << "\t" << "tax" << std::endl;

						// re-adjust the SP
						fetch_ss << "\t" << "tspa" << std::endl;
						fetch_ss << "\t" << "clc" << std::endl;
						fetch_ss << "\t" << "addca b" << std::endl;
						fetch_ss << "\t" << "tasp" << std::endl;
						fetch_ss << "\t" << "txa" << std::endl;

						// if we have a string, we need to get the length and address into A and B
						if (variable_symbol->type_information.get_subtype() == STRING) {
							// right now, A contains the address of the dynamic object; transfer to Y and use indirect addressing to load the value
							fetch_ss << "\t" << "tay" << std::endl;
							fetch_ss << "\t" << "tab" << std::endl;	// now the B register contains the address as well
							fetch_ss << "\t" << "loada $00, y" << std::endl;	// use indexed addressing to get the value at the address indicated by the Y register; note we don't need the indirect mode for this, as we don't want the value at the address indicated by "$00, y", we want the value at "$00, y"
							fetch_ss << "\t" << "incb" << "\n\t" << "incb" << std::endl;	// increment the address by 2, skipping the length
						}
					}
					else {
						// pull the value into the A register
						fetch_ss << "\t" << "pla" << std::endl;
						this->stack_offset -= 1;	// we pulled from the stack, so we must adjust the stack offset
					}
				}
			}
		}
		else {
			throw CompilerException("Variable '" + variable_symbol->name + "' referenced before assignment", 0, line_number);
		}
	}
	else if (to_fetch->get_expression_type() == DEREFERENCED) {
		/*
		
		To fetch a dereferenced variable, we use fetch_value on the shared_ptr contained within it recursively
		Then, after each fetch, we use the following code:
			tay
			loada #$00, y
		This should allow for doubly-dereferenced (and more) pointers
		
		*/
		Dereferenced* dereferenced_exp = dynamic_cast<Dereferenced*>(to_fetch.get());;

		// check to make sure what we are dereferencing is /actually/ a pointer
		LValue pointed_to = dereferenced_exp->get_ptr();
		Symbol* pointed_symbol = this->symbol_table.lookup(pointed_to.getValue()).get();
		if (pointed_symbol->type_information.get_primary() == PTR) {

			fetch_ss << this->fetch_value(dereferenced_exp->get_ptr_shared(), line_number, max_offset).str();
			fetch_ss << "\t" << "tay" << std::endl;

			// since the stack grows downwards, local variables will need to decrement y by 1 so they have the correct start address for the variable
			// todo: changing the stack to grow upwards will eliminate the need for this
			if (this->current_scope > 0) {
				fetch_ss << "\t" << "decy" << std::endl;
			}

			fetch_ss << "\t" << "loada $00, y" << std::endl;	// todo: peephole optimization here
		}
		// otherwise, throw an error
		else {
			throw CompilerException("You may not dereference a variable whose type is not ptr<...>", 0, line_number);
		}
	}
	else if (to_fetch->get_expression_type() == ADDRESS_OF) {
		// dynamic cast to AddressOf and get the variable's symbol from the symbol table
		AddressOf* address_of_exp = dynamic_cast<AddressOf*>(to_fetch.get());	// the AddressOf expression
		LValue address_to_get = address_of_exp->get_target();	// the actual LValue of the address we want
		std::shared_ptr<Symbol> fetched = this->symbol_table.lookup(address_to_get.getValue(), this->current_scope_name, this->current_scope);
		Symbol* variable_symbol = dynamic_cast<Symbol*>(fetched.get());

		// get our qualities

		bool is_const = variable_symbol->type_information.get_qualities().is_const();
		bool is_dynamic = variable_symbol->type_information.get_qualities().is_dynamic();
		bool is_signed = variable_symbol->type_information.get_qualities().is_signed();

		// todo: remove these comments? or uncomment?
		// make sure the variable was defined
		if (is_dynamic) {
			// Getting the address of a dynamic variable is easy -- we simply move the stack pointer to the proper byte, pull the value into A

			if (variable_symbol->freed) {
				throw CompilerException("Cannot reference dynamic memory that has already been freed", 0, line_number);
			}
			else {
				if (variable_symbol->scope_level == 0) {
					fetch_ss << "\t" << "loada " << variable_symbol->name << std::endl;
				}
				else {
					fetch_ss << this->move_sp_to_target_address(variable_symbol->stack_offset + 1).str();

					// now that the stack pointer is in the right place, we can pull the value into A
					fetch_ss << "\t" << "pla" << std::endl;
					this->stack_offset -= 1;
				}
			}
		}
		else {
			if (variable_symbol->scope_level == 0) {
				fetch_ss << "\t" << "loada #" << variable_symbol->name << std::endl;	// using  "loada var" would mean "load the A register with the value at address 'var' " while "loada #var" means "load the A register with the address of 'var' "
			}
			else {
				fetch_ss << this->move_sp_to_target_address(variable_symbol->stack_offset + 1).str();

				// now that the stack pointer is in the proper place to pull the variable from, increment it by one place and transfer the pointer value to A; that is the address where the variable we want lives
				this->stack_offset -= 1;
				fetch_ss << "\t" << "incsp" << std::endl;
				fetch_ss << "\t" << "tspa" << std::endl;
			}
		}
	}
	else if (to_fetch->get_expression_type() == UNARY) {
		Unary* unary_expression = dynamic_cast<Unary*>(to_fetch.get());
		fetch_ss << this->evaluate_unary_tree(*unary_expression, line_number, max_offset).str();
	}
	else if (to_fetch->get_expression_type() == BINARY) {
		Binary* binary_expression = dynamic_cast<Binary*>(to_fetch.get());
		fetch_ss << this->evaluate_binary_tree(*binary_expression, line_number, max_offset).str();
	}
	else if (to_fetch->get_expression_type() == VALUE_RETURNING_CALL) {
		ValueReturningFunctionCall* val_ret = dynamic_cast<ValueReturningFunctionCall*>(to_fetch.get());

		// now, search through the symbol table for the function so we can get its return type
		FunctionSymbol* function_symbol = dynamic_cast<FunctionSymbol*>(this->symbol_table.lookup(val_ret->get_name()->getValue()).get());	// todo: use 'fetched' variable and validate it?

		// increment the stack pointer to the stack frame
		fetch_ss << this->move_sp_to_target_address(max_offset).str();

		// call the function
		Call to_call(val_ret->get_name(), val_ret->get_args());	// create a 'call' object from val_ret
		to_call.set_line_number(line_number);
		fetch_ss << this->call(to_call, max_offset).str();	// add that to the asm

		// now, the returned value will be in the registers; if the type is of variable length (array or struct), handle it separately because it is on the stack
		if (function_symbol->type_information.get_primary() == ARRAY) {
			// TODO: implement arrays
		}
		else if (function_symbol->type_information.get_primary() == STRUCT) {
			// TODO: implement structs
		}
		else if (function_symbol->type_information.get_primary() == VOID || function_symbol->type_information.get_primary() == NONE) {
			// we cannot use void functions as value returning types
			throw CompilerException("Cannot retrieve value of '" + get_string_from_type(function_symbol->type_information.get_primary()) + "' type", 0, line_number);
		}

		// We are done -- the values are where they are expected to be
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

std::stringstream Compiler::move_sp_to_target_address(size_t target_offset, bool preserve_registers)
{
	/*

	Increment the stack pointer to the end of the current stack frame so that we can enter a new scope.

	If the register values do not need to be preserved, the function will

	*/
	std::stringstream inc_ss;

	// increment the stack pointer to the end of the current stack frame
	if (this->stack_offset < target_offset) {
		// if we need to increment more than three times, use a transfer and add -- otherwise, it's efficient enough to use decsp; we will also elect not to use this method if we must preserve our register values
		if ((target_offset - this->stack_offset > 3) && !preserve_registers) {
			// transfer the stack pointer to a, add the difference, and transfer it back
			size_t difference = target_offset - this->stack_offset;
			inc_ss << "\t" << "tspa" << std::endl;
			inc_ss << "\t" << "sec" << std::endl;
			inc_ss << "\t" << "subca #$" << std::hex << WORD_W * difference << std::endl;	// advance by the difference between them in _words_, so multiply the difference by WORD_W and add it to the SP
			inc_ss << "\t" << "tasp" << std::endl;

			this->stack_offset = target_offset;
		}
		else {
			while (this->stack_offset < target_offset) {
				inc_ss << "\t" << "decsp" << std::endl;
				this->stack_offset += 1;
			}
		}
	}
	else if (this->stack_offset > target_offset) {
		// do the reverse here of what we did above
		if ((this->stack_offset - target_offset > 3) && !preserve_registers) {
			size_t difference = this->stack_offset - target_offset;
			inc_ss << "\t" << "tspa" << std::endl;
			inc_ss << "\t" << "clc" << std::endl;
			inc_ss << "\t" << "addca #$" << std::hex << WORD_W * difference << std::endl;
			inc_ss << "\t" << "tasp" << std::endl;

			this->stack_offset = target_offset;
		}
		else {
			while (this->stack_offset > target_offset) {
				inc_ss << "\t" << "incsp" << std::endl;
				this->stack_offset -= 1;
			}
		}
	}

	return inc_ss;
}
