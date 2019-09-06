/*

SIN Toolchain
Copyright 2019 Riley Lannon
Allocate.cpp

Contains the various functions to allocate data in the SIN compiler. Local and global variables use their own functions, as do global constants.
Dynamic memory, since it allocates a pointer to dynamic memory in the appropriate scope, is handled in local/global allocation functions.

*/

#include "Compiler.h"

std::stringstream Compiler::allocate(Allocation allocation_statement, size_t* max_offset) {
	/*

	Allocates a variable, adding it to the symbol table with the assistance of alloc_global(...) and alloc_local(...)

	*/

	std::stringstream allocation_ss;
	std::shared_ptr<Expression> initial_value = allocation_statement.get_initial_value();
	Symbol to_allocate(allocation_statement.get_var_name(), allocation_statement.get_type_information(), current_scope_name, current_scope, allocation_statement.was_initialized());

	// handle global variables -- they are those which have the "static" qualifier OR are declared at scope level 0
	if (to_allocate.type_information.get_qualities().is_static() || to_allocate.scope_level == 0) {
		// use our static_alloc function to allocate the global variable
		allocation_ss << this->alloc_global(&to_allocate, allocation_statement.get_line_number(), *max_offset, initial_value).str();

		// after we have successfully allocated the symbol, insert it into the table
		this->symbol_table.insert(std::make_shared<Symbol>(to_allocate), allocation_statement.get_line_number());
	}
	// handle local variables
	else {
		// make sure we have a valid max_offset pointer
		if (max_offset) {
			// our local variables will use the stack; they will directly modify the list of variable names and the stack offset
			allocation_ss << this->move_sp_to_target_address(*max_offset).str();	// move to the end of the stack frame
			to_allocate.stack_offset = this->stack_offset;	// the stack offset for the symbol will now be the current stack offset

			// allocate the variable
			allocation_ss << this->alloc_local(&to_allocate, allocation_statement.get_line_number(), max_offset, initial_value).str();

			// add the symbol to the table after it has been allocated
			this->symbol_table.insert(std::make_shared<Symbol>(to_allocate), allocation_statement.get_line_number());
		}
		else {
			// if we forgot to supply the address of our counter, it will throw an exception
			throw CompilerException("Cannot allocate memory for variable; expected pointer to stack offset counter, but found 'nullptr' instead.");
		}
	}

	// return the code generated for our allocation statement
	return allocation_ss;
}

std::stringstream Compiler::alloc_global(Symbol* to_allocate, unsigned int line_number, size_t max_offset, std::shared_ptr<Expression> initial_value)
{
	// Allocate static memory (global variable) -- note that some dynamic allocation may happen here

	std::stringstream alloc_global_ss;	// to contain our generated code
	
	if (to_allocate->type_information.get_qualities().is_const()) {
		alloc_global_ss << this->define_global_constant(to_allocate, line_number, max_offset, initial_value).str();
	}
	else {
		bool is_dynamic = to_allocate->type_information.get_qualities().is_dynamic();	// in case our static memory /points to/ dynamic memory

		// arrays must be allocated on their own because they are not fixed-width
		if (to_allocate->type_information.get_primary() == ARRAY) {
			// arrays may only contain the fundamental types; you can have an array of pointers to arrays or structs, but not of arrays or structs themselves (at this time)
			if (to_allocate->type_information.get_subtype() == ARRAY || to_allocate->type_information.get_subtype() == STRUCT) {
				throw CompilerException("Arrays may not contain other arrays nor structs (only pointers to such members)", 0, line_number);
			}
			else {
				// reserve one word for each element
				size_t num_bytes = to_allocate->type_information.get_array_length() * WORD_W;	// number of bytes = number of elements * number of bytes per word
				alloc_global_ss << "@rs " << std::dec << num_bytes << " " << to_allocate->name << std::endl;	// since we have been using std::hex, switch back to decimal mode here to be safe

				// if we initialzed the array, make the initial assignments
				if (initial_value) {
					std::shared_ptr<Expression> initial_exp = initial_value;

					// we cannot have an initialization of only one element -- the type must be 'LIST'
					if (initial_exp->get_expression_type() == LIST) {
						// first, get the list expression that is the initializer
						ListExpression* list_exp = dynamic_cast<ListExpression*>(initial_exp.get());
						std::vector<std::shared_ptr<Expression>> initializer_list = list_exp->get_list();

						// keep track of our index in the list by using the X register and the stack
						// since we are in the global scope, no need to move the SP around
						alloc_global_ss << "\t" << "loadx #$FFFE" << std::endl;

						// for each element in the list, fetch it and assign it to the next position in the list
						// todo: could refactor by creating a data array in memory containing the proper values and making the assignments by indexing into that array
						std::vector<std::shared_ptr<Expression>>::iterator it = initializer_list.begin();
						while (it != initializer_list.end()) {
							/*

							Start by incrementing the X register at the top of every loop; start at 0xFFFE so when we increment, it rolls over to 0
							This prevents us from pushing to the stack an extra time and requiring a 'decsp' instruction
							It's an optimization, albeit a tiny one

							After doing that, push it to the stack, fetch the value, pull the index from the stack, and make the assignment

							*/

							alloc_global_ss << "\t" << "incx" << std::endl;
							alloc_global_ss << "\t" << "incx" << std::endl;	// increment X twice to skip ahead one word
							alloc_global_ss << "\t" << "txa" << "\n\t" << "pha" << std::endl;

							// dereference 'it' as the shared_ptr to pass into fetch_value(...)
							alloc_global_ss << this->fetch_value(*it, line_number, max_offset).str() << std::endl;

							// now, get the index value from the stack into X and make the assignment
							alloc_global_ss << "\t" << "tab" << "\n\t" << "pla" << "\n\t" << "tax" << "\n\t" << "tba" << std::endl;
							alloc_global_ss << "\t" << "storea " << to_allocate->name << ", x" << std::endl;

							// increment the iterator
							it++;
						}
					}
					// if the initial value is not a LIST expression type, throw an error
					else {
						throw CompilerException("Expected initializer list for initialization of aggregate data type", 0,
							line_number);
					}
				}
			}
		}
		// structs also require special allocation
		else if (to_allocate->type_information.get_primary() == STRUCT) {
			// TODO: implement structs
			throw CompilerException("Structs currently unsupported", 0, line_number);
		}
		// all other types are fixed-width, and can be allocated the same (using @rs <size> <name>)
		else {
			// reserve the variable itself first -- it may be a pointer to the variable if we need to dynamically allocate it
			alloc_global_ss << "@rs " << std::dec << WORD_W << " " << to_allocate->name << std::endl;

			// if the variable type is anything with a variable length, we need a different mechanism for handling them
			if (to_allocate->type_information.get_primary() == STRING) {
				// We can only allocate space dynamically if we have an initial value; we shouldn't guess on a size
				if (to_allocate->defined) {
					alloc_global_ss << this->string_assignment(to_allocate, initial_value, line_number, max_offset).str();
				}
			}
			else {
				if (is_dynamic) {
					alloc_global_ss << "\t" << "loada #$" << std::hex << WORD_W << std::endl;
					alloc_global_ss << "\t" << "syscall #$" << MEMALLOC << std::endl;
					alloc_global_ss << "\t" << "storeb " << to_allocate->name << std::endl;	// store the dynamic address in the variable
				}

				// check to see if we have alloc-assign syntax for our other data types
				if (to_allocate->defined) {
					alloc_global_ss << this->fetch_value(initial_value, line_number, max_offset).str();

					if (is_dynamic) {
						alloc_global_ss << "\t" << "loady #" << to_allocate->name << std::endl;
						alloc_global_ss << "\t" << "storea $00, y" << std::endl;
					}
					else {
						alloc_global_ss << "\t" << "storea " << to_allocate->name << std::endl;
					}
				}
			}
		}
	}

	return alloc_global_ss;
}

std::stringstream Compiler::alloc_local(Symbol* to_allocate, unsigned int line_number, size_t* max_offset, std::shared_ptr<Expression> initial_value)
{
	// Allocate a local variable (or a local pointer to dynamic data)

	std::stringstream alloc_local_ss;
	
	// if we have a const local variable, we need to make sure it is defined with a value that can be computed at compile-time
	if (to_allocate->type_information.get_qualities().is_const()) {
		if (!to_allocate->defined) {
			throw CompilerException("Const-qualified variables must be initialized in allocation", 0, line_number);
		}
		else if (initial_value->get_expression_type() != LITERAL) {
			// todo: write some function to ensure the variable's value can be determined at compile time so "literal" doesn't have to be used
			throw CompilerException("Const-qualified variables must be initialized with literal values", 0, line_number);
		}
	}
	
	// check to see if the variable uses dynamic memory
	bool is_dynamic = to_allocate->type_information.get_qualities().is_dynamic();

	// if the variable is defined, we can push its initial value
	if (to_allocate->defined) {
		// we don't need to check if the variable has been freed when we are allocating it
		if (to_allocate->type_information.get_primary() == STRING) {
			// allocate a word on the stack for the pointer to the string
			this->stack_offset += 1;
			*max_offset += 1;
			alloc_local_ss << "\t" << "decsp" << std::endl;

			// strings will use the member function for string assignment
			alloc_local_ss << this->string_assignment(to_allocate, initial_value, line_number, *max_offset).str();

			// update the symbol's 'allocated' member
			std::shared_ptr<Symbol> fetched = this->symbol_table.lookup(to_allocate->name, to_allocate->scope_name, to_allocate->scope_level);
			Symbol* allocated_symbol = dynamic_cast<Symbol*>(fetched.get());	// todo: validate the symbol_type of fetched?
			allocated_symbol->allocated = to_allocate->allocated;
		}
		// array initializations must be handled slightly differently than other types
		else if (to_allocate->type_information.get_primary() == ARRAY) {
			// arrays must be initialized with initializer-lists
			if (initial_value->get_expression_type() == LIST) {
				// todo: initialize local arrays with lists
				ListExpression* list_exp = dynamic_cast<ListExpression*>(initial_value.get());
				std::vector<std::shared_ptr<Expression>> initializer_list = list_exp->get_list();

				// first, we must move to the end of the stack frame so we can use the stack without overwriting our local variables
				alloc_local_ss << this->move_sp_to_target_address(*max_offset).str();
				to_allocate->stack_offset = *max_offset;

				// next, we need to set up our loop
				// todo: we could also create a list in memory and index the list to make the assignment here using a loop -- could be useful for lists of constants
				for (std::vector<std::shared_ptr<Expression>>::iterator it = initializer_list.begin(); it != initializer_list.end(); it++) {
					// next, fetch the value
					alloc_local_ss << this->fetch_value(*it, line_number, *max_offset);

					// next, move the SP back to the end -- but only if we aren't at the end already
					if (this->stack_offset != *max_offset) {
						// if the difference between the two values is greater than three, we need to preserve the A register
						if (abs((int)this->stack_offset - (int)*max_offset) > 3) {
							alloc_local_ss << "\t" << "tab" << std::endl;	// preserve in B, as it's untouched by our move function
							alloc_local_ss << this->move_sp_to_target_address(*max_offset);
							alloc_local_ss << "\t" << "tba" << std::endl;
						}
						// otherwise, no sense in making the transfer
						else {
							alloc_local_ss << this->move_sp_to_target_address(*max_offset);
						}
					}

					// since the object is at the end, we don't actually have to move anywhere to push the value
					alloc_local_ss << "\t" << "pha" << std::endl;
					this->stack_offset += 1;
					*max_offset += 1;
				}
			}
			else {
				throw CompilerException("Expected initializer list for initialization of aggregate type", 0,
					line_number);
			}
		}
		// all other data types can simply fetch the value and push the A register
		else {
			if (is_dynamic) {
				// dynamically allocate one word; note this method does not check to make sure allocation was successful, the onus is on the programmer
				alloc_local_ss << "\t" << "loada #$" << std::hex << WORD_W << std::endl;
				alloc_local_ss << "\t" << "syscall #$" << std::hex << MEMALLOC << std::endl;	// allocate A bytes
				alloc_local_ss << "\t" << "prsb" << std::endl;	// preserve B (location)
				alloc_local_ss << this->fetch_value(initial_value, line_number, *max_offset).str();
				alloc_local_ss << "\t" << "rstb" << std::endl;	// restore B
				alloc_local_ss << "\t" << "phb" << std::endl;	// push the address to the stack
				this->stack_offset += 1;
				(*max_offset) += 1;
				alloc_local_ss << "\t" << "tby" << std::endl;
				alloc_local_ss << "\t" << "storea $00, y" << std::endl;	// assign to dynamic memory
			}
			else {
				// get the initial value
				alloc_local_ss << this->fetch_value(initial_value, line_number, *max_offset).str();
				// push the A register and increment our counters
				alloc_local_ss << "\t" << "pha" << std::endl;
				this->stack_offset += 1;
				(*max_offset) += 1;
			}
		}
	}
	// if it is not defined, 
	else {
		// arrays must be specially allocated
		if (to_allocate->type_information.get_primary() == ARRAY) {
			if (is_dynamic) {
				// todo: dynamic, undefined arrays
				throw CompilerException("Dynamic arrays currently unsupported", 0, line_number);
			}
			else {
				// load B with the length, transfer SP to A, subtract the length from the stack pointer, and move the stack pointer back
				alloc_local_ss << "\t" << "loadb #$" << std::hex << to_allocate->type_information.get_array_length() << std::endl;
				alloc_local_ss << "\t" << "tspa" << std::endl;
				alloc_local_ss << "\t" << "sec" << "\n\t" << "subca b" << std::endl;		// must always set carry before subtraction
				alloc_local_ss << "\t" << "tasp" << std::endl;

				// update compiler's stack offset counter
				this->stack_offset += to_allocate->type_information.get_array_length();
				(*max_offset) += to_allocate->type_information.get_array_length();
			}
		}
		// as must structs
		else if (to_allocate->type_information.get_primary() == STRUCT) {
			// todo: implement struct
			throw CompilerException("Structs currently unsupported", 0, line_number);
		}
		// other types don't need special treatment
		else {
			if (is_dynamic) {
				alloc_local_ss << "\t" << "loada #$" << std::hex << INT_W << std::endl;
				alloc_local_ss << "\t" << "syscall #$" << std::hex << MEMALLOC << std::endl;	// allocate A bytes
				alloc_local_ss << "\t" << "phb" << std::endl;	// push the address to the stack
				this->stack_offset += 1;
				(*max_offset) += 1;
			}
			else {
				alloc_local_ss << "\t" << "decsp" << std::endl;
				this->stack_offset += 1;
				(*max_offset) += 1;
			}
		}
	}

	return alloc_local_ss;
}

std::stringstream Compiler::define_global_constant(Symbol* to_allocate, unsigned int line_number, size_t max_offset, std::shared_ptr<Expression> initial_value) {
	// Define a global constant; they use @db instead of static memory

	std::stringstream def_const_ss;

	// constants must initialized when they are allocated (i.e. they must use alloc-assign syntax)
	if (to_allocate->defined) {
		// get the initial value's expression type and handle it accordingly
		if (initial_value->get_expression_type() == LITERAL) {
			// literal values are easy
			Literal* const_literal = dynamic_cast<Literal*>(initial_value.get());

			// make sure the types match
			if (to_allocate->type_information.is_compatible(const_literal->get_data_type())) {
				std::string const_value = const_literal->get_value();

				// use "@db"
				def_const_ss << "@db " << to_allocate->name << " (" << const_value << ")" << std::endl;	// todo: update constant definition syntax
			}
			else {
				throw CompilerException("Types are incompatible", 0, line_number);
			}
		}
		else if (initial_value->get_expression_type() == LVALUE) {
			// dynamic cast to lvalue
			LValue* initializer_lvalue = dynamic_cast<LValue*>(initial_value.get());

			// look through the symbol table to get the symbol
			std::shared_ptr<Symbol> fetched = this->symbol_table.lookup(initializer_lvalue->getValue(), this->current_scope_name, this->current_scope);	// this will throw an exception if the object isn't in the symbol table
			Symbol* initializer_symbol = nullptr;	// todo: eliminate the validation of 'fetched' ?

			if (fetched->symbol_type == VARIABLE) {
				initializer_symbol = dynamic_cast<Symbol*>(fetched.get());
			}
			else {
				throw CompilerException("Symbol found was not a variable symbol", 0, line_number);
			}

			// todo: consider whether run-time constants should be allowed
			// verify the lvalue we are initializing this const-qualified variable with is also const-qualified
			if (initializer_symbol->type_information.get_qualities().is_const()) {	// if the initializer is a constant
				// verify the symbol is defined
				if (initializer_symbol->defined) {
					// verify the types match -- we don't need to worry about subtypes just yet
					if (initializer_symbol->type_information.get_primary() == to_allocate->type_information.get_primary()) {
						// define the constant using a dummy value
						def_const_ss << "@db " << to_allocate->name << " (0)" << std::endl;

						// different data types must be treated differently
						if (to_allocate->type_information.get_primary() == STRING) {
							/*

							To allocate a string constant, we must fetch the value and then utilize memcpy to copy the data from our old location to the new one.

							Usage for memcpy is as follows:
								- Push source
								- Push destination
								- Push number of bytes to copy

							After fetch_value is called, A will contain the number of _bytes_, B will contain the address of the string

							*/

							def_const_ss << this->fetch_value(initial_value, line_number, max_offset).str();
							def_const_ss << this->move_sp_to_target_address(max_offset, true).str();	// increment the stack pointer to our stack frame, but preserve our register values

							// push our parameters and invoke our memcpy subroutine
							def_const_ss << "\t" << "phb" << "\n\t" << "loadb #" << to_allocate->name << "\n\t" << "phb" << "\n\t" << "pha" << std::endl;
							def_const_ss << "\t" << "jsr __builtins_memcpy" << std::endl;
						}
						// todo: add initializer-lists for arrays
						else {
							// now, use fetch_value to get the value of our lvalue and store the value in A at the constant we have defined
							def_const_ss << this->fetch_value(initial_value, line_number, max_offset).str();
							def_const_ss << "storea " << to_allocate->name << std::endl;
						}
					}
				}
				else {
					throw CompilerException("'" + initializer_symbol->name + "' was referenced before assignment.", 0, line_number);
				}
			}
			// otherwise, if it's not also a constant,
			else {
				throw CompilerException("Initializing const-qualified variables with non-const-qualified variables is illegal", 0, line_number);
			}
		}
		else if (initial_value->get_expression_type() == UNARY) {
			Unary* initializer_unary = dynamic_cast<Unary*>(initial_value.get());
			// if the types match
			if (this->get_expression_data_type(initial_value, line_number) == to_allocate->type_information) {
				def_const_ss << "@db " << to_allocate->name << " (0)" << std::endl;
				// get the evaluated unary
				this->evaluate_unary_tree(*initializer_unary, line_number, max_offset).str();	// evaluate the unary expression
				def_const_ss << "storea " << to_allocate->name << std::endl;
			}
			else {
				throw CompilerException("Types do not match", 0, line_number);
			}
		}
		else if (initial_value->get_expression_type() == BINARY) {
			Binary* initializer_binary = dynamic_cast<Binary*>(initial_value.get());
			// if the types match
			if (this->get_expression_data_type(initial_value, line_number) == to_allocate->type_information) {
				def_const_ss << "@db " << to_allocate->name << " (0)";
				this->evaluate_binary_tree(*initializer_binary, line_number, max_offset).str();
				def_const_ss << "storea " << to_allocate->name << std::endl;
			}
			else {
				throw CompilerException("Types do not match", 0, line_number);
			}
		}
		// it is illegal to initialize const-qualified variables with pointers
		else if (initial_value->get_expression_type() == DEREFERENCED || initial_value->get_expression_type() == ADDRESS_OF)
		{
			throw CompilerException("It is illegal to initialize const-qualified variables with pointers or addresses; these values must be computed at compile time", 0, line_number);
		}
		else {
			throw CompilerException("It is illegal to initialize a const-qualified variable with an expression of this type", 0, line_number);
		}
	}
	else {
		// this error should have been caught by the parser, but just to be safe...
		throw CompilerException("Const-qualified variables must be initialized in allocation", 0, line_number);
	}

	return def_const_ss;
}
