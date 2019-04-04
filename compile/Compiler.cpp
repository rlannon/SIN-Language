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
				for (std::vector<Symbol>::iterator it = include_compiler->symbol_table.symbols.begin(); it != include_compiler->symbol_table.symbols.end(); it++) {
					this->symbol_table.insert(it->name, it->type, it->scope_name, it->scope_level, it->sub_type, it->qualities, it->defined, it->formal_parameters, include_statement.get_line_number());
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


std::stringstream Compiler::string_assignment(Symbol* target_symbol, std::shared_ptr<Expression> rvalue, unsigned int line_number, size_t max_offset)
{
	std::stringstream string_assign_ss;

	// if we have an indexed statement, temporarily store the Y value (the index value) in LOCAL_DYNAMIC_POINTER, as we don't need that address quite yet
	if (target_symbol->type == ARRAY) {
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
			if (target_symbol->type == ARRAY) {
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
			if (target_symbol->type == ARRAY) {
				// subtract the offset from the stack pointer
				string_assign_ss << "\t" << "tspa" << std::endl;
				string_assign_ss << "\t" << "subca $" << _LOCAL_DYNAMIC_POINTER << std::endl;
				string_assign_ss << "\t" << "tasp" << std::endl;

				// pull the value into B and adjust the compiler's offset counter
				string_assign_ss << "\t" << "plb" << std::endl;
				this->stack_offset -= 1;
				max_offset -= 1;

				// move the stack pointer back as many bytes as we advanced it
				string_assign_ss << "\t" << "tspa" << std::endl;
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

		if (target_symbol->type == ARRAY) {
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
		if (target_symbol->type == ARRAY) {
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
		if (target_symbol->type == ARRAY) {
			// preserve the A and B values by moving them to X and Y
			string_assign_ss << "\t" << "tax" << "\n\t" << "tba" << "\n\t" << "tay" << std::endl;

			// load the A register with the stack pointer; load the B register with the proper offset (in bytes) from the beginning of the variable
			string_assign_ss << "\t" << "tspa" << std::endl;
			string_assign_ss << "\t" << "loadb $" << std::hex << _LOCAL_DYNAMIC_POINTER << std::endl;
			// _subtract_ the value from A; stack grows downwards and the 0th element is highest up
			string_assign_ss << "\t" << "subca b" << std::endl;
			string_assign_ss << "\t" << "tasp" << std::endl;

			// push the B value (in Y register), store it in _LOCAL_DYNAMIC_POINTER; use the A register so the index offset stays in B
			string_assign_ss << "\t" << "tya" << std::endl;
			string_assign_ss << "\t" << "pha" << std::endl;
			this->stack_offset += 1;	// since we pushed a value, increment the stack offset
			string_assign_ss << "\t" << "storea $" << _LOCAL_DYNAMIC_POINTER << std::endl;

			// now, the index offset is still in the B register; we must add it back to where it was before so we don't mess up the compiler's stack offset counter
			string_assign_ss << "\t" << "tspa" << std::endl;
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


// allocate a variable
std::stringstream Compiler::allocate(Allocation allocation_statement, size_t* max_offset) {
	// first, add the variable to the symbol table
	// next, create a macro for it and assign it to the next available address in the scope; local variables will use pages incrementally
	// next, every time a variable is referenced, make sure it is in the symbol table and within the appropriate scope
	// after that, simply use that variable's name as a macro in the code
	// the macro will reference its address and hold the proper value

	std::stringstream allocation_ss;

	std::shared_ptr<Expression> initial_value = allocation_statement.get_initial_value();
	Symbol to_allocate(allocation_statement.get_var_name(), allocation_statement.get_var_type(), current_scope_name, current_scope, allocation_statement.get_var_subtype(), allocation_statement.get_qualities(), allocation_statement.was_initialized(), {}, allocation_statement.get_array_length());

	// get our qualities
	bool is_const = false;
	bool is_dynamic = false;
	bool is_signed;
	std::vector<SymbolQuality>::iterator quality_iter = to_allocate.qualities.begin();
	while (quality_iter != to_allocate.qualities.end()) {
		if (*quality_iter == CONSTANT) {
			is_const = true;
		}
		else if (*quality_iter == DYNAMIC) {
			is_dynamic = true;
		}
		else if (*quality_iter == UNSIGNED) {
			is_signed = false;
		}
		else if (*quality_iter == SIGNED) {
			is_signed = true;
		}

		quality_iter++;
	}

	// if we have a const, we can use the "@db" directive
	if (is_const) {
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

							allocation_ss << this->fetch_value(initial_value, allocation_statement.get_line_number(), *max_offset).str();
							allocation_ss << this->move_sp_to_target_address(*max_offset, true).str();	// increment the stack pointer to our stack frame, but preserve our register values

							// push our parameters and invoke our memcpy subroutine
							allocation_ss << "\t" << "phb" << "\n\t" << "loadb #" << to_allocate.name << "\n\t" << "phb" << "\n\t" << "pha" << std::endl;
							allocation_ss << "\t" << "jsr __builtins_memcpy" << std::endl;
						}
						// todo: add initializer-lists for arrays
						else {
							// now, use fetch_value to get the value of our lvalue and store the value in A at the constant we have defined
							allocation_ss << this->fetch_value(initial_value, allocation_statement.get_line_number(), *max_offset).str();
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
					this->evaluate_unary_tree(*initializer_unary, allocation_statement.get_line_number(), *max_offset).str();	// evaluate the unary expression
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
					this->evaluate_binary_tree(*initializer_binary, allocation_statement.get_line_number(), *max_offset).str();
					allocation_ss << "storea " << to_allocate.name << std::endl;
				}
				else {
					throw CompilerException("Types do not match", 0, allocation_statement.get_line_number());
				}
			}
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
				throw CompilerException("Arrays with subtype 'array' or 'struct' are not supported", 0, allocation_statement.get_line_number());
			}
			else {
				// reserve one word for each element
				size_t num_bytes = to_allocate.array_length * WORD_W;	// number of bytes = number of elements * number of bytes per word
				allocation_ss << "@rs " << std::dec << num_bytes << " " << to_allocate.name << std::endl;	// since we have been using std::hex, switch back to decimal mode here to be safe
			}
		}
		else if (to_allocate.type == STRUCT) {
			// TODO: implement structs
		}
		else {
			// reserve the variable itself first -- it may be a pointer to the variable if we need to dynamically allocate it
			allocation_ss << "@rs " << WORD_W  << " " << to_allocate.name << std::endl;

			// if the variable type is anything with a variable length, we need a different mechanism for handling them
			if (to_allocate.type == STRING) {
				// We can only allocate space dynamically if we have an initial value; we shouldn't guess on a size
				if (to_allocate.defined) {
					allocation_ss << this->string_assignment(&to_allocate, initial_value, allocation_statement.get_line_number(), *max_offset).str();

					// We need to update the symbol's 'allocated' member
					Symbol* allocated_symbol = this->symbol_table.lookup(to_allocate.name, to_allocate.scope_name, to_allocate.scope_level);
					allocated_symbol->allocated = to_allocate.allocated;
				}
			}
			else {
				// check to see if we have alloc-assign syntax for our other data types
				if (to_allocate.defined) {
					allocation_ss << this->fetch_value(allocation_statement.get_initial_value(), allocation_statement.get_line_number(), *max_offset).str();
					allocation_ss << "\t" << "storea " << to_allocate.name << std::endl;
				}
			}
		}
	}

	// handle all non-const local variables
	else {
		// make sure we have a valid max_offset pointer
		if (max_offset) {
			// our local variables will use the stack; they will directly modify the list of variable names and the stack offset
			allocation_ss << this->move_sp_to_target_address(*max_offset).str();	// make sure the stack pointer is at the end of the stack frame before allocating a local variable (so we don't overwrite anything)
			
			to_allocate.stack_offset = this->stack_offset;	// the stack offset for the symbol will now be the current stack offset
			this->symbol_table.insert(to_allocate, allocation_statement.get_line_number());	// now that the symbol's stack offset has been determined, we can add the symbol to the table

			// If the variable is defined, we can push its initial value
			if (to_allocate.defined) {
				// we must handle dynamic memory differently
				if (is_dynamic) {
					// we don't need to check if the variable has been freed when we are allocating it
					if (to_allocate.type == STRING) {
						// allocate a word on the stack for the pointer to the string
						this->stack_offset += 1;
						*max_offset += 1;
						allocation_ss << "\t" << "decsp" << std::endl;

						// strings will use the member function for string assignment
						allocation_ss << this->string_assignment(&to_allocate, initial_value, allocation_statement.get_line_number(), *max_offset).str();

						// update the symbol's 'allocated' member
						Symbol* allocated_symbol = this->symbol_table.lookup(to_allocate.name, to_allocate.scope_name, to_allocate.scope_level);
						allocated_symbol->allocated = to_allocate.allocated;
					}
					// TODO: implement more dynamic memory types
				}
				else {
					// get the initial value
					allocation_ss << this->fetch_value(initial_value, allocation_statement.get_line_number(), *max_offset).str();
					// push the A register and increment our counters
					allocation_ss << "\t" << "pha" << std::endl;
					this->stack_offset += 1;
					(*max_offset) += 1;
				}
			}
			else {
				// update the stack offset -- all types will increment the stack offset by one word; allocate space in the stack and increase our offset
				if (to_allocate.type == ARRAY) {
					allocation_ss << "\t" << "loada #$00" << std::endl;
					allocation_ss << "\t" << "loadx #$" << std::hex << to_allocate.array_length << std::endl;
					allocation_ss << ".__ALLOC_ARRAY_LOOP__BR_" << this->branch_number << "__:" << std::endl;
					allocation_ss << "\t" << "pha" << std::endl;
					allocation_ss << "\t" << "decx" << std::endl;
					allocation_ss << "\t" << "cmpx #$00" << std::endl;
					allocation_ss << "\t" << "brne .__ALLOC_ARRAY_LOOP__BR_" << this->branch_number << "__" << std::endl;

					this->branch_number += 1;
					this->stack_offset += to_allocate.array_length;
					(*max_offset) += to_allocate.array_length;
				}
				else {
					allocation_ss << "\t" << "decsp" << std::endl;
					this->stack_offset += 1;
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


std::stringstream Compiler::assign(Assignment assignment_statement, size_t max_offset) {

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
		throw CompilerException("Cannot use expression of this type in lvalue!", 0, assignment_statement.get_line_number());
	}

	// look for the symbol of the specified name in the symbol table
	if (this->symbol_table.is_in_symbol_table(var_name, this->current_scope_name)) {
		// get the symbol information
		Symbol* fetched = this->symbol_table.lookup(var_name, this->current_scope_name, this->current_scope);

		bool is_const = false;
		bool is_dynamic = false;
		bool is_signed;

		std::vector<SymbolQuality>::iterator quality_iter = fetched->qualities.begin();
		while (quality_iter != fetched->qualities.end()) {
			if (*quality_iter == CONSTANT) {
				is_const = true;
			}
			else if (*quality_iter == DYNAMIC) {
				is_dynamic = true;
			}
			else if (*quality_iter == SIGNED) {
				is_signed = true;
			}
			else if (*quality_iter == UNSIGNED) {
				is_signed = false;
			}

			quality_iter++;
		}
		
		// if the lvalue_exp_type is 'indexed', make sure 'fetched->type' is either a string or an array -- those are the only data types we can index
		if (lvalue_exp_type == INDEXED && ((fetched->type != STRING) && (fetched->type != ARRAY))) {
			throw CompilerException("Cannot index variables of this type", 0, assignment_statement.get_line_number());
		}

		// if the quality is "const" throw an error; we cannot make assignments to const-qualified variables
		if (is_const) {
			throw CompilerException("Cannot make an assignment to a const-qualified variable!", 0, assignment_statement.get_line_number());
		}
		// otherwise, make the assignment
		else {
			// get the anticipated type of the rvalue
			Type rvalue_data_type = this->get_expression_data_type(assignment_statement.get_rvalue());

			// if the types match, continue with the assignment
			if ((fetched->type == rvalue_data_type) || (fetched->sub_type == rvalue_data_type)) {

				// dynamic memory must be handled a little differently than automatic memory because under the hood, it is implemented through pointers
				if (is_dynamic) {
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
							assignment_ss << this->string_assignment(fetched, assignment_statement.get_rvalue(), assignment_statement.get_line_number(), max_offset).str();
						}
					}
					else {
						// TODO: add support for other dynamic types
						throw CompilerException("Other dynamic memory types not supported at this time");
					}
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
							// if we have a string, use string_assignment to handle it
							if (fetched->sub_type == STRING) {
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

						// now, make the assignment
						if (lvalue_exp_type != DEREFERENCED) {
							assignment_ss << "\t" << "storea " << var_name << ", y" << std::endl;	// y will be zero if we have no index
						}
						else {
							assignment_ss << "\t" << "storea (" << var_name << ", y)" << std::endl;	// use indirect addressing for pointers
						}
					}
					else {
						// if we are assigning to an index, we must do that differently than if it's to a non-indexed variable
						if (lvalue_exp_type == INDEXED) {
							// first, fetch the index value and push it to the stack
							assignment_ss << this->fetch_value(assignment_index, assignment_statement.get_line_number(), max_offset).str();

							// now, we will behave differently based on whether the subtype is string or not
							if (fetched->sub_type == STRING) {
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
						}
						else {
							assignment_ss << this->fetch_value(assignment_statement.get_rvalue(), assignment_statement.get_line_number(), max_offset).str() << std::endl;

							// move the SP to the target address, but store A and B in X and Y, respectively, before we move it
							assignment_ss << "\t" << "tax" << "\n\t" << "tba" << "\n\t" << "tay" << std::endl;
							assignment_ss << this->move_sp_to_target_address(fetched->stack_offset).str();
							assignment_ss << "\t" << "tya" << "\n\t" << "tab" << "\n\t" << "txa" << std::endl;	// move the register values back


							// make the assignment
							if (lvalue_exp_type != DEREFERENCED) {
								assignment_ss << "\t" << "pha" << std::endl;
								this->stack_offset += 1;	// when we push a variable, we need to update our stack offset
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
	// if we can't find the symbol table, throw an exception
	else {
		throw CompilerException("Could not find '" + var_name + "' in symbol table", 0, assignment_statement.get_line_number());
	}

	// return the assignment statement
	return assignment_ss;
}


std::stringstream Compiler::ite(IfThenElse ite_statement, size_t max_offset)
{
	/*
	
	Produce SINASM code for an If/Then/Else branch.

	*/

	// a stringstream for our generated asm
	std::stringstream ite_ss;

	//std::string parent_scope_name = this->current_scope_name;	// the parent scope name must be restored once we exit the scope

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
	std::vector<Symbol>::iterator it = this->symbol_table.symbols.begin();
	while (it != this->symbol_table.symbols.end()) {
		// only delete variables that are in the current subscope
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

			bool is_const = false;
			bool is_dynamic = false;
			bool is_signed;

			std::vector<SymbolQuality>::iterator quality_iter = to_free->qualities.begin();
			while (quality_iter != to_free->qualities.end()) {
				if (*quality_iter == CONSTANT) {
					is_const = true;
				}
				else if (*quality_iter == DYNAMIC) {
					is_dynamic = true;
				}
				else if (*quality_iter == SIGNED) {
					is_signed = true;
				}
				else if (*quality_iter == UNSIGNED) {
					is_signed = false;
				}

				quality_iter++;
			}

			// we can only free dynamic memory that hasn't already been freed
			if (!to_free->freed && (is_dynamic)) {
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

	// get the AST from the parser
	this->AST = parser.create_ast();

	this->current_scope = 0;	// start at the global scope
	this->current_scope_name = "global";

	this->stack_offset = 0;

	this->strc_number = 0;
	this->branch_number = 0;

	this->AST_index = 0;	// we use "get_next_statement()" every time, which increments before returning; as such, start at -1 so we fetch the 0th item, not the 1st, when we first call the compilation function
	
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
