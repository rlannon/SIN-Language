#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <string>
#include <tuple>
#include <exception>

#include "Statement.h"
#include "Expression.h"
#include "Lexer.h"

class Parser
{
	// Our lexeme data
	typedef std::tuple<std::string, std::string> lexeme;

	std::vector<lexeme> tokens;
	int position;
	int num_tokens;

	// Sentinel variable
	bool quit;

	static const std::vector<std::tuple<std::string, int>> precedence;
	static const int getPrecedence(std::string symbol);

	// Some utility functions
	bool is_at_end();	// tells us whether we have run out of tokens
	lexeme peek();	// get next token without moving the position
	lexeme next();	// get next token
	lexeme current_token();	// get token at current position
	lexeme previous();	// similar to peek; get previous token without moving back
	lexeme back();	// move backward one
	void error(std::string message, int code);

	void skipPunc(char punc);	// skips the specified punctuation mark

	std::shared_ptr<Statement> parseStatement();
	std::shared_ptr<Expression> parseExpression(int prec=0);	// put default argument here because we call "parse_expression" in "maybe_binary"; as a reuslt, "his_prec" appears as if it is being passed to the next maybe_binary, but isn't because we parse an expression before we parse the binary, meaning my_prec gets set to 0, and not to his_prec as it should
	std::shared_ptr<Expression> createDereferenceObject();
	LValue getDereferencedLValue(Dereferenced to_eval);
	std::shared_ptr<Expression> maybeBinary(std::shared_ptr<Expression> left, int my_prec);

	void populateTokenList(std::ifstream* token_stream);
public:
	// our entry function
	StatementBlock createAST();

	Parser(Lexer& lexer);
	Parser(std::ifstream* token_stream);
	Parser();
	~Parser();
};


// Our general exception class
class ParserException : public std::exception {
	std::string message_;
	int code_;
public:
	explicit ParserException(const std::string& err_message, const int& err_code);
	virtual const char* what() const;
	int get_code();
};
