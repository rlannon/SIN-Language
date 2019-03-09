#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <string>
#include <tuple>

#include "Statement.h"
#include "Expression.h"
#include "Lexer.h"
#include "Exceptions.h"	// ParserException


class Parser
{
	std::vector<lexeme> tokens;
	size_t position;
	size_t num_tokens;

	// Sentinel variable
	bool quit;

	// 'include' directives can only come at the very beginning of the program; once any other statement comes, the include directives will throw errors
	bool can_use_include_statement;

	// our precedence handlers
	static const std::vector<std::tuple<std::string, int>> precedence;
	static const int get_precedence(std::string symbol);

	// Some utility functions
	bool is_at_end();	// tells us whether we have run out of tokens
	lexeme peek();	// get next token without moving the position
	lexeme next();	// get next token
	lexeme current_token();	// get token at current position
	lexeme previous();	// similar to peek; get previous token without moving back
	lexeme back();	// move backward one
	void skipPunc(char punc);	// skips the specified punctuation mark
	bool is_type(std::string lex_value);
	std::string get_closing_grouping_symbol(std::string beginning_symbol);
	bool is_opening_grouping_symbol(std::string to_test);

	// Parsing statements
	std::shared_ptr<Statement> parse_statement();	// entry function to parse a statement
	std::shared_ptr<Statement> parse_include(lexeme current_lex);
	std::shared_ptr<Statement> parse_ite(lexeme current_lex);
	std::shared_ptr<Statement> parse_allocation(lexeme current_lex);
	std::shared_ptr<Statement> parse_assignment(lexeme current_lex);
	std::shared_ptr<Statement> parse_return(lexeme current_lex);
	std::shared_ptr<Statement> parse_while(lexeme current_lex);
	std::shared_ptr<Statement> parse_definition(lexeme current_lex);
	std::shared_ptr<Statement> parse_function_call(lexeme current_lex);

	// Parseing expressions
	/*
	put default argument here because we call "parse_expression" in "maybe_binary"; as a reuslt, "his_prec" appears as if it is being passed to the next maybe_binary, but isn't because we parse an expression before we parse the binary, meaning my_prec gets set to 0, and not to his_prec as it should
	Note we also have a 'not_binary' flag here; if the expression is indexed, we may not want to have a binary expression parsed
	*/
	std::shared_ptr<Expression> parse_expression(size_t prec=0, std::string grouping_symbol = "(", bool not_binary = false);
	std::shared_ptr<Expression> create_dereference_object();
	LValue getDereferencedLValue(Dereferenced to_eval);
	std::shared_ptr<Expression> maybe_binary(std::shared_ptr<Expression> left, size_t my_prec, std::string grouping_symbol = "(");	// check to see if we need to fashion a binary expression

	// Create a list of tokens for the parser from an input stream
	void populate_token_list(std::ifstream* token_stream);
public:
	// our entry function
	StatementBlock create_ast();

	Parser(Lexer& lexer);
	Parser(std::ifstream* token_stream);
	Parser();
	~Parser();
};
