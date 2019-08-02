/*

SIN Toolchain
ParserUtil.cpp
Copyright 2019 Riley Lannon

Contains the implementations of various utility functions for the parser.

*/

#include "Parser.h"


// Define our symbols and their precedences as a vector of tuples containing the operator string and its precedence (size_t)
const std::vector<std::tuple<std::string, size_t>> Parser::precedence{ std::make_tuple("or", 2), std::make_tuple("and", 2), std::make_tuple("!", 2),
	std::make_tuple("<", 4), std::make_tuple(">", 7), std::make_tuple("<", 7), std::make_tuple(">=", 7), std::make_tuple("<=", 7), std::make_tuple("=", 7),
	std::make_tuple("!=", 7), std::make_tuple("|", 8), std::make_tuple("^", 8), std::make_tuple("&", 9), std::make_tuple("+", 10),
	std::make_tuple("-", 10),std::make_tuple("$", 15), std::make_tuple("*", 20), std::make_tuple("/", 20), std::make_tuple("%", 20) };

const size_t Parser::get_precedence(std::string symbol, size_t line) {
	// Iterate through the vector and find the tuple that matches our symbol; if found, return its precedence; if not, throw an exception and return 0
	std::vector<std::tuple<std::string, size_t>>::const_iterator it = Parser::precedence.begin();
	bool match = false;
	size_t precedence;

	while (it != Parser::precedence.end()) {
		if (std::get<0>(*it) == symbol && !match) {
			match = true;
			precedence = std::get<1>(*it);
		}
		else {
			it++;
		}
	}

	if (match) {
		return precedence;
	}
	else {
		throw ParserException("Unknown operator '" + symbol + "'!", 0, line);
		return 0;
	}
}

// Utility functinos for traversing the token list

bool Parser::is_at_end() {
	// Determines whether we have run out of tokens. Returns true if we have, false if not.
	if (this->position >= this->num_tokens - 2 || this->num_tokens == 0) {	// the last element is list.size() - 1;  if we are at list.size(), we have gone over
		return true;
	}
	else {
		return false;
	}
}

lexeme Parser::peek() {
	// peek to the next position
	if (position + 1 < this->tokens.size()) {
		return this->tokens[this->position + 1];
	}
	else {
		throw ParserException("No more lexemes to parse!", 1, this->tokens[this->position].line_number);
	}
}

lexeme Parser::next() {
	// Increments the position and returns the token there, provided we haven't hit the end

	// increment the position
	this->position += 1;

	// if we haven't hit the end, return the next token
	if (position < this->tokens.size()) {
		return this->tokens[this->position];
	}
	// if we have hit the end
	else {
		throw ParserException("No more lexemes to parse!", 1, this->tokens[this->position - 1].line_number);
	}
}

lexeme Parser::current_token() {
	return this->tokens[this->position];
}

lexeme Parser::previous() {
	return this->tokens[this->position - 1];
}

lexeme Parser::back() {
	this->position -= 1;
	return this->tokens[this->position];
}

void Parser::skipPunc(char punc) {
	// Skip a punctuation mark

	if (this->current_token().type == "punc") {
		if (this->current_token().value == &punc) {
			this->position += 1;
			return;
		}
		else {
			return;
		}
	}
	else {
		return;
	}
}

bool Parser::is_type(std::string lex_value)
{
	// Determines whether a given string is a type name

	std::string type_strings[] = { "int", "bool", "string", "float", "raw", "ptr", "array", "struct" };

	// iterate through our list of type names
	size_t i = 0;
	bool found = false;

	while (i < type_strings->size() && !found) {
		if (lex_value == type_strings[i]) {
			found = true;
		}
	}

	return found;
}

std::string Parser::get_closing_grouping_symbol(std::string beginning_symbol)
{
	/*

	Returns the appropriate closing symbol for some opening grouping symbol 'beginning_symbol'; e.g., if beginning_symbol is '(', the function will return ')'

	Operates with strings because that's what lexemes use

	*/

	if (beginning_symbol == "(") {
		return ")";
	}
	else if (beginning_symbol == "[") {
		return "]";
	}
	else if (beginning_symbol == "{") {		// while curly braces are not considered grouping symbols by these functions, we will include it here
		return "}";
	}
	else if (beginning_symbol == "<") {
		return ">";
	}
	else {
		throw ParserException("Invalid grouping symbol in expression!", 0);
		return 0;
	}
}

bool Parser::is_opening_grouping_symbol(std::string to_test)
{
	/*

	Checks to see whether a given string is a opening parenthesis or a bracket.
	Curly braces not included here because they are not considered grouping symbols like ( and [ -- they serve a different purpose

	*/
	return (to_test == "(" || to_test == "[");
}

const bool Parser::has_return(StatementBlock to_test)
{
	/*
	
	Checks to see whether a statement block 'to_test' has a return statement in it.
	This is useful for checking to ensure all of our functions have return statements in them.
	This can be modified in the future to test whether all control paths return in a function, which is required. For now, keep it simple.
	
	*/

	// our base case is that the statement block has a return statement
	if (to_test.has_return) {
		return true;
	}
	// if it doesn't, check to see whether we have a scope block in the last statement
	else {
		// if we have no statements in the scope block, we obviously have no return statement
		if (to_test.statements_list.size() == 0) {
			return false;
		}
		else {
			/*
			
			The last statement in the function _must_ contain a return. For example:
				def void myFunc(alloc int myVar) {
					if (myVar = 0) {
						return void;
					}
					else {
						@print("myVar is not 0");
					}

					@print("unreachable");
				}
				
				This will generate the error that the last print is unreachable, but it should also generate the error that the function does not return.
				This is because the last statement has no return.
			
			*/
			
			// get the last statement and check its type
			Statement* last_statement = to_test.statements_list.back().get();
			if (last_statement->get_statement_type() == IF_THEN_ELSE) {
				IfThenElse* ite = dynamic_cast<IfThenElse*>(last_statement);

				// the branches may be null pointers; we have to be careful here so we can't just pass *ite->get_else_branch().get() in directly
				StatementBlock* if_branch = ite->get_if_branch().get();
				StatementBlock* else_branch = ite->get_else_branch().get();

				bool if_has_return = has_return(*if_branch);
				bool else_has_return = false;

				if (else_branch == nullptr) {
					else_has_return = true;
				}
				else {
					else_has_return = has_return(*else_branch);
				}

				// if both branches return a value, we are golden
				if (if_has_return && else_has_return) {
					return true;
				}
				else {
					compiler_warning("Not all control paths return a value", last_statement->get_line_number());
					return if_has_return;
				}
			}
			else if (last_statement->get_statement_type() == WHILE_LOOP) {
				WhileLoop* while_loop = dynamic_cast<WhileLoop*>(last_statement);

				// while loops are a little simpler, we can simply pass in the branch for the while loop
				return has_return(*while_loop->get_branch().get());
			}
			else {
				return false;
			}
		}
	}
}

DataType Parser::get_type()
{
	/*

	Gets the qualities and type data about a symbol in an allocation or declaration. The tuple contains:
		0 - Type	-	The data type
		1 - Type	-	The subtype
		2 - vector	-	Symbol qualities
		3 - size_t	-	The array size, if applicable

	*/

	// the parse function that calls this one will have advanced the token iterator to the first token in the type data
	lexeme current_lex = this->current_token();

	// check our quality, if any
	std::vector<SymbolQuality> qualities;
	if (current_lex.value == "const") {
		// change the variable quality
		qualities.push_back(CONSTANT);

		// get the actual variable type
		current_lex = this->next();
	}
	else if (current_lex.value == "dynamic") {
		qualities.push_back(DYNAMIC);
		current_lex = this->next();
	}
	else if (current_lex.value == "static") {
		qualities.push_back(STATIC);
		current_lex = this->next();
	}

	// check to see if we have a sign quality; this always comes immediately before the type
	if (current_lex.value == "unsigned") {
		// make sure the next value is 'int' by checking the 
		if (this->peek().value == "int") {
			qualities.push_back(UNSIGNED);
			current_lex = this->next();
		}
		else {
			throw ParserException("Cannot use sign qualifier for variable of this type", 0, current_lex.line_number);
		}
	}
	else if (current_lex.value == "signed") {
		// make sure the next value is 'int'
		if (this->peek().value == "int") {
			qualities.push_back(SIGNED);
			current_lex = this->next();
		}
		else {
			throw ParserException("Cannot use sign qualifier for variable of this type", 0, current_lex.line_number);
		}
	}

	// set the quality to DYNAMIC if we have a string
	if (current_lex.value == "string") {
		qualities.push_back(DYNAMIC);
	}

	Type new_var_type;
	Type new_var_subtype = NONE;
	size_t array_length = 0;

	if (current_lex.value == "ptr") {
		// set the type
		new_var_type = PTR;

		// 'ptr' must be followed by '<'
		if (this->peek().value == "<") {
			this->next();
			// a keyword must be in the angle brackets following 'ptr'
			if (this->peek().type == "kwd") {
				lexeme subtype = this->next();
				new_var_subtype = get_type_from_string(subtype.value);

				// the next character must be ">"
				if (this->peek().value == ">") {
					// skip the angle bracket
					this->next();
				}
				// if it isn't, throw an exception
				else if (this->peek().value != ">") {
					throw ParserException("Pointer type must be enclosed in angle brackets", 212, current_lex.line_number);
				}
			}
		}
		// if it's not, we have a syntax error
		else {
			throw ParserException("Proper syntax is 'alloc ptr<type>'", 212, current_lex.line_number);
		}
	}
	// otherwise, if it's an array,
	else if (current_lex.value == "array") {
		new_var_type = ARRAY;
		// check to make sure we have the size and type in angle brackets
		if (this->peek().value == "<") {
			this->next();	// eat the angle bracket

			// we must see the size next; must be an int
			if (this->peek().type == "int") {
				array_length = (size_t)std::stoi(this->next().value);	// get the array length

				// a comma should follow the size
				if (this->peek().value == ",") {
					this->next();
					std::shared_ptr<LValue> new_var_struct_name;	// in case we have a struct

					// next, we should see a keyword or an identifier (we can have an array of structs, which the lexer would see as an ident)
					if (this->peek().type == "kwd") {
						// new_var_subtype will be the type indicated
						new_var_subtype = get_type_from_string(this->next().value);
					}
					else if (this->peek().type == "ident") {
						// the subtype will be struct, and we should set the struct type as the identifier
						new_var_subtype = STRUCT;
						new_var_struct_name = std::make_shared<LValue>(this->next().value);
					}
					else {
						throw ParserException("Invalid subtype in array allocation", 0, this->peek().line_number);
					}

					// now, we should see a closing angle bracket and the name of the array
					if (this->peek().value == ">") {
						this->next();	// eat the angle bracket
					}
				}
				else {
					throw ParserException("The size of an array must be followed by the type", 0, current_lex.line_number);
				}
			}
			else {
				throw ParserException("The size of an array must be a positive integer expression", 0, current_lex.line_number);
			}
		}
		else {
			throw ParserException("You must specify the size and type of an array", 0, current_lex.line_number);
		}
	}
	// otherwise, if it is not a pointer or an array,
	else {
		// if we have an int, but we haven't pushed back signed/unsigned, default to signed
		if (current_lex.value == "int") {
			// if the last quality in the list (which is our signed/unsigned specifier, if we have one) is not signed and also not unsigned, push back 'signed'
			if (qualities.size() != 0 && (qualities[qualities.size() - 1] != SIGNED && qualities[qualities.size() - 1] != UNSIGNED)) {
				qualities.push_back(SIGNED);
			}
			// if we don't have _any_ qualities, push back 'signed'
			else if (qualities.size() == 0) {
				qualities.push_back(SIGNED);
			}
		}

		// store the type name in our Type object
		new_var_type = get_type_from_string(current_lex.value); // note: Type get_type_from_string() is found in "Expression" (.h and .cpp)
	}

	// create the symbol type data
	DataType symbol_type_data(new_var_type, new_var_subtype, qualities, array_length);
	return symbol_type_data;
}

std::vector<SymbolQuality> Parser::get_postfix_qualities()
{
	/*

	Symbol qualities can be given after an allocation -- as such, the following statements are valid:
		alloc const unsigned int x: 10;
		alloc int x: 10 &const unsigned;

	This function begins on the first token of the postfix quality -- that is to say, "current_token" should be the ampersand.

	*/

	std::vector<SymbolQuality> qualities = {};	// create our qualities vector, initialize to an empty vector

	// a keyword should follow the '&'
	if (this->peek().type == "kwd") {
		// continue parsing our SymbolQualities until we hit a semicolon, at which point we will trigger the 'done' flag
		bool done = false;
		while (this->peek().type == "kwd" && !done) {
			lexeme quality_token = this->next();	// get the token for the quality
			SymbolQuality quality = this->get_quality(quality_token);	// use our 'get_quality' function to get the SymbolQuality based on the token

			// if the quality is NO_QUALITY, there was an error; don't add it to the vector
			if (quality != NO_QUALITY) {
				qualities.push_back(quality);
			}

			// the quality must be followed by either another quality or a semicolon
			if (this->peek().value == ";") {
				done = true;
			}
			// there's an error if the next token is not a keyword and also not a semicolon
			else if (this->peek().type != "kwd") {
				throw ParserException("Expected ';' or symbol qualifier in expression", 0, this->peek().line_number);
			}
		}
	}
	else {
		throw ParserException("Expected symbol quality following '&'", 0, this->current_token().line_number);
	}

	return qualities;
}

SymbolQuality Parser::get_quality(lexeme quality_token)
{
	// Given a lexeme containing a quality, returns the appropriate member from SymbolQuality

	const SymbolQuality qualities[5] = { CONSTANT, STATIC, DYNAMIC, SIGNED, UNSIGNED };
	const std::string quality_string[5] = { "const", "static", "dynamic", "signed", "unsigned" };
	SymbolQuality to_return = NO_QUALITY;

	// ensure the token is a kwd
	if (quality_token.type == "kwd") {
		// iterate through the quality_string array and compare quality_token.value with it to find the index of the appropriate quality
		bool found = false;
		size_t index = 0;
		while (index < num_qualities && !found) {
			if (quality_token.value == quality_string[index]) {
				found = true;
			}
			else {
				index++;
			}
		}

		if (found) {
			to_return = qualities[index];
		}
		else {
			throw ParserException("Invalid qualifier", 0, quality_token.line_number);
		}
	}
	else {
		throw ParserException("Invalid qualifier", 0, quality_token.line_number);
	}

	return to_return;
}
