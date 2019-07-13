/*

SIN Toolchain
ParseStatement.cpp
Copyright 2019 Riley Lannon

The implementation of the Parser member functions to parse statements, including:
	- std::shared_ptr<Statement> parse_statement();	// entry function to parse a statement
	- std::shared_ptr<Statement> parse_include(lexeme current_lex);
	- std::shared_ptr<Statement> parse_declaration(lexeme current_lex);
	- std::shared_ptr<Statement> parse_ite(lexeme current_lex);
	- std::shared_ptr<Statement> parse_allocation(lexeme current_lex);
	- std::shared_ptr<Statement> parse_assignment(lexeme current_lex);
	- std::shared_ptr<Statement> parse_return(lexeme current_lex);
	- std::shared_ptr<Statement> parse_while(lexeme current_lex);
	- std::shared_ptr<Statement> parse_definition(lexeme current_lex);
	- std::shared_ptr<Statement> parse_function_call(lexeme current_lex);

Keeping the statement parsing functions together makes for a smaller 'Parser.cpp' file and more readable code.

*/

#include "Parser.h"

std::shared_ptr<Statement> Parser::parse_statement(bool is_function_parameter) {
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
					throw MissingSemicolonError(current_lex.line_number);
				}
			}
			else {
				throw ParserException("Expected identifier after 'free'", 0, current_lex.line_number);
			}
		}
		// parse a declaration
		else if (current_lex.value == "decl") {
			return this->parse_declaration(current_lex, is_function_parameter);
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

std::shared_ptr<Statement> Parser::parse_declaration(lexeme current_lex, bool is_function_parameter) {
	/*

	Parse a declaration statement. Appropriate syntax is:
		decl <type> <name>;
	or
		decl <type> <name>(<formal parameters>);
	where the formal parameters are also declarations

	*/

	lexeme next_lexeme = this->peek();

	// the next lexeme must be a keyword (specifically, a type)
	if (next_lexeme.type == "kwd") {
		next_lexeme = this->next();
		TypeData symbol_type_data = this->get_type();
		std::shared_ptr<Expression> initial_value = std::make_shared<Expression>(EXPRESSION_GENERAL);

		// get the variable name
		next_lexeme = this->next();
		if (next_lexeme.type == "ident") {		// variable names must be identifiers; if an identifier doesn't follow the type, we have an error
			// get our variable name
			std::string var_name = next_lexeme.value;
			bool is_function = false;
			// todo: struct declarations...

			std::vector<std::shared_ptr<Statement>> formal_parameters = {};

			// next, check to see if we have a paren following the name; if so, it's a function, so we need to get the formal parameters
			if (this->peek().value == "(") {
				// if there is a paren, it's a function declaration
				is_function = true;

				// todo: parse function declarations
				this->next();	// eat the opening paren

				// so long as we haven't hit the end of the formal parameters, continue parsing
				while (this->peek().value != ")") {
					this->next();
					std::shared_ptr<Statement> next = this->parse_statement(true);

					// the statement _must_ be a declaration, not an allocation
					if (next->get_statement_type() == DECLARATION) {
						formal_parameters.push_back(next);
					}
					else {
						throw ParserException("Definitions of formal parameters in a declaration of a function must use 'decl' (not 'alloc'", 0,
							this->current_token().line_number);
					}

					if (this->peek().value == ",") {
						this->next();
					}
				}

				this->next();	// eat the closing paren
			}
			// otherwise, if the name is followed by a colon, we have a default value
			else if (this->peek().value == ":") {
				// however, we may only use alloc-assign syntax if the 'decl' is part of a function parameter
				if (is_function_parameter) {
					this->next();
					this->next();	// parse_expression(...) begins on the _first token_ of the expression
					initial_value = this->parse_expression();
				}
				else {
					throw CompilerException("Cannot use alloc-assign syntax in declarations unless said declaration is a default function parameter",
						this->current_token().line_number);
				}
			}

			// check to see if we have postfixed symbol qualities using the & operator
			if (this->peek().value == "&") {
				// append the posftixed qualities to symbol_type_data.qualities
				this->next();
				std::vector<SymbolQuality> postfixed_qualities = this->get_postfix_qualities();

				// todo: ensure there are no duplicates and no collisions in symbol qualities

				symbol_type_data.qualities.insert(symbol_type_data.qualities.end(), postfixed_qualities.begin(), postfixed_qualities.end());
			}
			
			// finally, we must have a semicolon, a comma, or a closing paren
			if (this->peek().value == ";" || this->peek().value == "," || this->peek().value == ")") {
				Declaration decl_statement(symbol_type_data.data_type, var_name, symbol_type_data.subtype, symbol_type_data.array_length,
					symbol_type_data.qualities, initial_value, is_function, false, formal_parameters);
				decl_statement.set_line_number(next_lexeme.line_number);

				return std::make_shared<Declaration>(decl_statement);
			}
			else if (this->peek().value == ":") {
				throw ParserException("Initializations are forbidden in declaration statements", 0, next_lexeme.line_number);
			}
			else {
				throw MissingSemicolonError(next_lexeme.line_number);
			}
		}
		else {
			throw ParserException("Expected variable name after type in Declaration", 0, next_lexeme.line_number);
		}
	}
	// if it is not followed by a type name, throw an exception
	else {
		throw ParserException("Expected type name following 'decl' in variable declaration", 0, current_lex.line_number);
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
	// create an object for the statement as well as the variable's name
	std::shared_ptr<Statement> stmt;
	std::string new_var_name = "";

	// check our next token; it must be a keyword
	lexeme next_token = this->next();
	if (next_token.type == "kwd") {
		// get the type data using Parser::get_type()
		TypeData symbol_type_data = this->get_type();

		// next, get the name
		if (this->peek().type == "ident") {
			next_token = this->next();
			new_var_name = next_token.value;
			bool initialized = false;
			std::shared_ptr<Expression> initial_value = std::make_shared<Expression>();

			// the name can be followed by a semicolon, a comma, a closing paren, or a colon
			// if it's a colon, we have an initial value
			if (this->peek().value == ":") {
				this->next();
				this->next();	// advance the iterator so it points to the first character of the expression
				initialized = true;
				initial_value = this->parse_expression();
			}

			// check to see if we have any postfixed qualities
			if (this->peek().value == "&") {
				// append the posftixed qualities to symbol_type_data.qualities
				this->next();
				std::vector<SymbolQuality> postfixed_qualities = this->get_postfix_qualities();
				
				// todo: ensure there are no duplicates and no collisions in symbol qualities

				symbol_type_data.qualities.insert(symbol_type_data.qualities.end(), postfixed_qualities.begin(), postfixed_qualities.end());
			}

			// if it's a semicolon, comma, or closing paren, craft the statement and return
			if (this->peek().value == ";" || this->peek().value == "," || this->peek().value == ")") {
				// craft the statement
				stmt = std::make_shared<Allocation>(symbol_type_data.data_type, new_var_name, symbol_type_data.subtype, initialized, initial_value,
					symbol_type_data.qualities, symbol_type_data.array_length);
				stmt->set_line_number(next_token.line_number);	// set the line number
			}
			// otherwise, it's an invalid character
			else {
				throw ParserException("Syntax error; invalid character following allocation", 0, next_token.line_number);
			}
		}
		else {
			throw ParserException("The variable's type must be followed by a valid identifier", 0, next_token.line_number);
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
				throw MissingSemicolonError(current_lex.line_number);
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
	this->next();	// skip the 'def' keyword; 'get_type' begins parsing the function type _on the first token of the type data_
	TypeData func_type_data = this->get_type();

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
				stmt = std::make_shared<Definition>(_func, func_type_data.data_type, func_type_data.subtype, func_type_data.qualities, args, std::make_shared<StatementBlock>(procedure));
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
