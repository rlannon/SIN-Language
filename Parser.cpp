#include "Parser.h"


// define "lexeme" data type
typedef std::tuple<std::string, std::string> lexeme;


// Define our symbols and their precedences as a vector of tuples
const std::vector<std::tuple<std::string, int>> Parser::precedence{ std::make_tuple("&", 2), std::make_tuple("|", 3), \
	std::make_tuple("<", 4), std::make_tuple(">", 7), std::make_tuple("<", 7), std::make_tuple(">=", 7), std::make_tuple("<=", 7), std::make_tuple("=", 7),\
	std::make_tuple("!=", 7), std::make_tuple("+", 10), std::make_tuple("-", 10), std::make_tuple("*", 20), std::make_tuple("/", 20), std::make_tuple("%", 20) };

// Iterate through the vector and find the tuple that matches our symbol; if found, return its precedence; if not, return -1
const int Parser::getPrecedence(std::string symbol) {
	std::vector<std::tuple<std::string, int>>::const_iterator it = Parser::precedence.begin();
	bool match = false;
	int precedence;

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
		std::cerr << "No such operator \"" << symbol << "\" found in operator precedence vector!" << std::endl;
		return -1;	// -1 will be our error value
	}
}



// Tells us whether we have run out of tokens
bool Parser::is_at_end() {
	if (this->position >= this->num_tokens - 2) {	// the last element is list.size() - 1;  if we are at list.size(), we have gone over
		return true;
	}
	else {
		return false;
	}
}

// peek to the next position
lexeme Parser::peek() {
	if (position + 1 <= this->tokens.size()) {
		return this->tokens[this->position + 1];
	}
	else {
		//this->error("No more lexemes to parse!", 1);
		throw ParserException("No more lexemes to parse!", 1);
	}
}

lexeme Parser::next() {
	//	std::cout << std::endl << this->current_token << std::endl << std::get<0>(this->tokens[this->current_token]) << std::endl;
	this->position += 1;
	if (position <= this->tokens.size()) {
		return this->tokens[this->position];
	}
	else {
		this->error("No more lexemes to parse!", 1);
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

void Parser::error(std::string message, int code) {
	//throw ParserException(code, message);
	throw ParserException(message, code);
}

// Skip a punctuation mark
void Parser::skipPunc(char punc) {
	if (std::get<0>(this->current_token()) == "punc") {
		if (std::get<1>(this->current_token()) == &punc) {
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


/*

Create an abstract syntax tree using Parser::tokens. This is used not only as the entry function to the parser, but whenever an AST is needed as part of a statement. For example, a "Definition" statement requires an AST as one of its members; parse_top() is used to genereate the function's procedure's AST.

*/

StatementBlock Parser::createAST() {
	// allocate a StatementBlock, which will be used to store our AST
	StatementBlock prog;

	// creating an empty lexeme will allow us to test if the current token has nothing in it
	// sometimes, the lexer will produce a null lexeme, so we want to skip over it if we find one
	std::tuple<std::string, std::string> null_lexeme = std::make_tuple("", "");

	// Parse a token file
	// While we are within the program and we have not reached the end of a procedure block, keep parsing
	while (!this->is_at_end() && !this->quit && !(std::get<1>(this->peek()) == "}")) {
		// skip any semicolons and newline characters, if there are any in the tokens list
		this->skipPunc(';');
		this->skipPunc('\n');
		
		// if we encounter a null lexeme, skip it
		while (this->current_token() == null_lexeme) {
			this->next();
		}

		// Parse a statement and add it to our AST
		prog.StatementsList.push_back(this->parseStatement());

		// check to see if we are at the end now that we have advanced through the tokens list; if not, continue; if so, do nothing and the while loop will abort and return the AST we have produced
		if (!this->is_at_end() && !(std::get<1>(this->peek()) == "}")) {
			this->next();
		}
	}

	// return the AST
	return prog;
}



/*

Parse a statement. This function looks at the next token to determine what it needs to do, calling the appropriate functions if necessary.

*/

std::shared_ptr<Statement> Parser::parseStatement() {
	// get our current lexeme and its information so we don't need to call these functions every time we need to reference it
	lexeme current_lex = this->current_token();
	std::string lex_type = std::get<0>(current_lex);
	std::string lex_val = std::get<1>(current_lex);

	// create a shared_ptr to the statement we are going to parse so that we can return it when we are done
	std::shared_ptr<Statement> stmt;

	// first, we will check to see if we need any keyword parsing
	if (lex_type == "kwd") {

		// Check to see what the keyword is

		if (lex_val == "if") {
			// Get the next lexeme
			lexeme next = this->next();

			// Check to see if condition is enclosed in parens
			if (std::get<1>(next) == "(") {
				// get the condition
				std::shared_ptr<Expression> condition = this->parseExpression();
				// Initialize the if_block
				StatementBlock if_branch;
				StatementBlock else_branch;
				// Check to make sure a curly brace follows the condition
				next = this->peek();
				if (std::get<1>(next) == "{") {
					this->next();
					this->next();	// skip ahead to the first character of the statementblock
					if_branch = this->createAST();
					this->next();	// skip the closing curly brace

					// First, check for an else clause
					if (!this->is_at_end()) {
						// Now, check to see if we have an 'else' clause
						next = this->peek();
						std::cout << std::endl;

						if (std::get<1>(next) == "else") {
							this->next();
							next = this->peek();
							// Again, check for curlies
							if (std::get<1>(next) == "{") {
								this->next();
								this->next();	// skip ahead to the first character of the statementblock
								else_branch = this->createAST();
								this->next();	// skip the closing curly brace

								return std::make_shared<IfThenElse>(condition, std::make_shared<StatementBlock>(if_branch), std::make_shared<StatementBlock>(else_branch));
							}
							else {
								this->error("Expected '{' after 'else' in conditional", 331);
							}
						}
					}

					return std::make_shared<IfThenElse>(condition, std::make_shared<StatementBlock>(if_branch));
				}
				// If our condition is not followed by an opening curly
				else {
					this->error("Expected '{' after condition in conditional", 331);
				}
			}
			// If condition is not enclosed in parens
			else {
				this->error("Condition must be enclosed in parens", 331);
			}

		}
		else if (lex_val == "alloc") {
			// Create objects for our variable's type and name
			Type new_var_type;
			std::string new_var_name = "";

			// check our next token; it must be a keyword
			lexeme var_type = this->next();
			if (std::get<0>(var_type) == "kwd") {
				if (std::get<1>(var_type) == "int" || std::get<1>(var_type) == "float" || std::get<1>(var_type) == "bool" || std::get<1>(var_type) == "string" || std::get<1>(var_type) == "ptr") {
					// Note: pointers must be treated a little bit differently than other fundamental types
					// if we have a pointer,
					if (std::get<1>(var_type) == "ptr") {
						// 'ptr' must be followed by '['
						if (std::get<1>(this->peek()) == "[") {
							this->next();
							// a keyword must be in the square brackets following 'ptr'
							if (std::get<0>(this->peek()) == "kwd") {
								var_type = this->next();
								// append "ptr" to the type, so we have, for example, "intptr" or "stringptr"
								std::get<1>(var_type) += "ptr";
								new_var_type = get_type_from_string(std::get<1>(var_type));	// note: Type get_type_from_string() is found in "Expression" (.h and .cpp)

								// the next character must be "]"
								if (std::get<1>(this->peek()) == "]") {
									// skip the square bracket
									this->next();
								}
								// if it isn't, throw an exception
								else if (std::get<1>(this->peek()) != "]") {
									throw ParserException("Pointer type must be enclosed in square brackets", 212);
								}
							}
						}
						// if it's not, we have a syntax error
						else {
							throw ParserException("Proper syntax is 'alloc ptr[type]'", 212);
						}
					}
					// otherwise, if it is not a pointer,
					else {
						// store the type name in our Type object
						new_var_type = get_type_from_string(std::get<1>(var_type)); // note: Type get_type_from_string() is found in "Expression" (.h and .cpp)
					}

					// next, get the variable's name
					// the following token must be an identifier
					lexeme var_name = this->next();
					if (std::get<0>(var_name) == "ident") {
						new_var_name = std::get<1>(var_name);
						
						// Finally, return our new variable
						return std::make_shared<Allocation>(new_var_type, new_var_name);
					}
					else {
						//this->error("Expected an identifier", 111);
						throw ParserException("Expected an identifier", 111);
					}
				}
				else {
					this->error("Expected a variable type; must be int, float, bool, or string", 211);
				}
			}
			else {
				this->error("Expected a variable type; token type must be a keyword", 111);
			}
		}
		// Parse an assignment
		else if (lex_val == "let") {
			// Create a shared_ptr to our assignment expression
			std::shared_ptr<Assignment> assign;
			// Create an LValue object for our left expression
			LValue lvalue;

			// if the next lexeme is an op_char, we have a pointer
			if (std::get<0>(this->peek()) == "op_char") {
				// get the pointer operator
				lexeme ptr_op = this->next();
				// check to see if it is an address-of or dereference operator
				if (std::get<1>(ptr_op) == "$") {
					// set the LValue_Type to "var_address"
					// might not need to do this -- as it will be dealing with the pointer data itself, not the variable to which it points
				}
				else if (std::get<1>(ptr_op) == "*") {
					// set the LValue_Type to "var_dereferenced"
					lvalue.setLValueType("var_dereferenced");
				}
				// if it isn't $ or *, it's an invalid op_char before an LValue
				else {
					throw ParserException("Operator character not allowed in an LValue", 211);
				}
			}

			// get the next token, which should be the variable name
			lexeme _lvalue_lex = this->next();

			// ensure it's an identifier
			if (std::get<0>(_lvalue_lex) == "ident") {
				lvalue.setValue(std::get<1>(_lvalue_lex));
			}
			// if it isn't a valid LValue, then we can't continue
			else {
				this->error("Expected an LValue", 111);
			}

			// get the operator character, make sure it's an equals sign
			lexeme _operator = this->next();
			if (std::get<1>(_operator) == "=") {
				// create a shared_ptr for our rvalue expression
				std::shared_ptr<Expression> rvalue;
				this->next();
				rvalue = this->parseExpression();

				assign = std::make_shared<Assignment>(lvalue, rvalue);
				return assign;
			}
		}
		// Parse a return statement
		else if (lex_val == "return") {
			this->next();	// go to the expression
			std::shared_ptr<Expression> return_exp = this->parseExpression();
			return std::make_shared<ReturnStatement>(return_exp);
		}
		// Parse a 'while' loop
		else if (lex_val == "while") {
			// A while loop is very similar to a "for" loop in how we parse it; the only difference is we don't need to check for an "else" branch
			std::shared_ptr<Expression> condition;	// create the object for our condition
			StatementBlock branch;	// and for the loop body

			if (std::get<1>(this->peek()) == "(") {
				this->next();
				condition = this->parseExpression();
				if (std::get<1>(this->peek()) == "{") {
					this->next();
					this->next();	// skip opening curly
					branch = this->createAST();

					// If we are not at the end, go to the next token
					if (!(this->is_at_end())) {
						this->next();
					}

					// Make a pointer to our branch
					std::shared_ptr<StatementBlock> loop_body = std::make_shared<StatementBlock>(branch);

					// return our object
					return std::make_shared<WhileLoop>(condition, loop_body);
				}
				else {
					this->error("Loop body must be enclosed in curly braces", 331);
				}
			}
			else {
				this->error("Expected a condition", 331);
			}
		}
		// Parse a function declaration
		else if (lex_val == "def") {
			// First, get the type of function that we have -- the return value
			lexeme func_type = this->next();
			Type return_type;
			// func_type must be a keyword
			if (std::get<0>(func_type) == "kwd") {
				return_type = get_type_from_string(std::get<1>(func_type));
				// Get the function name and verify it is of the correct type
				lexeme func_name = this->next();
				if (std::get<0>(func_name) == "ident") {
					lexeme _peek = this->peek();
					if (std::get<1>(_peek) == "(") {
						this->next();
						// Create our arguments vector and our StatementBlock variable
						StatementBlock procedure;
						std::vector<std::shared_ptr<Statement>> args;
						// Populate our arguments vector if there are arguments
						if (std::get<1>(this->peek()) != ")") {
							this->next();
							while (std::get<1>(this->current_token()) != ")") {
								args.push_back(this->parseStatement());
								this->next();
							}
						}
						else {
							this->next();	// skip the closing paren
						}
						// Args should be empty if we don't have any
						// Now, check to make sure we have a curly brace
						if (std::get<1>(this->peek()) == "{") {
							this->next();
							this->next();
							procedure = this->createAST();
							this->next();	// skip closing curly brace

							// Return the pointer to our function
							std::shared_ptr<LValue> _func = std::make_shared<LValue>(std::get<1>(func_name), "func");
							return std::make_shared<Definition>(_func, return_type, args, std::make_shared<StatementBlock>(procedure));
						}
						else {
							this->error("Function definition requires use of curly braces after arguments", 331);
						}
					}
					else {
						this->error("Function definition requires '(' and ')'", 331);
					}
				}
				// if NOT "ident"
				else {
					this->error("Expected identifier", 330);
				}
			}
		}
		// if none of the keywords were valid, throw an error
		else {
			this->error("Invalid keyword", 211);
		}

	}

	// if it's not a keyword, check to see if we need to parse a function call
	else if (lex_type == "op_char") {	// "@" is an op_char, but we may update the lexer to make it a "control_char"
		if (lex_val == "@") {
			// Get the function's name
			lexeme func_name = this->next();
			if (std::get<0>(func_name) == "ident") {
				std::vector<std::shared_ptr<Expression>> args;
				this->next();
				this->next();
				while (std::get<1>(this->current_token()) != ")") {
					args.push_back(this->parseExpression());
					this->next();
				}
				while (std::get<1>(this->peek()) != ";") {
					this->next();
				}
				this->next();
				return std::make_shared<Call>(std::make_shared<LValue>(std::get<1>(func_name), "func"), args);
			}
			else {
				this->error("Expected an identifier", 111);
			}

		}
	}

	// if it is a punctuation character, specifically a curly brace, advance the character
	else if (lex_type == "punc" && lex_val == "}") {
		this->next();
	}

	// otherwise, if the lexeme is not a valid beginning to a statement, abort
	else {
		this->error("Lexeme '" + lex_val + "' is not a valid beginning to a statement", 000);
		std::exception my_ex;
	}

	// return the statement
	return stmt;
}

std::shared_ptr<Expression> Parser::parseExpression(int prec) {
	lexeme current_lex = this->current_token();
	std::string lex_type = std::get<0>(current_lex);
	std::string lex_val = std::get<1>(current_lex);

	// Create a pointer to our first value
	std::shared_ptr<Expression> left;

	// Check if our expression begins with parens; if so, only return what is inside them
	if (lex_val == "(") {
		this->next();
		left = this->parseExpression();
		this->next();
		// if our next character is a semicolon or closing paren, then we should just return the expression we just parsed
		if (std::get<1>(this->peek()) == ";" || std::get<1>(this->peek()) == ")" || std::get<1>(this->peek()) == "{") {
			return left;
		}
		// if our next character is an op_char, returning the expression would skip it, so we need to parse a binary using the expression in parens as our left expression
		else if (std::get<1>(this->peek()) == "op_char") {
			return this->maybeBinary(left, prec);
		}
	}
	else if (lex_val == ",") {
		this->next();
		return this->parseExpression();
	}
	// if it is not an expression within parens
	else if (is_literal(lex_type)) {
		left = std::make_shared<Literal>(get_type_from_string(lex_type), lex_val);
	}
	else if (lex_type == "ident") {
		left = std::make_shared<LValue>(lex_val);
	}
	// if we have an op_char to begin an expression, parse it (could be a pointer or a function call)
	else if (lex_type == "op_char") {

		// TODO: Fix double ref pointer -- Lexer currently sees "**" as one lexeme, when it should be 2

		// if we have a function call
		if (lex_val == "@") {
			current_lex = this->next();
			lex_type = std::get<0>(current_lex);
			lex_val = std::get<1>(current_lex);

			if (lex_type == "ident") {
				// Same code as is in statement
				std::vector<std::shared_ptr<Expression>> args;
				this->next();
				this->next();
				while (std::get<1>(this->current_token()) != ")") {
					args.push_back(this->parseExpression());
					this->next();
				}
				return std::make_shared<ValueReturningFunctionCall>(std::make_shared<LValue>(lex_val, "func"), args);
			}
			// the "@" character must be followed by an identifier
			else {
				this->error("Expected identifier in function call", 330);
			}
		}
		// check to see if we have the address-of operator
		else if (lex_val == "$") {

			// TODO: parse address-of operator

			// if we have a $ character, it HAS TO be the address-of operator
			// current lexeme is the $, so get the variable for which we need the address
			lexeme next_lexeme = this->next();
			// the next lexeme MUST be an identifier
			if (std::get<0>(next_lexeme) == "ident") {

				// turn the identifier into an LValue
				LValue target_var(std::get<1>(next_lexeme), "var_address");
				
				// get the address of the vector position of the variable
				return std::make_shared<AddressOf>(target_var);
			}
			// if it's not, throw an exception
			else {
				throw ParserException("An address-of operator must be followed by an identifier; illegal to follow with '" + std::get<1>(next_lexeme) + "' (not an identifier)", 111);
			}

		}
		// check to see if we have a pointer dereference operator
		else if (lex_val == "*") {
			// if we have an asterisk, it could be a pointer dereference OR a part of a binary expression
			// in order to check, we have to make sure that the previous character is neither a literal nor an identifier
			// the current lexeme is the asterisk, so get the previous lexeme
			lexeme previous_lex = this->previous();
			// note that previous() does not update the current position

			// get the previous lexeme's type
			std::string previous_type = std::get<0>(previous_lex);
			// if it is an int, float, string, or bool literal; or an identifier, then continue
			if (previous_type == "int" || previous_type == "float" || previous_type == "string" || previous_type == "bool" || previous_type == "ident") {
				// do nothing
			}
			// otherwise, check to make see if the next character is an identifier
			else if (std::get<0>(this->peek()) == "ident") {
				// get the identifier and advance the position counter
				lexeme next_lexeme = this->next();

				// turn the pointer into an LValue
				LValue _ptr(std::get<1>(next_lexeme), "var_dereferenced");

				// return a shared_ptr to the Dereferenced object containing _ptr
				return std::make_shared<Dereferenced>(std::make_shared<LValue>(_ptr));
			}
			// the next character CAN be an asterisk; in that case, we have a double or triple ref pointer that we need to parse
			else if (std::get<1>(this->peek()) == "*") {
				// advance the position pointer
				this->next();
				// dereference the pointer to get the address so we can dereference the other pointer
				std::shared_ptr<Expression> deref = this->parseExpression();
				if (deref->getExpType() == "dereferenced") {
					//// get the Dereferenced obj
					//Dereferenced* _deref = dynamic_cast<Dereferenced*>(deref.get());
					//// next, get the name of that variable
					//LValue _ptr(_deref->get_ptr().getValue(), "var_dereferenced");
					//return std::make_shared<Dereferenced>(_ptr);
					return std::make_shared<Dereferenced>(deref);
				}
			}
			// if it is not a literal or an ident and the next character is also not an ident or asterisk, we have an error
			else {
				throw ParserException("Expected an identifier in pointer dereference operation", 332);
			}
		}
	}

	// Use the maybe_binary function to determine whether we need to return a binary expression or a simple expression

	// always start it at 0; the first time it is called, it will be 0, as nothing will have been passed to parse_expression, but will be updated to the appropriate precedence level each time after. This results in a binary tree that shows the proper order of operations
	return this->maybeBinary(left, prec);
}

std::shared_ptr<Expression> Parser::maybeBinary(std::shared_ptr<Expression> left, int my_prec) {

	// Determines whether to wrap the expression in a binary or return as is

	lexeme next = this->peek();

	// if the next character is a semicolon, another end paren, or a comma, return
	if (std::get<1>(next) == ";" || std::get<1>(next) == ")" || std::get<1>(next) == ",") {
		return left;
	}
	// Otherwise, if we have an op_char...
	else if (std::get<0>(next) == "op_char") {

		// get the next op_char's data
		int his_prec = getPrecedence(std::get<1>(next));

		// If the next operator is of a higher precedence than ours, we may need to parse a second binary expression first
		if (his_prec > my_prec) {
			this->next();	// go to the next character in our stream (the op_char)
			this->next();
			// Parse out the next expression
			std::shared_ptr<Expression> right = this->maybeBinary(this->parseExpression(his_prec), his_prec);	// make sure his_prec gets passed into parse_expression so that it is actually passed into maybe_binary

			// Create the binary expression
			std::shared_ptr<Binary> binary = std::make_shared<Binary>(left, right, translate_operator(std::get<1>(next)));

			// call maybe_binary again at the old prec level in case this expression is followed by one of a higher precedence
			return this->maybeBinary(binary, my_prec);
		}
		else {
			return left;
		}

	}
	// There shouldn't be anything besides a semicolon, closing paren, or an op_char immediately following "left"
	else {
		//this->error("Invalid character in expression", 312);
		throw ParserException("Invalid character in expression", 312);
	}
}



// Populate our tokens list
void Parser::populateTokenList(std::ifstream* token_stream) {
	token_stream->peek();	// to make sure we haven't gone beyond the end of the file

	while (!token_stream->eof()) {
		lexeme current_token;
		std::string type;
		std::string value;

		if (token_stream->peek() != '\n') {
			*token_stream >> type;
			if (token_stream->peek() == '\n') {
				token_stream->get();
			}
			std::getline(*token_stream, value);
		}
		else {
			token_stream->get();
		}

		current_token = std::make_tuple(type, value);

		if (std::get<0>(current_token) == "") {
			continue;
		}
		else {
			this->tokens.push_back(current_token);
		}
	}
}


Parser::Parser(Lexer& lexer) {
	while (!lexer.eof() && !lexer.exit_flag_is_set()) {
		std::tuple<std::string, std::string> token = lexer.read_next();
		Parser::tokens.push_back(token);
	}
	Parser::quit = false;
	Parser::position = 0;
	Parser::num_tokens = Parser::tokens.size();
}

Parser::Parser(std::ifstream* token_stream) {
	Parser::quit = false;
	Parser::position = 0;
	Parser::populateTokenList(token_stream);
	Parser::num_tokens = Parser::tokens.size();
}

Parser::Parser()
{
}


Parser::~Parser()
{
}



// Define our exceptions

const char* ParserException::what() const {
	return ParserException::message_.c_str();
}

int ParserException::get_code() {
	return ParserException::code_;
}

ParserException::ParserException(const std::string& err_message, const int& err_code) : message_(err_message), code_(err_code) {

}
