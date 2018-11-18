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

class Parser
{
	typedef std::tuple<std::string, std::string> lexeme;

	std::vector<lexeme> tokens;
	int position;
	int num_tokens;

	bool quit;
public:
	static const std::vector<std::tuple<std::string, int>> precedence;
	static const int get_precedence(std::string symbol);

	// Some utility functions
	bool is_at_end();	// tells us whether we have run out of tokens
	lexeme peek();	// get next token without moving the position
	lexeme next();	// get next token
	lexeme current_token();	// get token at current position
	lexeme previous();	// similar to peek; get previous token without moving back
	lexeme back();	// move backward one
	void error(std::string message, int code);

	void skip_punc(char punc);	// skips the specified punctuation mark

	StatementBlock parse_top();

	std::shared_ptr<Statement> parse_atomic();
	std::shared_ptr<Expression> parse_expression(int prec=0);	// put default argument here because we call "parse_expression" in "maybe_binary"; as a reuslt, "his_prec" appears as if it is being passed to the next maybe_binary, but isn't because we parse an expression before we parse the binary, meaning my_prec gets set to 0, and not to his_prec as it should
	std::shared_ptr<Expression> maybe_binary(std::shared_ptr<Expression> left, int my_prec);

	void populate_tokens_list(std::ifstream* token_stream);

	Parser(Lexer& lexer);
	Parser(std::ifstream* token_stream);
	Parser();
	~Parser();
};
