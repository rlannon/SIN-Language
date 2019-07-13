/*

SIN Toolchain
ParseExpression.cpp
Copyright 2019 Riley Lannon

Contains the implementations of the functions to parse expressions, including:
	- std::shared_ptr<Expression> parse_expression(size_t prec=0, std::string grouping_symbol = "(", bool not_binary = false);
	- std::shared_ptr<Expression> create_dereference_object();
	- LValue getDereferencedLValue(Dereferenced to_eval);
	- std::shared_ptr<Expression> maybe_binary(std::shared_ptr<Expression> left, size_t my_prec, std::string grouping_symbol = "(");

*/

#include "Parser.h"

std::shared_ptr<Expression> Parser::parse_expression(size_t prec, std::string grouping_symbol, bool not_binary) {
	lexeme current_lex = this->current_token();

	// Create a pointer to our first value
	std::shared_ptr<Expression> left;

	// Check if our expression begins with a grouping symbol; if so, only return what is inside the symbols
	// note that curly braces are NOT included here; they are parsed separately as they are not considered grouping symbols in the same way as parentheses and brackets are
	if (is_opening_grouping_symbol(current_lex.value)) {
		grouping_symbol = current_lex.value;
		this->next();
		left = this->parse_expression(0, grouping_symbol);
		this->next();

		/*

		if we are getting the expression within an indexed expression, we don't want to parse out a binary (otherwise it might parse:
			let myArray[3] = 0;
		as having the expression 3 = 0, which is not correct

		*/
		if (this->current_token().value == "]" && not_binary) {
			return left;
		}

		// Otherwise, carry on parsing
		// if our next character is a semicolon or closing paren, then we should just return the expression we just parsed
		if (this->peek().value == ";" || this->peek().value == get_closing_grouping_symbol(grouping_symbol) || this->peek().value == "{") {
			return left;
		}
		// if our next character is an op_char, returning the expression would skip it, so we need to parse a binary using the expression in parens as our left expression
		else if (this->peek().value == "op_char") {
			return this->maybe_binary(left, prec, grouping_symbol);
		}
	}
	// list expressions (array literals) have to be handled slightly differently than other expressions
	else if (current_lex.value == "{") {
		this->next();
		std::vector<std::shared_ptr<Expression>> list_members = {};

		// as long as the next token is a comma, we have elements to parse
		while (this->peek().value != "}" && this->peek().value != ";") {
			list_members.push_back(this->parse_expression(prec, "{"));
			this->next();	// skip the last character of the expression
		}

		// once we escape the loop, we must find a closing curly brace
		if (this->peek().value == ";") {
			left = std::make_shared<ListExpression>(list_members);
		}
		else {
			throw ParserException("Invalid character in expression", 0, this->current_token().line_number);
		}
	}
	// if expressions are separated by commas, continue parsing the next one
	else if (current_lex.value == ",") {
		this->next();
		return this->parse_expression(prec, grouping_symbol, not_binary);
	}
	// if it is not an expression within a grouping symbol, it is parsed below
	else if (is_literal(current_lex.type)) {
		left = std::make_shared<Literal>(get_type_from_string(current_lex.type), current_lex.value);
	}
	else if (current_lex.type == "ident") {
		// check to see if we have the identifier alone, or whether we have an index
		if (this->peek().value == "[") {
			this->next();
			left = std::make_shared<Indexed>(current_lex.value, "var", this->parse_expression(0, "[", true));
		}
		else {
			left = std::make_shared<LValue>(current_lex.value);
		}
	}
	// if we have a keyword to begin an expression, parse it (could be a sizeof expression)
	else if (current_lex.type == "kwd") {
		if (current_lex.value == "sizeof") {
			// expression must be enclosed in angle brackets
			if (this->peek().value == "<") {
				grouping_symbol = "<";
				this->next();
				// valid words in sizeof are idents and types, so long as the type is not a struct or an array
				if (this->peek().type == "ident" || (is_type(this->peek().value) && this->peek().value != "struct" && this->peek().value != "array")) {
					lexeme to_check = this->next();

					if (this->peek().value == get_closing_grouping_symbol(grouping_symbol)) {
						this->next();	// eat the end paren
						left = std::make_shared<SizeOf>(to_check.value);
					}
					else {
						throw ParserException("Syntax error; expected '>'", 0, current_lex.line_number);
					}
				}
				else {
					throw ParserException("Invalid 'sizeof' argument", 0, current_lex.line_number);
				}
			}
			else {
				throw ParserException("Syntax error; expected '<'", 0, current_lex.line_number);
			}
		}
		else {
			throw ParserException("Invalid keyword in expression", 0, current_lex.line_number);
		}
	}
	// if we have an op_char to begin an expression, parse it (could be a pointer or a function call)
	else if (current_lex.type == "op_char") {
		// if we have a function call
		if (current_lex.value == "@") {
			current_lex = this->next();

			if (current_lex.type == "ident") {
				// Same code as is in statement
				std::vector<std::shared_ptr<Expression>> args;

				// make sure we have parens -- if not, throw an exception
				if (this->peek().value != "(") {
					throw ParserException("Syntax error; expected parens enclosing arguments in function call.", 0, current_lex.line_number);
				}
				else {

					this->next();
					this->next();
					while (this->current_token().value != get_closing_grouping_symbol(grouping_symbol)) {
						args.push_back(this->parse_expression());
						this->next();
					}
				}

				// assemble the value returning call so we can pass into maybe_binary
				left = std::make_shared<ValueReturningFunctionCall>(std::make_shared<LValue>(current_lex.value, "func"), args);
			}
			// the "@" character must be followed by an identifier
			else {
				throw ParserException("Expected identifier in function call", 330, current_lex.line_number);
			}
		}
		// check to see if we have the address-of operator
		else if (current_lex.value == "$") {
			// if we have a $ character, it HAS TO be the address-of operator
			// current lexeme is the $, so get the variable for which we need the address
			lexeme next_lexeme = this->next();
			// the next lexeme MUST be an identifier
			if (next_lexeme.type == "ident") {

				// turn the identifier into an LValue
				LValue target_var(next_lexeme.value, "var_address");

				// get the address of the vector position of the variable
				return std::make_shared<AddressOf>(target_var);
			}
			// if it's not, throw an exception
			else {
				throw ParserException("An address-of operator must be followed by an identifier; illegal to follow with '" + next_lexeme.value + "' (not an identifier)", 111, current_lex.line_number);
			}

		}
		// check to see if we have a pointer dereference operator
		else if (current_lex.value == "*") {
			left = this->create_dereference_object();
		}
		// check to see if we have a unary operator
		else if ((current_lex.value == "+") || (current_lex.value == "-") || (current_lex.value == "!")) {
			// get the next leceme
			lexeme next = this->next();
			// declare our operand
			std::shared_ptr<Expression> operand;

			if (next.type == "ident") {
				// make a shared pointer to our variable (lvalue, type will be "var")
				operand = std::make_shared<LValue>(next.value);
			}
			else if (next.type == "int") {
				// make our operand a literal
				operand = std::make_shared<Literal>(INT, next.value);
			}
			else if (next.type == "float") {
				// make our operand a literal
				operand = std::make_shared<Literal>(FLOAT, next.value);
			}
			else {
				// TODO: fix parser exception code for unary +
				throw ParserException("Cannot use unary operators with this type", 000, current_lex.line_number);
			}

			// now, "operand" should have our operand (and if the type was invalid, it will have thrown an error)
			// make a unary + or - depending on the type; we have already checked to make sure it's a valid unary operator
			if (current_lex.value == "+") {
				left = std::make_shared<Unary>(operand, PLUS);
			}
			else if (current_lex.value == "-") {
				left = std::make_shared<Unary>(operand, MINUS);
			}
			else if (current_lex.value == "!") {
				left = std::make_shared<Unary>(operand, NOT);
			}
		}
	}

	// Use the maybe_binary function to determine whether we need to return a binary expression or a simple expression

	// always start it at 0; the first time it is called, it will be 0, as nothing will have been passed to parse_expression, but will be updated to the appropriate precedence level each time after. This results in a binary tree that shows the proper order of operations
	return this->maybe_binary(left, prec, grouping_symbol);
}


// Create a Dereferenced object when we dereference a pointer
std::shared_ptr<Expression> Parser::create_dereference_object() {
	// if we have an asterisk, it could be a pointer dereference OR a part of a binary expression
	// in order to check, we have to make sure that the previous character is neither a literal nor an identifier
	// the current lexeme is the asterisk, so get the previous lexeme
	lexeme previous_lex = this->previous();	// note that previous() does not update the current position

	// if it is an int, float, string, or bool literal; or an identifier, then continue
	if (previous_lex.type == "int" || previous_lex.type == "float" || previous_lex.type == "string" || previous_lex.type == "bool" || previous_lex.type == "ident") {
		// do nothing
		// TODO: this control path does not return a value -- what to do in this event?
	}
	// otherwise, check to make see if the next character is an identifier
	else if (this->peek().type == "ident") {
		// get the identifier and advance the position counter
		lexeme next_lexeme = this->next();

		// turn the pointer into an LValue
		LValue _ptr(next_lexeme.value, "var_dereferenced");

		// return a shared_ptr to the Dereferenced object containing _ptr
		return std::make_shared<Dereferenced>(std::make_shared<LValue>(_ptr));
	}
	// the next character CAN be an asterisk; in that case, we have a double or triple ref pointer that we need to parse
	else if (this->peek().value == "*") {
		// advance the position pointer
		this->next();
		// dereference the pointer to get the address so we can dereference the other pointer
		std::shared_ptr<Expression> deref = this->create_dereference_object();
		if (deref->get_expression_type() == DEREFERENCED) {
			// get the Dereferenced obj
			return std::make_shared<Dereferenced>(deref);
		}
	}
	// if it is not a literal or an ident and the next character is also not an ident or asterisk, we have an error
	else {
		throw ParserException("Expected an identifier in pointer dereference operation", 332, current_token().line_number);
	}
}


// get the end LValue pointed to by a pointer recursively
LValue Parser::getDereferencedLValue(Dereferenced to_eval) {
	// if the type of the Expression within "to_eval" is an LValue, we are done
	if (to_eval.get_ptr_shared()->get_expression_type() == LVALUE) {
		return to_eval.get_ptr();
	}
	// otherwise, if it is another Dereferenced object, get the object stored within that
	// the recutsion here will return the LValue pointed to by the last pointer
	else if (to_eval.get_ptr_shared()->get_expression_type() == DEREFERENCED) {
		Dereferenced* _deref = dynamic_cast<Dereferenced*>(to_eval.get_ptr_shared().get());
		return this->getDereferencedLValue(*_deref);
	}
}

std::shared_ptr<Expression> Parser::maybe_binary(std::shared_ptr<Expression> left, size_t my_prec, std::string grouping_symbol) {

	// Determines whether to wrap the expression in a binary or return as is

	lexeme next = this->peek();

	// if the next character is a semicolon, another end paren, or a comma, return
	if (next.value == ";" || next.value == get_closing_grouping_symbol(grouping_symbol) || next.value == ",") {
		return left;
	}
	// Otherwise, if we have an op_char or the 'and' or 'or' keyword
	else if (next.type == "op_char" || next.value == "and" || next.value == "or") {
		// if the operator is '&', it could be used for bitwise-and OR for postfixed symbol qualities; if the token following is a keyword, it cannot be bitwise-and
		if (next.value == "&") {
			this->next();	// advance the iterator so we can see what comes after the ampersand
			lexeme operand = this->peek();
			
			// if the operand is a keyword, the & must not be intended to be the bitwise-and operator
			if (operand.type == "kwd") {
				this->back();	// move the iterator back
				return left;	// return our left argument
			}
			else {
				this->back();	// move the iterator back where it was
			}
		}

		// get the next op_char's data
		int his_prec = get_precedence(next.value, next.line_number);

		// If the next operator is of a higher precedence than ours, we may need to parse a second binary expression first
		if (his_prec > my_prec) {
			this->next();	// go to the next character in our stream (the op_char)
			this->next();	// go to the character after the op char

			// Parse out the next expression
			std::shared_ptr<Expression> right = this->maybe_binary(this->parse_expression(his_prec, grouping_symbol), his_prec, grouping_symbol);	// make sure his_prec gets passed into parse_expression so that it is actually passed into maybe_binary

			// Create the binary expression
			std::shared_ptr<Binary> binary = std::make_shared<Binary>(left, right, translate_operator(next.value));	// "next" still contains the op_char; we haven't updated it yet

			// call maybe_binary again at the old prec level in case this expression is followed by one of a higher precedence
			return this->maybe_binary(binary, my_prec, grouping_symbol);
		}
		else {
			return left;
		}

	}
	// There shouldn't be anything besides a semicolon, closing paren, or an op_char immediately following "left"
	else {
		throw ParserException("Invalid character in expression", 312, current_token().line_number);
	}
}
