#include "Parser.h"



// Define our symbols and their precedences as a vector of tuples
const std::vector<std::tuple<std::string, int>> Parser::precedence{ std::make_tuple("or", 2), std::make_tuple("and", 2), std::make_tuple("!", 2), \
	std::make_tuple("<", 4), std::make_tuple(">", 7), std::make_tuple("<", 7), std::make_tuple(">=", 7), std::make_tuple("<=", 7), std::make_tuple("=", 7),\
	std::make_tuple("!=", 7), std::make_tuple("|", 8), std::make_tuple("^", 8), std::make_tuple("&", 9), std::make_tuple("+", 10), std::make_tuple("-", 10), std::make_tuple("$", 15), std::make_tuple("*", 20), std::make_tuple("/", 20), std::make_tuple("%", 20) };

// Iterate through the vector and find the tuple that matches our symbol; if found, return its precedence; if not, return -1
const int Parser::get_precedence(std::string symbol) {
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
	if (this->position >= this->num_tokens - 2 || this->num_tokens == 0) {	// the last element is list.size() - 1;  if we are at list.size(), we have gone over
		return true;
	}
	else {
		return false;
	}
}

// peek to the next position
lexeme Parser::peek() {
	if (position + 1 < this->tokens.size()) {
		return this->tokens[this->position + 1];
	}
	else {
		throw ParserException("No more lexemes to parse!", 1, this->tokens[this->position].line_number);
	}
}

lexeme Parser::next() {
	//	increment the position
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

// Skip a punctuation mark
void Parser::skipPunc(char punc) {
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
	else {
		throw ParserException("Invalid grouping symbol in expression!", 0);
		return 0;
	}
}

bool Parser::is_opening_grouping_symbol(std::string to_test)
{
	return (to_test == "(" || to_test == "[");
}


/*

Create an abstract syntax tree using Parser::tokens. This is used not only as the entry function to the parser, but whenever an AST is needed as part of a statement. For example, a "Definition" statement requires an AST as one of its members; parse_top() is used to genereate the function's procedure's AST.

*/

StatementBlock Parser::create_ast() {
	// allocate a StatementBlock, which will be used to store our AST
	StatementBlock prog = StatementBlock();

	// creating an empty lexeme will allow us to test if the current token has nothing in it
	// sometimes, the lexer will produce a null lexeme, so we want to skip over it if we find one
	lexeme null_lexeme("", "", NULL);

	// Parse a token file
	// While we are within the program and we have not reached the end of a procedure block, keep parsing
	while (!this->is_at_end() && !this->quit && (this->peek().value != "}") && (this->current_token().value != "}")) {
		// skip any semicolons and newline characters, if there are any in the tokens list
		this->skipPunc(';');
		this->skipPunc('\n');

		// if we encounter a null lexeme, skip it
		while (this->current_token() == null_lexeme) {
			this->next();
		}

		// Parse a statement and add it to our AST
		prog.statements_list.push_back(this->parse_statement());

		// check to see if we are at the end now that we have advanced through the tokens list; if not, continue; if so, do nothing and the while loop will abort and return the AST we have produced
		if (!this->is_at_end() && !(this->peek().value == "}")) {
			this->next();
		}
	}

	// return the AST
	return prog;
}



/*

PARSE STATEMENTS

*/

std::shared_ptr<Statement> Parser::parse_statement() {
	// get our current lexeme and its information so we don't need to call these functions every time we need to reference it
	lexeme current_lex = this->current_token();

	// create a shared_ptr to the statement we are going to parse so that we can return it when we are done
	std::shared_ptr<Statement> stmt;
	// set the statement's line number

	// first, we will check to see if we need any keyword parsing
	if (current_lex.type == "kwd") {

		// Check to see what the keyword is

		// parse an "include" directive
		if (current_lex.value == "include") {
			return this->parse_include(current_lex);
		}
		else {
			// as soon as we hit a statement that is not an include statement, we are no longer allowed to use them (they must go at the top of the file)
			can_use_include_statement = false;
		}

		// parse inline assembly
		if (current_lex.value == "asm") {
			lexeme next = this->next();

			if (next.value == "<") {
				lexeme asm_type = this->next();
				if (asm_type.type == "ident") {
					std::string asm_architecture = asm_type.value;
					
					if (this->peek().value == ">") {
						this->next();
					}
					else {
						throw ParserException("Need closing angle bracket around asm type", 000, current_lex.line_number);
					}

					if (this->peek().value == "{") {
						this->next();

						// TODO: figure out how to implement line breaks in asm

						// continue reading values into a stringstream until we hit another curly brace
						bool end_asm = false;
						std::stringstream asm_code;

						lexeme asm_data = this->next();
						int current_line = asm_data.line_number;
						
						while (!end_asm) {
							// if we have advanced in line number, add a newline 
							if (asm_data.line_number > current_line) {
								asm_code << std::endl;
								current_line = asm_data.line_number;	// update 'current_line' -- if we advance lines from here, we will add a newline at the top of the next loop
							}

							if (asm_data.value == "}") {
								end_asm = true;
							}
							else {
								asm_code << asm_data.value;
									
								// put a space after idents, but not if a colon follows; also put spaces before semicolons
								if (((asm_data.type == "ident") && (this->peek().value != ":")) || (this->peek().value == ";")) {
									asm_code << " ";
								}
								// advance to the next token, but ONLY if we haven't hit the closing curly brace
								asm_data = this->next();
							}
						}

						stmt = std::make_shared<InlineAssembly>(asm_architecture, asm_code.str());
						stmt->set_line_number(current_lex.line_number);	// sets the line number for errors to the ASM block start; any ASM errors will be made known in the assembler
						return stmt;
					}
				}
				else {
					throw ParserException("Inline Assembly must include the target architecture!", 000, current_lex.line_number);
				}
			}
		}
		// parse a "free" statement
		else if (current_lex.value == "free") {
			// TODO: parse a "free" statement
			// the next character must be a paren

			if (this->peek().type == "ident") {
				current_lex = this->next();
					
				// make sure we end the statement correctly
				if (this->peek().value == ";") {
					this->next();

					LValue to_free(current_lex.value, "var");
					stmt = std::make_shared<FreeMemory>(to_free);
					return stmt;
				}
				else {
					throw ParserException("Syntax error: expected ';'", 0, current_lex.line_number);
				}
			}
			else {
				throw ParserException("Expected identifier after 'free'", 0, current_lex.line_number);
			}
		}
		// parse an ITE
		else if (current_lex.value == "if") {
			return this->parse_ite(current_lex);
		}
		// pare an allocation
		else if (current_lex.value == "alloc") {
			return this->parse_allocation(current_lex);
		}
		// Parse an assignment
		else if (current_lex.value == "let") {
			return this->parse_assignment(current_lex);
		}
		// Parse a return statement
		else if (current_lex.value == "return") {
			return this->parse_return(current_lex);
		}
		// Parse a 'while' loop
		else if (current_lex.value == "while") {
			return this->parse_while(current_lex);
		}
		// Parse a function declaration
		else if (current_lex.value == "def") {
			return this->parse_definition(current_lex);
		}
		else if (current_lex.value == "pass") {
			this->next();
			return std::make_shared<Statement>(STATEMENT_GENERAL, current_lex.line_number);	// an explicit pass will, essentially, be ignored by the compiler; it does nothing
		}
		// if none of the keywords were valid, throw an error
		else {
			throw ParserException("Invalid keyword", 211, current_lex.line_number);
		}

	}

	// if it's not a keyword, check to see if we need to parse a function call
	else if (current_lex.type == "op_char") {	// "@" is an op_char, but we may update the lexer to make it a "control_char"
		if (current_lex.value == "@") {
			return this->parse_function_call(current_lex);
		}
	}

	// if it is a curly brace, advance the character
	else if (current_lex.value == "}") {
		this->next();
	}

	// otherwise, if the lexeme is not a valid beginning to a statement, abort
	else {
		throw ParserException("Lexeme '" + current_lex.value + "' is not a valid beginning to a statement", 000, current_lex.line_number);
	}
}

std::shared_ptr<Statement> Parser::parse_include(lexeme current_lex)
{
	std::shared_ptr<Statement> stmt;

	if (this->can_use_include_statement) {

		lexeme next = this->next();

		if (next.type == "string") {
			std::string filename = next.value;

			stmt = std::make_shared<Include>(filename);
			stmt->set_line_number(current_lex.line_number);
			return stmt;
		}
		else {
			throw ParserException("Expected a filename in quotes in 'include' statement", 0, current_lex.line_number);
			// TODO: error numbers for includes
		}
	}
	else {
		throw ParserException("Include statements must come at the top of the file.", 0, current_lex.line_number);
	}
}

std::shared_ptr<Statement> Parser::parse_ite(lexeme current_lex)
{
	std::shared_ptr<Statement> stmt;

	// Get the next lexeme
	lexeme next = this->next();

	// Check to see if condition is enclosed in parens
	if (next.value == "(") {
		// get the condition
		std::shared_ptr<Expression> condition = this->parse_expression();
		// Initialize the if_block
		StatementBlock if_branch;
		StatementBlock else_branch;
		// Check to make sure a curly brace follows the condition
		next = this->peek();
		if (next.value == "{") {
			this->next();
			this->next();	// skip ahead to the first character of the statementblock
			if_branch = this->create_ast();

			// if we have an empty 'if' clause, we will be on the curly brace; only skip the curly brace if the branch isn't empty
			if (if_branch.statements_list.size() > 0) {
				// skip the closing curly brace
				this->next();
			}
			else {
				compiler_warning("Empty statement block in if condition", this->current_token().line_number);
			}

			// Check for an else clause
			if (!this->is_at_end() && this->peek().value == "else") {
				// if we have an else clause
				this->next();
				next = this->peek();
				// Again, check for curlies
				if (next.value == "{") {
					this->next();
					this->next();	// skip ahead to the first character of the statementblock
					else_branch = this->create_ast();

					// if the 'else' clause is empty, the current token will be the curly brace, so we don't need to eat it if the statement list wasn't empty
					if (else_branch.statements_list.size() > 0) {
						this->next();	// skip the closing curly brace
					}
					else {
						compiler_warning("Empty statement block in else condition", this->current_token().line_number);
					}

					stmt = std::make_shared<IfThenElse>(condition, std::make_shared<StatementBlock>(if_branch), std::make_shared<StatementBlock>(else_branch));
					stmt->set_line_number(current_lex.line_number);
					return stmt;
				}
				else {
					throw ParserException("Expected '{' after 'else' in conditional", 331, current_lex.line_number);
				}
			}
			else {
				// if we do not have an else clause, we will return the if clause alone here
				stmt = std::make_shared<IfThenElse>(condition, std::make_shared<StatementBlock>(if_branch));
				stmt->set_line_number(current_lex.line_number);
				return stmt;
			}
		}
		// If our condition is not followed by an opening curly
		else {
			throw ParserException("Expected '{' after condition in conditional", 331, current_lex.line_number);
		}
	}
	// If condition is not enclosed in parens
	else {
		throw ParserException("Condition must be enclosed in parens", 331, current_lex.line_number);
	}
}

std::shared_ptr<Statement> Parser::parse_allocation(lexeme current_lex)
{
	std::shared_ptr<Statement> stmt;

	// Create objects for our variable's type and name
	Type new_var_type;
	Type new_var_subtype = NONE;	// if we have a type that requires a subtype, this will be modified

	std::string new_var_name = "";

	// check our next token; it must be a keyword
	lexeme var_type = this->next();
	if (var_type.type == "kwd") {

		// check our quality, if any
		std::vector<SymbolQuality> qualities;
		if (var_type.value == "const") {
			// change the variable quality
			qualities.push_back(CONSTANT);

			// get the actual variable type
			var_type = this->next();
		}
		else if (var_type.value == "dynamic") {
			qualities.push_back(DYNAMIC);
			var_type = this->next();
		}
		else if (var_type.value == "static") {
			qualities.push_back(STATIC);
			var_type = this->next();
		}

		// check to see if we have a sign quality; this always comes immediately before the type
		if (var_type.value == "unsigned") {
			// make sure the next value is 'int'
			if (this->peek().value != "int") {
				throw ParserException("Cannot use sign qualifier for variable of this type", 0, var_type.line_number);
			}
			else {
				qualities.push_back(UNSIGNED);
				var_type = this->next();
			}
		}
		else if (var_type.value == "signed") {
			// make sure the next value is 'int'
			if (this->peek().value != "int") {
				throw ParserException("Cannot use sign qualifier for variable of this type", 0, var_type.line_number);
			}
			else {
				qualities.push_back(SIGNED);
				var_type = this->next();
			}
		}

		if (var_type.value == "int" || var_type.value == "float" || var_type.value == "bool" || var_type.value == "string" || var_type.value == "raw" || var_type.value == "ptr") {
			// Note: pointers, consts, and RAWs must be treated a little bit differently than other fundamental types

			// set the quality to DYNAMIC if we have a string
			if (var_type.value == "string") {
				qualities.push_back(DYNAMIC);
			}

			bool initialized = false;
			std::shared_ptr<Expression> initial_value = std::make_shared<Expression>();	// empty expression by default
			
			if (var_type.value == "ptr") {
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
			// otherwise, if it is not a pointer,
			else {
				// if we have an int, but we haven't pushed back signed/unsigned, default to signed
				if (var_type.value == "int") {
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
				new_var_type = get_type_from_string(var_type.value); // note: Type get_type_from_string() is found in "Expression" (.h and .cpp)
			}

			// next, get the variable's name
			// the following token must be an identifier
			lexeme var_name = this->next();
			if (var_name.type == "ident") {
				new_var_name = var_name.value;

				// we must check to see if we have the allloc-assign syntax:
				if (this->peek().value == ":") {
					// the variable was initialized
					initialized = true;

					// assign the initial value
					this->next();

					// move ahead to the first lexeme of the expression and parse the expression
					this->next();
					initial_value = this->parse_expression();

					// return the allocation
					stmt = std::make_shared<Allocation>(new_var_type, new_var_name, new_var_subtype, initialized, initial_value, qualities);
					stmt->set_line_number(current_lex.line_number);
				}
				// we can only ever have a semicolon, comma, or end paren at the end of an allocation statement
				else if (this->peek().value == ";" || this->peek().value == "," || this->peek().value == ")") {
					// if it is NOT allloc-assign syntax, we have to make sure the variable is not const before allocating it -- all const variables MUST be defined in the allocation
					bool is_constant = false;
					std::vector<SymbolQuality>::iterator quality_iter = qualities.begin();
					while (quality_iter != qualities.end() && !is_constant) {
						if (*quality_iter == CONSTANT) {
							is_constant = true;
						}
						else {
							quality_iter++;
						}
					}

					if (is_constant) {
						throw ParserException("Const variables must use allloc-assign syntax (e.g., 'alloc const int a: 5').", 000, current_lex.line_number);
					}
					else {
						// Otherwise, if it is not const, return our new variable
						Allocation allocation_statement(new_var_type, new_var_name, new_var_subtype);
						allocation_statement.set_symbol_qualities(qualities);
						allocation_statement.set_line_number(current_lex.line_number);

						stmt = std::make_shared<Allocation>(allocation_statement);
					}
				}
				else {
					throw ParserException("Unrecognized token.", 0, current_lex.line_number);
				}
			}
			else {
				throw ParserException("Expected an identifier", 111, current_lex.line_number);
			}
		}
		else if (var_type.value == "array") {
			// TODO: parse array allocation
			// set the variable type
			new_var_type = ARRAY;
			// check to make sure we have the size and type in angle brackets
			if (this->peek().value == "<") {
				this->next();	// eat the angle bracket

				// we must see the size next; must be an int
				if (this->peek().type == "int") {
					size_t array_length = (size_t)std::stoi(this->next().value);	// get the array length

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

							if (this->peek().type == "ident") {
								new_var_name = this->next().value;	// get the name
							}
							else {
								throw ParserException("Invalid name for a struct", 0, this->peek().line_number);
							}

							bool initialized = false;
							std::shared_ptr<Expression> initial_value;
							// now, check to see if we have alloc-assign syntax
							if (this->peek().value == ":") {
								initialized = true;
							}
							else {
								initial_value = std::make_shared<Expression>();
							}

							// check to make sure the next character is a semicolon
							if (this->peek().value != ";") {
								throw ParserException("Syntax error; expected ';'", 0, current_lex.line_number);
							}
							else {
								// finally, create the allocation statement
								stmt = std::make_shared<Allocation>(new_var_type, new_var_name, new_var_subtype, initialized, initial_value, qualities, array_length);
								stmt->set_line_number(current_lex.line_number);
							}
						}
						else {
							throw ParserException("Syntax error; expected '>'", 0, this->peek().line_number);
						}
					}
					else {
						throw ParserException("Syntax error; expected ','", 0, this->peek().line_number);
					}
				}
				else {
					throw ParserException("Invalid syntax; array allocation must specify <size, type>", 0, this->peek().line_number);
				}
			}
			else {
				throw ParserException("You must specify the size and type of array in its allocation", 0, var_type.line_number);
			}
		}
		else {
			throw ParserException("Expected a variable type; must be int, float, bool, or string", 211, current_lex.line_number);
		}
	}
	else {
		throw ParserException("Expected a variable type; token type must be a keyword", 111, current_lex.line_number);
	}

	return stmt;
}

std::shared_ptr<Statement> Parser::parse_assignment(lexeme current_lex)
{

	// Create a shared_ptr to our assignment expression
	std::shared_ptr<Assignment> assign;
	// Create an object for our left expression
	std::shared_ptr<Expression> lvalue;

	// if the next lexeme is an op_char, we have a pointer
	if (this->peek().type == "op_char") {
		// get the pointer operator
		lexeme ptr_op = this->next();
		// check to see if it is an address-of or dereference operator
		if (ptr_op.value == "$") {
			// set the LValue_Type to "var_address"
			// might not need to do this -- as it will be dealing with the pointer data itself, not the variable to which it points
		}
		else if (ptr_op.value == "*") {
			lvalue = this->create_dereference_object();
		}
		// if it isn't $ or *, it's an invalid op_char before an LValue
		else {
			throw ParserException("Operator character not allowed in an LValue", 211, current_lex.line_number);
		}
	}
	else {
		// get the next token, which should be the variable name
		lexeme _lvalue_lex = this->next();

		// ensure it's an identifier
		if (_lvalue_lex.type == "ident") {
			// the lvalue might be a dereferenced value; check to see if that's the case
			std::shared_ptr<Expression> index_number;
			if (this->peek().value == "[") {
				this->next();
				index_number = this->parse_expression(0, "[", true);	// set the not_binary flag to true
				lvalue = std::make_shared<Indexed>(_lvalue_lex.value, "var", index_number);
			}
			else {
				lvalue = std::make_shared<LValue>(_lvalue_lex.value);
			}
		}
		// if it isn't a valid LValue, then we can't continue
		else {
			throw ParserException("Expected an LValue", 111, current_lex.line_number);
		}
	}

	// now, "lvalue" should hold the proper variable reference for the assignment
	// get the operator character, make sure it's an equals sign
	lexeme _operator = this->next();
	if (_operator.value == "=") {
		// if the next lexeme is not a semicolon and the next lexeme's line number is the same as the current lexeme's line number, we are ok
		if ((this->peek().value != ";") && (this->peek().line_number == current_lex.line_number)) {
			// create a shared_ptr for our rvalue expression
			std::shared_ptr<Expression> rvalue;
			this->next();
			rvalue = this->parse_expression();

			assign = std::make_shared<Assignment>(lvalue, rvalue);
			assign->set_line_number(current_lex.line_number);
			return assign;
		}
		// otherwise, we have a syntax error -- we didn't get an expression where we expected it
		else {
			throw ParserException("Expected expression", 0, current_lex.line_number);
		}
	}
	else {
		throw ParserException("Unrecognized token.", 0, current_lex.line_number);
	}
}

std::shared_ptr<Statement> Parser::parse_return(lexeme current_lex)
{
	std::shared_ptr<Statement> stmt;
	this->next();	// go to the expression

	// if the current token is a semicolon, return a Literal Void
	if (this->current_token().value == ";" || this->current_token().value == "void") {
		// if we have "void", we need to skip ahead to the semicolon
		if (this->current_token().value == "void") {
			if (this->peek().value == ";") {
				this->next();
			}
			else {
				// we expect a semicolon after the return statement; throw an exception if there isn't one
				throw ParserException("Syntax error: expected ';'", 0, current_lex.line_number);
			}
		}

		// craft the statement
		stmt = std::make_shared<ReturnStatement>(std::make_shared<Literal>(VOID, "", NONE));
		stmt->set_line_number(current_lex.line_number);
	}
	// otherwise, we must have an expression
	else {
		// get the return expression
		std::shared_ptr<Expression> return_exp = this->parse_expression();

		// create a return statement from it and set the line number
		stmt = std::make_shared<ReturnStatement>(return_exp);
		stmt->set_line_number(current_lex.line_number);
	}

	// return the statement
	return stmt;
}

std::shared_ptr<Statement> Parser::parse_while(lexeme current_lex)
{
	std::shared_ptr<Statement> stmt;

	// A while loop is very similar to an ITE in how we parse it; the only difference is we don't need to check for an "else" branch
	std::shared_ptr<Expression> condition;	// create the object for our condition
	StatementBlock branch;	// and for the loop body

	if (this->peek().value == "(") {
		this->next();
		condition = this->parse_expression();
		if (this->peek().value == "{") {
			this->next();

			// so that we don't get errors if we have an empty statement block
			if (this->peek().value == "}") {
				compiler_warning("Empty statement block in while loop", this->current_token().line_number);
				this->next();
			}
			else {
				this->next();	// skip opening curly
				branch = this->create_ast();

				// If we are not at the end, go to the next token
				if (!(this->is_at_end())) {
					this->next();
				}
			}

			// Make a pointer to our branch
			std::shared_ptr<StatementBlock> loop_body = std::make_shared<StatementBlock>(branch);

			// create our object, set the line number, and return it
			stmt = std::make_shared<WhileLoop>(condition, loop_body);
			stmt->set_line_number(current_lex.line_number);
			return stmt;
		}
		else {
			throw ParserException("Loop body must be enclosed in curly braces", 331, current_lex.line_number);
		}
	}
	else {
		throw ParserException("Expected a condition", 331, current_lex.line_number);
	}
}

std::shared_ptr<Statement> Parser::parse_definition(lexeme current_lex)
{
	std::shared_ptr<Statement> stmt;

	// First, get the type of function that we have -- the return value
	lexeme func_type = this->next();
	Type return_type;

	// func_type must be a keyword
	if (func_type.type == "kwd") {
		// get the return type
		return_type = get_type_from_string(func_type.value);

		// Get the function name and verify it is of the correct type
		lexeme func_name = this->next();
		if (func_name.type == "ident") {
			lexeme _peek = this->peek();
			if (_peek.value == "(") {
				this->next();
				// Create our arguments vector and our StatementBlock variable
				StatementBlock procedure;
				std::vector<std::shared_ptr<Statement>> args;
				// Populate our arguments vector if there are arguments
				if (this->peek().value != ")") {
					this->next();
					while (this->current_token().value != ")") {
						args.push_back(this->parse_statement());
						this->next();

						// if we have multiple arguments, current_token() will return a comma, but we don't want to advance twice in case we hit the closing paren; as a result, we only advance once more if there is a comma
						if (this->current_token().value == ",") {
							this->next();
						}
					}
				}
				else {
					this->next();	// skip the closing paren
				}

				// Args should be empty if we don't have any
				// Now, check to make sure we have a curly brace
				if (this->peek().value == "{") {
					this->next();

					// if we have an empty definition, print a warning but continue parsing
					if (this->peek().value != "}") {
						this->next();	// if the definition isn't empty we can skip ahead, but we don't want to if it is (it will cause the parser to crash)
					}
					else {
						parser_warning("Empty function definition", this->current_token().line_number);	// print a warning and don't advance the token pointer
					}

					procedure = this->create_ast();
					this->next();	// skip closing curly brace

					// Return the pointer to our function
					std::shared_ptr<LValue> _func = std::make_shared<LValue>(func_name.value, "func");
					stmt = std::make_shared<Definition>(_func, return_type, args, std::make_shared<StatementBlock>(procedure));
					stmt->set_line_number(current_lex.line_number);

					return stmt;
				}
				else {
					throw ParserException("Function definition requires use of curly braces after arguments", 331, current_lex.line_number);
				}
			}
			else {
				throw ParserException("Function definition requires '(' and ')'", 331, current_lex.line_number);
			}
		}
		// if NOT "ident"
		else {
			throw ParserException("Expected identifier", 330, current_lex.line_number);
		}
	}
}

std::shared_ptr<Statement> Parser::parse_function_call(lexeme current_lex)
{
	std::shared_ptr<Statement> stmt;

	// Get the function's name
	lexeme func_name = this->next();
	if (func_name.type == "ident") {
		std::vector<std::shared_ptr<Expression>> args;
		this->next();
		this->next();
		while (this->current_token().value != ")") {
			args.push_back(this->parse_expression());
			this->next();
		}
		while (this->peek().value != ";") {
			this->next();
		}
		this->next();
		stmt = std::make_shared<Call>(std::make_shared<LValue>(func_name.value, "func"), args);
		stmt->set_line_number(current_lex.line_number);
		return stmt;
	}
	else {
		throw ParserException("Expected an identifier", 111, current_lex.line_number);
	}
}



/*

PARSE EXPRESSIONS

*/

std::shared_ptr<Expression> Parser::parse_expression(size_t prec, std::string grouping_symbol, bool not_binary) {
	lexeme current_lex = this->current_token();

	// Create a pointer to our first value
	std::shared_ptr<Expression> left;

	// Check if our expression begins with parens; if so, only return what is inside them
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
	else if (current_lex.value == ",") {
		this->next();
		return this->parse_expression();
	}
	// if it is not an expression within parens
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
			// expression must be enclosed in parens
			if (this->peek().value == "(") {
				grouping_symbol = "(";
				this->next();
				// valid words in sizeof are idents and types, so long as the type is not a struct or an array
				if (this->peek().type == "ident" || (is_type(this->peek().value) && this->peek().value != "struct" && this->peek().value != "array")) {
					lexeme to_check = this->next();

					if (this->peek().value == get_closing_grouping_symbol(grouping_symbol)) {
						this->next();	// eat the end paren
						left = std::make_shared<SizeOf>(to_check.value);
					}
					else {
						throw ParserException("Syntax error; expected ')'", 0, current_lex.line_number);
					}
				}
				else {
					throw ParserException("Invalid 'sizeof' argument", 0, current_lex.line_number);
				}
			}
			else {
				throw ParserException("Syntax error; expected '('", 0, current_lex.line_number);
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

		// get the next op_char's data
		int his_prec = get_precedence(next.value);

		// If the next operator is of a higher precedence than ours, we may need to parse a second binary expression first
		if (his_prec > my_prec) {
			this->next();	// go to the next character in our stream (the op_char)
			this->next();	// go to the character after the op char

			// Parse out the next expression
			std::shared_ptr<Expression> right = this->maybe_binary(this->parse_expression(his_prec), his_prec);	// make sure his_prec gets passed into parse_expression so that it is actually passed into maybe_binary

			// Create the binary expression
			std::shared_ptr<Binary> binary = std::make_shared<Binary>(left, right, translate_operator(next.value));	// "next" still contains the op_char; we haven't updated it yet

			// call maybe_binary again at the old prec level in case this expression is followed by one of a higher precedence
			return this->maybe_binary(binary, my_prec);
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



// Populate our tokens list
void Parser::populate_token_list(std::ifstream* token_stream) {
	token_stream->peek();	// to make sure we haven't gone beyond the end of the file

	while (!token_stream->eof()) {
		lexeme current_token;
		std::string type;
		std::string value;
		std::string line_number_string;
		int line_number = -1;	// initialize to -1

		// get the type
		if (token_stream->peek() != '\n') {
			*token_stream >> type;

			// get the value
			if (token_stream->peek() == '\n') {
				token_stream->get();
			}
			std::getline(*token_stream, value);

			// get the line number
			if (token_stream->peek() == '\n') {
				token_stream->get();
			}
			// convert the line number to an int
			std::getline(*token_stream, line_number_string);
			line_number = std::stoi(line_number_string);
		}
		else {
			token_stream->get();
		}

		// ensure that empty tokens are not added to the tokens list
		current_token = lexeme(type, value, line_number);
		if (current_token.type == "") {
			continue;
		}
		else {
			this->tokens.push_back(current_token);
		}
	}
}


Parser::Parser(Lexer& lexer) {
	while (!lexer.eof() && !lexer.exit_flag_is_set()) {
		lexeme token = lexer.read_next();

		// only push back tokens that aren't empty
		if ((token.type != "") && (token.value != "") && (token.line_number != 0)) {
			Parser::tokens.push_back(token);
		}
		else {
			continue;
		}
	}

	Parser::quit = false;
	Parser::can_use_include_statement = true;	// include statements must be first in the file
	Parser::position = 0;
	Parser::num_tokens = Parser::tokens.size();
}

Parser::Parser(std::ifstream* token_stream) {
	Parser::quit = false;
	Parser::position = 0;
	Parser::populate_token_list(token_stream);
	Parser::num_tokens = Parser::tokens.size();
	Parser::can_use_include_statement = false;	// initialize to false
}

Parser::Parser()
{
	// Default constructor will intialize (almost) everything to 0
	this->tokens = {};
	this->position = 0;
	this->num_tokens = 0;

	this->quit = false;
	this->can_use_include_statement = true;	// this will initialize to true because we haven't hit any other statement
}


Parser::~Parser()
{
}
