/*

SIN Toolchain
Parser.cpp
Copyright 2019 Riley Lannon

The implementation of some general and utility functions for the Parser class.
Implementations of statement and expression parsing functions can be found in ParseExpression.cpp and ParseStatement.cpp

*/

#include "Parser.h"


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

		// Parse a statement
		std::shared_ptr<Statement> next = this->parse_statement();

		// check to see if it is a return statement; function definitions require them, but they are forbidden outside of them
		if (next->get_statement_type() == RETURN_STATEMENT) {
			prog.has_return = true;
		}

		// push the statement back
		prog.statements_list.push_back(next);

		// check to see if we are at the end now that we have advanced through the tokens list; if not, continue; if so, do nothing and the while loop will abort and return the AST we have produced
		if (!this->is_at_end() && !(this->peek().value == "}")) {
			this->next();
		}
	}

	// return the AST
	return prog;
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
