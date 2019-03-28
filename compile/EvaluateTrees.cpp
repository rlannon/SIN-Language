/*

SIN Toolchain
EvaluateTrees.cpp
Copyright 2019 Riley Lannon

Contains the implementation of two Compiler functions that evaluate unary and binary trees.

*/


#include "Compiler.h"


std::stringstream Compiler::evaluate_binary_tree(Binary bin_exp, unsigned int line_number, size_t max_offset, Type left_type)
{
	std::stringstream binary_ss;

	Binary current_tree = bin_exp;
	Expression* left_exp = current_tree.get_left().get();
	Expression* right_exp = current_tree.get_right().get();

	// first, move to the end of the stack frame
	binary_ss << this->move_sp_to_target_address(max_offset).str();

	/*

	The binary evaluation algorithm works as follows:
		1. Look at the left operand
			A. If the operand is another binary tree, call the function recursively on that tree (returning to step one)
			B. Otherwise, fetch the values in the expression
			C. Push appropriate register values to the stack
		2. Now look at the right operand
			A. If the operand is another binary tree, call the function recursively on that tree (returning to step one)
			B. Otherwise, fetch the values
			C. Move the values from A to B, or from A/B to the temp variables
			D. Pull the previously pushed values into A and B (depending on type)
		3. Evaluate the expression
			If the value is being returned from a recursive call, the result will be pushed to the stack or moved in registers as is appropriate

	Note that every time the stack is used, this->stack_offset and max_offset must be adjusted so that the values can be retrieved, pushed, pulled, and stored reliably. If stack_offset and max_offset are not adjusted, errors can and _will_ occur when working with the stack
	An optimization for this portion will be to only modify the stack pointer if we are not handling global variables, and to only transfer values between registers when necessary.

	*/

	// Check the left side
	if (left_exp->get_expression_type() == BINARY) {
		Binary* left_op = dynamic_cast<Binary*>(left_exp);

		if (left_type == NONE) {
			left_type = this->get_expression_data_type(current_tree.get_left());
		}

		binary_ss << this->evaluate_binary_tree(*left_op, line_number, max_offset, left_type).str();

		if (left_type == STRING) {
			binary_ss << "\t" << "tax" << "\n\t" << "tba" << "\n\t" << "tay" << std::endl;
			binary_ss << this->move_sp_to_target_address(max_offset).str();
			binary_ss << "\t" << "tya" << "\n\t" << "tab" << "\n\t" << "txa" << std::endl;
			binary_ss << "\t" << "pha" << "\n\t" << "phb" << std::endl;
			this->stack_offset += 2;
			max_offset += 2;
		}
		else {
			binary_ss << "\t" << "tax" << std::endl;
			binary_ss << this->move_sp_to_target_address(max_offset).str();
			binary_ss << "\t" << "txa" << std::endl;
			binary_ss << "\t" << "pha" << std::endl;
			this->stack_offset += 1;
			max_offset += 1;
		}
	}

	// By this point, 'left' is not a binary expression; evaluate the left hand side of the tree
	if (left_exp->get_expression_type() == LVALUE || left_exp->get_expression_type() == INDEXED || left_exp->get_expression_type() == LITERAL || left_exp->get_expression_type() == DEREFERENCED || left_exp->get_expression_type() == VALUE_RETURNING_CALL) {
		/*
		
		Variables, indexed values, literals, dereferenced values, and value returning functions call be handled with fetch_value
		
		*/
		
		bool get_sub = (left_exp->get_expression_type() == INDEXED);
		left_type = this->get_expression_data_type(bin_exp.get_left(), get_sub);

		binary_ss << this->fetch_value(bin_exp.get_left(), line_number, max_offset).str();

		if (left_type == STRING) {
			binary_ss << "\t" << "tax" << "\n\t" << "tba" << "\n\t" << "tay" << std::endl;
			binary_ss << this->move_sp_to_target_address(max_offset).str();
			binary_ss << "\t" << "tya" << "\n\t" << "tab" << "\n\t" << "txa" << std::endl;
			binary_ss << "\t" << "pha" << "\n\t" << "phb" << std::endl;
			this->stack_offset += 2;
			max_offset += 2;
		}
		else {
			binary_ss << "\t" << "tax" << std::endl;
			binary_ss << this->move_sp_to_target_address(max_offset).str();
			binary_ss << "\t" << "txa" << std::endl;
			binary_ss << "\t" << "pha" << std::endl;
			this->stack_offset += 1;
			max_offset += 1;
		}
	}
	else if (left_exp->get_expression_type() == UNARY) {
		Unary* unary_operand = dynamic_cast<Unary*>(left_exp);

		left_type = get_expression_data_type(unary_operand->get_operand());

		// signed arithmetic will depend on the evaluation of a unary expression; if there are no unaries, there is no sign
		// if the quality is "minus", set the "N" flag
		if (unary_operand->get_operator() == MINUS) {
			binary_ss << "\t" << "tay" << std::endl;
			binary_ss << "\t" << "tstatusa" << std::endl;
			binary_ss << "\t" << "ora #%10000000" << std::endl;
			binary_ss << "\t" << "tastatus" << std::endl;
			binary_ss << "\t" << "tya" << std::endl;
		}

		binary_ss << this->evaluate_unary_tree(*unary_operand, line_number, max_offset).str();
		binary_ss << "\t" << "tax" << std::endl;
		binary_ss << this->move_sp_to_target_address(max_offset).str();
		binary_ss << "\t" << "txa" << std::endl;
		binary_ss << "\t" << "pha" << std::endl;

		this->stack_offset += 1;
		max_offset += 1;
	}

	// Now that we have evaluated the left side, check to see if 'right' is a binary tree; if so, we will do the same thing we did for left
	if (right_exp->get_expression_type() == BINARY) {
		// our left operand data is already on the stack, so our registers are safe

		Binary* right_op = dynamic_cast<Binary*>(right_exp);
		binary_ss << this->evaluate_binary_tree(*right_op, line_number, max_offset, left_type).str();

		if (left_type == STRING) {
			// right argument goes into __TEMP_A and __TEMP_B, left goes in registers
			binary_ss << "\t" << "storea __TEMP_A" << std::endl;
			binary_ss << "\t" << "storeb __TEMP_B" << std::endl;
			binary_ss << "\t" << "plb" << "\n\t" << "pla" << std::endl;	// we pushed A and then B, so pull B and then A
			this->stack_offset -= 2;
			max_offset -= 2;
		}
		else {
			// right argument goes in B, left goes in A
			binary_ss << "\t" << "tab" << std::endl;
			binary_ss << "\t" << "pla" << std::endl;
			this->stack_offset -= 1;
			max_offset -= 1;
		}
	}
	else {
		// make sure the types match
		Type right_type = this->get_expression_data_type(current_tree.get_right());

		if (right_type == left_type) {
			if (right_exp->get_expression_type() == LVALUE || right_exp->get_expression_type() == LITERAL || right_exp->get_expression_type() == DEREFERENCED || right_exp->get_expression_type() == VALUE_RETURNING_CALL) {
				binary_ss << this->fetch_value(bin_exp.get_right(), line_number, max_offset).str();
			}
			else if (right_exp->get_expression_type() == UNARY) {
				Unary* unary_operand = dynamic_cast<Unary*>(right_exp);
				binary_ss << this->evaluate_unary_tree(*unary_operand, line_number, max_offset).str();
			}

			if (left_type == STRING) {
				// left side goes in registers, right in __TEMP variables
				binary_ss << "\t" << "storea __TEMP_A" << std::endl;
				binary_ss << "\t" << "storeb __TEMP_B" << std::endl;

				// make sure we are actually pulling the next thing we want to pull from the stack
				binary_ss << this->move_sp_to_target_address(max_offset).str();

				binary_ss << "\t" << "plb" << "\n\t" << "pla" << std::endl;	//pull the values
				this->stack_offset -= 2;	// update the stack offset
				max_offset -= 2;	// update the max offset; since we pulled, the address to pull from has changed
			}
			else {
				// left side goes in A, right side goes in B
				binary_ss << "\t" << "tax" << std::endl;	// protect the value in A
				binary_ss << this->move_sp_to_target_address(max_offset).str();	// move to the end of the stack frame so we can pull properly
				binary_ss << "\t" << "txa" << "\n\t" << "tab" << std::endl;	// get the right side in B
				binary_ss << "\t" << "pla" << std::endl;
				this->stack_offset -= 1;
				max_offset -= 1;
			}
		}
		// the types in a binary expression must match
		else {
			throw CompilerException("Types in binary expression do not match!", 0, line_number);
		}
	}

	// set STATUS bits accordingly
	if (left_type == FLOAT) {
		binary_ss << "\n\t" << "; Set 'float' bit in STATUS" << std::endl;
		binary_ss << "\t" << "tay" << std::endl;
		binary_ss << "\t" << "tstatusa" << std::endl;
		binary_ss << "\t" << "ora #%00000100" << std::endl;	// set the F bit in STATUS
		binary_ss << "\t" << "tastatus" << std::endl;
		binary_ss << "\t" << "tya" << std::endl << std::endl;
	}

	// now, we can perform the specified operation on the two operands
	if (bin_exp.get_operator() == PLUS) {
		// the + operator can be addition or string concatenation
		if (left_type == STRING) {
			/*

			If we have a string, we don't just use 'addca b'; we have to add to the length and then write to the location.
			However, we also have to check to see if we need to reallocate the string or if the space we allocated for it initially is enough. For this, we will use the realloc() function; if the amount of space allocated for the old string will hold the new string, then it won't change anything about the heap objects.

			The buffer at _STRING_BUFFER_START will contain the concatenated string, and the __INPUT_LEN variable will contain the length of the string. These values will also be loaded into B and A, respectively

			*/

			// store the length of the string in __INPUT_LEN so we can index the buffer's start address
			binary_ss << "\t" << "storea __INPUT_LEN" << std::endl;

			// if our left expression is binary, we don't need to do this -- the string data is already in the buffer
			if (bin_exp.get_left()->get_expression_type() != BINARY) {
				// copy the left argument into the string buffer
				binary_ss << "\t" << "phb" << std::endl;	// push the source
				binary_ss << "\t" << "loadb __INPUT_BUFFER_START_ADDR" << std::endl;	// get the destination and push it
				binary_ss << "\t" << "phb" << std::endl;
				binary_ss << "\t" << "pha" << std::endl;	// push the length

				// call __builtins_memcpy
				binary_ss << "\t" << "jsr __builtins_memcpy" << std::endl;
			}

			// load A with the length of the last string written and add the address of _STRING_BUFFER_START; this is our destination
			binary_ss << "\t" << "loada __INPUT_LEN" << std::endl;
			binary_ss << "\t" << "addca __INPUT_BUFFER_START_ADDR" << std::endl;

			// load B with the source address
			binary_ss << "\t" << "loadb __TEMP_B" << std::endl;

			// push the addresses -- B has the source, A has the destination
			binary_ss << "\t" << "phb" << "\n\t" << "pha" << std::endl;

			// push the length
			binary_ss << "\t" << "loada __TEMP_A" << std::endl;
			binary_ss << "\t" << "pha" << std::endl;

			// add that length to the length of the other string, which is in __INPUT_LEN
			binary_ss << "\t" << "addca __INPUT_LEN" << std::endl;
			binary_ss << "\t" << "storea __INPUT_LEN" << std::endl;

			// copy the memory
			binary_ss << "\t" << "jsr __builtins_memcpy" << std::endl;

			// we are done; the concatenated string is at the string buffer and __INPUT_LEN contains the new length
			// all we need to do is load our A and B registers
			binary_ss << "\t" << "loadb __INPUT_BUFFER_START_ADDR" << std::endl;
			binary_ss << "\t" << "loada __INPUT_LEN" << std::endl;
		}
		else {
			binary_ss << "\t" << "addca b" << std::endl;
		}
	}
	else if (bin_exp.get_operator() == MINUS) {
		binary_ss << "\t" << "subca b" << std::endl;
	}
	else if (bin_exp.get_operator() == MULT) {
		// if we have _signed numbers_, use MULTA; otherwise use MULTUA
		if (this->is_signed(std::make_shared<Binary>(bin_exp), line_number)) {
			binary_ss << "\t" << "multa b" << std::endl;
		}
		else {
			binary_ss << "\t" << "multua b" << std::endl;
		}
	}
	else if (bin_exp.get_operator() == DIV) {
		// if we have signed numbers, use DIVA; otherwise, use DIVUA
		if (this->is_signed(std::make_shared<Binary>(bin_exp), line_number)) {
			binary_ss << "\t" << "diva b" << std::endl;
		}
		else {
			binary_ss << "\t" << "divua b" << std::endl;
		}
	}
	else if (bin_exp.get_operator() == MODULO) {
		// to get the modulo, simply use a mult instruction and transfer B (the remainder) into A
		binary_ss << "\t" << "diva b" << std::endl;
		binary_ss << "\t" << "tba" << std::endl;
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
	// TODO: add BIT_AND/BIT_OR binary operators

	return binary_ss;
}

std::stringstream Compiler::evaluate_unary_tree(Unary unary_exp, unsigned int line_number, size_t max_offset)
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
		unary_ss << this->fetch_value(unary_exp.get_operand(), line_number, max_offset).str();
	}
	else if (unary_exp.get_operand()->get_expression_type() == BINARY) {
		// TODO: parse binary tree to get the result in A
	}
	else if (unary_exp.get_operand()->get_expression_type() == UNARY) {
		// Our unary operand can be another unary expression -- if so, simply get the operand and call this function recursively
		Unary* unary_operand = dynamic_cast<Unary*>(unary_exp.get_operand().get());	// cast to appropriate type
		unary_ss << this->evaluate_unary_tree(*unary_operand, line_number, max_offset).str();	// add the produced code to our code here
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
