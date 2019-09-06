/*

SIN Toolchain
Assign.cpp
Copyright 2019 Riley Lannon

This file implements the functions to compile assignment statements.

*/

#include "Compiler.h"


std::stringstream Compiler::string_assignment(Symbol* target_symbol, std::shared_ptr<Expression> rvalue, unsigned int line_number, size_t max_offset)
{
	/*
	
	Handles a string assignment.

	*/
	
	std::stringstream string_assign_ss;

	// if we have an indexed statement, temporarily store the Y value (the index value) in LOCAL_DYNAMIC_POINTER, as we don't need that address quite yet
	if (target_symbol->type_information.get_primary() == ARRAY) {
		string_assign_ss << "\t" << "storey $" << std::hex << _LOCAL_DYNAMIC_POINTER << std::endl;
	}

	// fetch the rvalue -- A will contain the length, B will contain the address
	string_assign_ss << this->fetch_value(rvalue, line_number, max_offset).str();

	// move the stack pointer to the end of the stack frame
	if (this->stack_offset != max_offset) {
		// transfer A and B to X and Y before incrementing the stack pointer
		string_assign_ss << "\t" << "tax" << "\n\t" << "tba" << "\n\t" << "tay" << std::endl;

		// increment the stack pointer to the end of the stack frame so we can use the stack
		string_assign_ss << this->move_sp_to_target_address(max_offset).str();

		// move X and Y back into A and B
		string_assign_ss << "\t" << "tya" << "\n\t" << "tab" << "\n\t" << "txa" << std::endl;
	}

	string_assign_ss << "\t" << "phb" << std::endl;	// push the address of the string pointer
	this->stack_offset += 1;
	max_offset += 1;

	// add some padding to the string length
	string_assign_ss << "\t" << "pha" << std::endl;
	string_assign_ss << "\t" << "clc" << std::endl;
	string_assign_ss << "\t" << "addca #$10" << std::endl;
	this->stack_offset += 1;
	max_offset += 1;

	/*

	Next, we need to allocate memory for the object. This must be handled differently based on the variable's type.

	If we have a string:
		If the string has already been allocated, reallocate it; if not, then allocate it for the first time.
		If we need to reallocate:
			Since A contains the length already, we just need the address.
	Otherwise, if we have an array:
		If no object has been allocated, allocate one
		Get the address and attempt to use the "safe reallocation" method; here, the VM will attempt to find the specified object, or create a new one if it can't

	*/

	if (!target_symbol->allocated) {
		string_assign_ss << "\t" << "syscall #$21" << std::endl;
		target_symbol->allocated = true;
	}
	else {
		// preserve A by transfering to X
		string_assign_ss << "\t" << "tax" << std::endl;

		// if we have a static variable
		if (target_symbol->scope_name == "global" && target_symbol->scope_level == 0) {
			if (target_symbol->type_information.get_primary() == ARRAY) {
				string_assign_ss << "\t" << "loady $" << _LOCAL_DYNAMIC_POINTER << std::endl;
				string_assign_ss << "\t" << "loadb " << target_symbol->name << ", y" << std::endl;
			}
			else {
				string_assign_ss << "\t" << "loadb " << target_symbol->name << std::endl;
			}
		}
		// otherwise, it's on the stack
		else {
			// todo: add dynamic memory (re)allocation for local string arrays
			// fetch the variable into B
			size_t former_offset = this->stack_offset;
			string_assign_ss << this->move_sp_to_target_address(target_symbol->stack_offset + 1).str();

			// if we have a string array, we need to advance to the index position in the stack, pull, and then move back as far as we moved forward
			if (target_symbol->type_information.get_primary() == ARRAY) {
				// subtract the offset from the stack pointer
				string_assign_ss << "\t" << "tspa" << std::endl;
				string_assign_ss << "\t" << "sec" << std::endl;
				string_assign_ss << "\t" << "subca $" << _LOCAL_DYNAMIC_POINTER << std::endl;
				string_assign_ss << "\t" << "tasp" << std::endl;

				// pull the value into B and adjust the compiler's offset counter
				string_assign_ss << "\t" << "plb" << std::endl;
				this->stack_offset -= 1;
				max_offset -= 1;

				// move the stack pointer back as many bytes as we advanced it
				string_assign_ss << "\t" << "tspa" << std::endl;
				string_assign_ss << "\t" << "clc" << std::endl;
				string_assign_ss << "\t" << "addca $" << _LOCAL_DYNAMIC_POINTER << std::endl;
				string_assign_ss << "\t" << "tasp" << std::endl;
			}
			// otherwise, just pull
			else {
				string_assign_ss << "\t" << "plb" << std::endl;
				this->stack_offset -= 1;
				max_offset -= 1;
			}

			string_assign_ss << this->move_sp_to_target_address(former_offset).str();	// move the stack offset back
		}

		if (target_symbol->type_information.get_primary() == ARRAY) {
			string_assign_ss << "\t" << "txa" << std::endl;
			string_assign_ss << "\t" << "syscall #$23" << std::endl;
		}
		else {
			// move the length back into register A and make the syscall
			string_assign_ss << "\t" << "txa" << std::endl;
			string_assign_ss << "\t" << "syscall #$22" << std::endl;
		}
	}

	// Global and local variables must be handled differently
	if (target_symbol->scope_level == 0) {
		/*

		Global strings will use _LOCAL_DYNAMIC_POINTER to store the address rather than referencing the symbol's name This is so that we can reference array members without a problem.

		An example of the issue is if we use the symbol's name, we might get:
			storea (symbol_name), y
		when we really want
			storea (symbol_name + offet), y

		Since SINASM does not allow for this sort of nested offset, we will use _LOCAL_DYNAMIC_POINTER so we can just say
			storea (_LOCAL_DYNAMIC_POINTER), y
		where _LOCAL_DYNAMIC_POINTER holds the same value as the symbol (or symbol + offset) does

		Since the global string assignment routine was not using this memory address previously, we are free to use it for this purpose

		*/

		// if we have an indexed variable assignment, we need to fetch the value
		if (target_symbol->type_information.get_primary() == ARRAY) {
			// load the Y register with the value at _LOCAL_DYNAMIC_POINTER, which contains the index value; it has already been multiplied (before we stored it at the address of _LOCAL_DYNAMIC_POINTER, so we don't need to worry about that
			string_assign_ss << "\t" << "loady $" << std::hex << _LOCAL_DYNAMIC_POINTER << std::endl;
			string_assign_ss << "\t" << "storeb " << target_symbol->name << ", y" << std::endl;	// store the value in REG_B at the symbol name indexed by the number of bytes by which our array member is offset
			string_assign_ss << "\t" << "storeb $" << _LOCAL_DYNAMIC_POINTER << std::endl;	// store the value at _LOCAL_DYNAMIC_POINTER as well

		}
		else {
			string_assign_ss << "\t" << "storeb " << target_symbol->name << std::endl;	// store the address in our pointer variable
			string_assign_ss << "\t" << "storeb $" << std::hex << _LOCAL_DYNAMIC_POINTER << std::endl;	// store the value at _LOCAL_DYNAMIC_POINTER as well
		}

		// get the original value of A -- the actual string length -- back
		string_assign_ss << "\t" << "pla" << std::endl;
		this->stack_offset -= 1;
		max_offset -= 1;

		// next, store the length in the heap
		string_assign_ss << "\t" << "loady #$00" << std::endl;
		string_assign_ss << "\t" << "storea ($" << _LOCAL_DYNAMIC_POINTER << "), y" << std::endl;

		// next, get the value of our pointer and increment by 2 for memcpy
		string_assign_ss << "\t" << "loada $" << _LOCAL_DYNAMIC_POINTER << std::endl;
		string_assign_ss << "\t" << "clc" << std::endl;
		string_assign_ss << "\t" << "addca #$02" << std::endl;

		// the address of the string variable has already been pushed, so the next thing to push is the address of our destination
		string_assign_ss << "\t" << "pha" << std::endl;
		this->stack_offset += 1;
		max_offset += 1;

		// next, we need to push the length of the string, which we will get from our pointer variable
		string_assign_ss << "\t" << "loada ($" << _LOCAL_DYNAMIC_POINTER << "), y" << std::endl;
		string_assign_ss << "\t" << "pha" << std::endl;
		this->stack_offset += 1;
		max_offset += 1;
	}
	else {
		size_t previous_offset = this->stack_offset;	// we want to ensure that we know exactly where to return back to

		// now, we must move the stack pointer to the pointer variable; we don't need to set the retain registers flag because the addca method does not touch the B register and we have nothing valuable in A
		string_assign_ss << this->move_sp_to_target_address(target_symbol->stack_offset).str();

		// if we have an indexed assignment, we must navigate further into the stack
		if (target_symbol->type_information.get_primary() == ARRAY) {
			// preserve the A and B values by moving them to X and Y
			string_assign_ss << "\t" << "tax" << "\n\t" << "tba" << "\n\t" << "tay" << std::endl;

			// load the A register with the stack pointer; load the B register with the proper offset (in bytes) from the beginning of the variable
			string_assign_ss << "\t" << "tspa" << std::endl;
			string_assign_ss << "\t" << "loadb $" << std::hex << _LOCAL_DYNAMIC_POINTER << std::endl;
			// _subtract_ the value from A; stack grows downwards and the 0th element is highest up
			string_assign_ss << "\t" << "sec" << std::endl;
			string_assign_ss << "\t" << "subca b" << std::endl;
			string_assign_ss << "\t" << "tasp" << std::endl;

			// push the B value (in Y register), store it in _LOCAL_DYNAMIC_POINTER; use the A register so the index offset stays in B
			string_assign_ss << "\t" << "tya" << std::endl;
			string_assign_ss << "\t" << "pha" << std::endl;
			this->stack_offset += 1;	// since we pushed a value, increment the stack offset
			string_assign_ss << "\t" << "storea $" << _LOCAL_DYNAMIC_POINTER << std::endl;

			// now, the index offset is still in the B register; we must add it back to where it was before so we don't mess up the compiler's stack offset counter
			string_assign_ss << "\t" << "tspa" << std::endl;
			string_assign_ss << "\t" << "clc" << std::endl;
			string_assign_ss << "\t" << "addca b" << std::endl;
			string_assign_ss << "\t" << "tasp" << std::endl;

			// finally, move the X and Y values back into A and B
			string_assign_ss << "\t" << "tya" << "\n\t" << "tab" << "\n\t" << "txa" << std::endl;
		}
		else {
			// store the address of the dynamic memory in _LOCAL_DYNAMIC_POINTER in addition to putting it on the stack
			string_assign_ss << "\t" << "phb" << std::endl;
			this->stack_offset += 1;	// increase the stack offset so the compiler navigates the stack properly

			string_assign_ss << "\t" << "storeb $" << std::hex << _LOCAL_DYNAMIC_POINTER << std::endl;
		}

		// move the stack pointer back to where it was
		string_assign_ss << this->move_sp_to_target_address(previous_offset).str();

		// pull the length back into A and store it
		string_assign_ss << "\t" << "pla" << std::endl;
		this->stack_offset -= 1;
		max_offset -= 1;

		string_assign_ss << "\t" << "loady #$00" << std::endl;
		string_assign_ss << "\t" << "storea ($" << std::hex << _LOCAL_DYNAMIC_POINTER << "), y" << std::endl;

		// get the address of the dynamic memory and increment it by 2, for memcpy
		string_assign_ss << "\t" << "loada $" << _LOCAL_DYNAMIC_POINTER << std::endl;
		string_assign_ss << "\t" << "clc" << std::endl;
		string_assign_ss << "\t" << "addca #$02" << std::endl;

		// push the arguments to the stack
		string_assign_ss << "\t" << "pha" << std::endl;
		this->stack_offset += 1;
		max_offset += 1;

		string_assign_ss << "\t" << "loada ($" << std::hex << _LOCAL_DYNAMIC_POINTER << "), y" << std::endl;
		string_assign_ss << "\t" << "pha" << std::endl;
		this->stack_offset += 1;
		max_offset += 1;
	}

	// finally, invoke the subroutine
	string_assign_ss << "\t" << "jsr __builtins_memcpy" << std::endl;

	// move the stack offset back -- the memcpy routine pulls thrice from the stack
	this->stack_offset -= 3;

	// reset the pointers we used for string assignment
	string_assign_ss << "\t" << "loada #$00" << std::endl;
	string_assign_ss << "\t" << "storea __TEMP_A" << std::endl;
	string_assign_ss << "\t" << "storea __TEMP_B" << std::endl;
	string_assign_ss << "\t" << "storea __INPUT_LEN" << std::endl;

	// the memory has been successfully copied over; our assignment is done, so we can return the assembly we generated
	return string_assign_ss;
}


std::stringstream Compiler::assign(Assignment assignment_statement, size_t max_offset)
{
	/*
	
	Generates the SINASM16 code for an assignment statement 'assignment_statement'.
	The variable 'max_offset' contains the max offset from the stack frame base.
	
	*/

	// create a stringstream to which we will write write our generated code
	std::stringstream assignment_ss;

	exp_type lvalue_exp_type = assignment_statement.get_lvalue()->get_expression_type();
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
		throw CompilerException("Expression is not a modifiable-lvalue", 0, assignment_statement.get_line_number());
	}

	// look for the symbol of the specified name in the symbol table
	if (this->symbol_table.is_in_symbol_table(var_name, this->current_scope_name)) {
		// get the symbol information
		std::shared_ptr<Symbol> fetched_shared = this->symbol_table.lookup(var_name, this->current_scope_name, this->current_scope);
		Symbol* fetched;

		// make sure the symbol is a variable; else, throw an exception
		if (fetched_shared->symbol_type == VARIABLE) {
			fetched = dynamic_cast<Symbol*>(fetched_shared.get());
		}
		else {
			throw CompilerException("Expected modifiable-lvalue", 0, assignment_statement.get_line_number());
		}

		bool is_const = fetched->type_information.get_qualities().is_const();
		bool is_dynamic = fetched->type_information.get_qualities().is_dynamic();
		bool is_signed = fetched->type_information.get_qualities().is_signed();

		// if the lvalue_exp_type is 'indexed', make sure 'fetched->type' is either a string or an array -- those are the only data types we can index
		if (lvalue_exp_type == INDEXED && ((fetched->type_information != STRING) && (fetched->type_information != ARRAY))) {
			throw CompilerException("Cannot index variables of this type", 0, assignment_statement.get_line_number());
		}

		// if the quality is "const" throw an error; we cannot make assignments to const-qualified variables
		if (is_const) {
			throw CompilerException("Cannot make an assignment to a const-qualified variable!", 0, assignment_statement.get_line_number());
		}
		// if we have a dereferenced lvalue, make the assignment using that function
		else if (lvalue_exp_type == DEREFERENCED) {
			// first, make sure the symbol type is actually ptr<...> -- otherwise, throw an error
			Dereferenced* lvalue = dynamic_cast<Dereferenced*>(assignment_statement.get_lvalue().get());
			if (fetched->type_information.get_primary() == PTR) {
				assignment_ss << this->pointer_assignment(*lvalue, assignment_statement.get_rvalue(), assignment_statement.get_line_number(), max_offset).str();
			}
			else {
				throw CompilerException("You may not dereference a variable whose type is not ptr<...>", 0, assignment_statement.get_line_number());
			}
		}
		// otherwise, make the assignment
		else {
			/* 

			First, get the anticipated type of the rvalue
			If the rvalue expression type is ADDRESS_OF, its primary type should be PTR (as it is a "pointer literal") and its subtype should be the original primary type
			
			For example, an address of an integer will become:
				primary: PTR
				sub: INT
			while a pointer to an integer will become:
				primary: PTR
				sub: PTR

			*/

			DataType rvalue_data_type = this->get_expression_data_type(assignment_statement.get_rvalue(), assignment_statement.get_line_number());
			if (assignment_statement.get_rvalue()->get_expression_type() == ADDRESS_OF) {
				rvalue_data_type.set_subtype(rvalue_data_type.get_primary());
				rvalue_data_type.set_primary(PTR);
			}

			// if the types match, continue with the assignment
			if (this->types_are_compatible(assignment_statement.get_lvalue(), assignment_statement.get_rvalue(), assignment_statement.get_line_number())) {

				// dynamic memory must be handled a little differently than automatic memory because under the hood, it is implemented through pointers
				if (is_dynamic) {
					// we don't need to check if the memory has been freed here -- we do that in string assignment
					if (fetched->type_information.get_primary() == STRING) {
						// check to make sure the type isn't indexed; if it is, throw an exception -- string index assignment is disallowed
						if (lvalue_exp_type == INDEXED) {
							throw CompilerException("Index assignment on strings is forbidden", 0, assignment_statement.get_line_number());
						}
						else {
							// set the symbol to "defined" and call our string_assignment function
							fetched->defined = true;
							fetched->freed = false;
							assignment_ss << this->string_assignment(fetched, assignment_statement.get_rvalue(), assignment_statement.get_line_number(), max_offset).str();
						}
					}
					else if (fetched->type_information.get_primary() == ARRAY) {
						// todo: add support for arrays
					}
					else if (fetched->type_information.get_primary() == STRUCT) {
						// todo: add support for structs
					}
					else {
						assignment_ss << this->dynamic_assignment(fetched, assignment_statement.get_rvalue(), assignment_statement.get_line_number(), max_offset).str() << std::endl;
					}
				}
				// automatic and static memory are a little easier to handle than dynamic
				else {
					/*

					first, evaluate the right-hand statement
						(fetch_value will write assembly such the value currently in A (the value we fetched) is the evaluated rvalue)
					next, check to see whether the variable is /local/ or /global/; local variables will not use symbol names, but globals will

					*/

					// todo: add a function for array assignment using lists, separate from this one to keep code clean
					// todo: consider whether lists should be permissible _outside of_ initialization of an array
					// todo: split the assignment function into multiple smaller functions, one to handle dynamic memory, one to handle static, and one to handle automatic?
					
					// if the scope level is 0, we have static memory
					if (fetched->scope_level == 0) {
						// global variables

						// get the rvalue
						if (lvalue_exp_type == INDEXED) {
							// if we have a string, use string_assignment to handle it
							if (fetched->type_information.get_subtype() == STRING) {
								// get the index value in the Y register
								assignment_ss << this->fetch_value(assignment_index, assignment_statement.get_line_number(), max_offset).str();
								assignment_ss << "\t" << "lsl a" << std::endl;	// multiply by 2 _before_ we go the string assignment function
								assignment_ss << "\t" << "tay" << std::endl;
								assignment_ss << this->string_assignment(fetched, assignment_statement.get_rvalue(), assignment_statement.get_line_number(), max_offset).str(); // call the function to generate the assembly
							}
							else {
								// get the value of the index and push it onto the stack
								assignment_ss << this->fetch_value(assignment_index, assignment_statement.get_line_number(), max_offset).str();
								assignment_ss << "\t" << "pha" << std::endl;

								// get the rvalue
								assignment_ss << this->fetch_value(assignment_statement.get_rvalue(), assignment_statement.get_line_number(), max_offset).str() << std::endl;
								assignment_ss << "\t" << "tax" << std::endl;
								assignment_ss << "\t" << "pla" << std::endl;

								// we have to use an LSL because we need to multiply by 2 (the size of a word), but we want to shift in 0
								assignment_ss << "\t" << "lsl a" << std::endl;

								// finish getting the index
								assignment_ss << "\t" << "tay" << std::endl;
								assignment_ss << "\t" << "txa" << std::endl;
							}
						}
						else {
							assignment_ss << this->fetch_value(assignment_statement.get_rvalue(), assignment_statement.get_line_number(), max_offset).str() << std::endl;
							assignment_ss << "\t" << "loady #$00" << std::endl;
						}

						assignment_ss << "\t" << "storea " << var_name << ", y" << std::endl;	// y will be zero if we have no index
					}

					// otherwise, if the scope level > 0, we have automatic memory
					else {
						// if we are assigning to an index, we must do that differently than if it's to a non-indexed variable
						if (lvalue_exp_type == INDEXED) {
							// first, fetch the index value and push it to the stack
							assignment_ss << this->fetch_value(assignment_index, assignment_statement.get_line_number(), max_offset).str();

							// now, we will behave differently based on whether the subtype is string or not
							if (fetched->type_information.get_subtype() == STRING) {
								assignment_ss << "\t" << "lsl a" << std::endl;
								assignment_ss << "\t" << "tay" << std::endl;
								assignment_ss << this->string_assignment(fetched, assignment_statement.get_rvalue(), assignment_statement.get_line_number(), max_offset).str();
							}
							else {
								assignment_ss << "\t" << "tay" << std::endl;	// transfer A to Y so we don't have to set the preserve_registers flag
								assignment_ss << this->move_sp_to_target_address(max_offset).str();	// move the stack pointer to the stack frame
								assignment_ss << "\t" << "tya" << "\n\t" << "pha" << std::endl;	// push the index
								this->stack_offset += 1;

								// now, get the value we want
								assignment_ss << this->fetch_value(assignment_statement.get_rvalue(), assignment_statement.get_line_number(), max_offset).str() << std::endl;

								// move the stack pointer back to where it was, but preserve our registers
								assignment_ss << "\t" << "tax" << std::endl;
								assignment_ss << this->move_sp_to_target_address(max_offset + 1).str();

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
								this->stack_offset -= 1;
								assignment_ss << "\t" << "lsl a" << std::endl;	// multiply the index by 2 with lsl
								assignment_ss << "\t" << "tay" << std::endl;	// move the index into Y so it's safe to move the SP

								assignment_ss << this->move_sp_to_target_address(fetched->stack_offset).str();

								// move the index back into the B register
								assignment_ss << "\t" << "tya" << std::endl;
								assignment_ss << "\t" << "tab" << std::endl;

								// update the stack pointer value
								assignment_ss << "\t" << "tspa" << std::endl;
								assignment_ss << "\t" << "sec" << std::endl;
								assignment_ss << "\t" << "subca b" << std::endl;	// the B register holds the index
								assignment_ss << "\t" << "tasp" << std::endl;

								// push the value
								assignment_ss << "\t" << "txa" << std::endl;	// the value to assign was in X
								assignment_ss << "\t" << "pha" << std::endl;

								// move the stack pointer back to where it was (for the compiler's sake)
								assignment_ss << "\t" << "incsp" << std::endl;	// move it back to where it was before the push
								assignment_ss << "\t" << "tspa" << std::endl;
								assignment_ss << "\t" << "clc" << std::endl;
								assignment_ss << "\t" << "addca b" << std::endl;	// subtract the index from it
								assignment_ss << "\t" << "tasp" << std::endl;	// move the pointer value back
							}
						}
						else {
							assignment_ss << this->fetch_value(assignment_statement.get_rvalue(), assignment_statement.get_line_number(), max_offset).str() << std::endl;

							// move the SP to the target address, but store A and B in X and Y, respectively, before we move it
							assignment_ss << "\t" << "tax" << "\n\t" << "tby" << std::endl;
							assignment_ss << this->move_sp_to_target_address(fetched->stack_offset).str();
							assignment_ss << "\t" << "txa" << "\n\t" << "tyb" << std::endl;	// move the register values back

							assignment_ss << "\t" << "pha" << std::endl;
							this->stack_offset += 1;
						}
					}

					// update the symbol's "defined" status
					fetched->defined = true;
				}
			}
			// if the types do not match, we must throw an exception
			else {
				throw CompilerException("Cannot match '" + get_string_from_type(fetched->type_information.get_primary()) + "' and '" + get_string_from_type(rvalue_data_type.get_primary()) + "'", 0, assignment_statement.get_line_number());
			}
		}
	}
	// if we can't find the symbol table, throw an exception
	else {
		throw CompilerException("Could not find '" + var_name + "' in symbol table", 0, assignment_statement.get_line_number());
	}

	// return the assignment statement
	return assignment_ss;
}

std::stringstream Compiler::dynamic_assignment(Symbol* target_symbol, std::shared_ptr<Expression> rvalue, unsigned int line_number, size_t max_offset)
{
	std::stringstream dynamic_ss;

	// first, we need to fetch the value of the target symbol, as it is a pointer under the hood
	if (target_symbol->scope_level == 0)
	{
		dynamic_ss << "\t" << "loada " << target_symbol->name << std::endl;
	}
	else {
		dynamic_ss << this->move_sp_to_target_address(target_symbol->stack_offset + 1).str() << std::endl;
		dynamic_ss << "\t" << "pla" << std::endl;
		this->stack_offset -= 1;
	}

	// now, the A register contains the address of the dynamic data; preserve it
	dynamic_ss << "\t" << "prsa" << std::endl;

	// fetch the rvalue
	dynamic_ss << this->fetch_value(rvalue, line_number, max_offset).str() << std::endl;

	// restore the address and assign
	dynamic_ss << "\t" << "rstb" << "\n\t" << "tby" << std::endl;
	dynamic_ss << "\t" << "storea $00, y" << std::endl;

	target_symbol->defined = true;
	return dynamic_ss;
}

std::stringstream Compiler::pointer_assignment(Dereferenced lvalue, std::shared_ptr<Expression> rvalue, unsigned int line_number, size_t max_offset)
{
	/*
	
	Handles assignment where the lvalue is a dereferenced pointer, such as:
		let *a = 10;
	This is done by:
		1) fetching the address referenced by the pointer
		2) storing the rvalue at that address
	To fetch the address, we use fetch_value on the second level of the pointer -- for example, for the tree
			Dereferenced obj
				LValue obj
		We would pass the LValue in, as the LValue is what holds the address. Similarly,
			Dereferenced obj
				Dereferenced obj
						LValue obj
		We would pass in:
			Dereferenced obj
				LValue obj
		As that will give us the _address_ we want without giving us the value

	*/

	std::stringstream pointer_assignment_ss;

	// evaluate the rvalue first, preserve
	pointer_assignment_ss << this->fetch_value(rvalue, line_number, max_offset).str();
	pointer_assignment_ss << "\t" << "prsa" << std::endl;

	// fetch the value of the shared pointer held by this object, not the value of this object itself; this will give the address we want
	pointer_assignment_ss << this->fetch_value(lvalue.get_ptr_shared(), line_number, max_offset).str();

	// now, A holds the address; the value is on the stack
	pointer_assignment_ss << "\t" << "tay" << std::endl;

	/*
	
	if we have a local variable, we need to decrement Y by 1
	fetching the address of a stack value will give the value where the SP should be;
		since the stack grows downwards, we need to adjust so we write to the proper bytes

	todo: remodel so stack grows upwards? then this is a non-issue
	
	*/

	if (this->current_scope > 0) {
		pointer_assignment_ss << "\t" << "decy" << std::endl;
	}

	pointer_assignment_ss << "\t" << "rsta" << std::endl;
	pointer_assignment_ss << "\t" << "storea $00, y" << std::endl;

	return pointer_assignment_ss;
}
