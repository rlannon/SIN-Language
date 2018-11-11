#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <string>
#include <tuple>

#include "Statement.h"
#include "Expression.h"

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
	std::shared_ptr<Expression> parse_expression();
	std::shared_ptr<Expression> maybe_binary(std::shared_ptr<Expression> left, int my_prec);

	void populate_tokens_list(std::ifstream* token_stream);

	Parser(std::ifstream* token_stream);
	Parser();
	~Parser();
};
