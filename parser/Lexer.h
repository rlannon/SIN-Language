/*

SIN Toolchain
Copyright 2019 Riley Lannon
Lexer.h

The Lexer class is used to handle the stream of input that we wish to parse. It takes a stream of input and returns tokens, each with a type and a value.
A lexeme may be returned from the stream using the read_next() function.

Note that the Lexer class does /not/ parse source files; it simply puts those files in a format that is usable by the language's parser, which is contained within the Parser class.

*/

#pragma once

#include <string>
#include <tuple>
#include <iostream>
#include <fstream>
#include <regex>
#include <functional>
#include <algorithm>
#include <vector>
#include <exception>


// Our lexeme data
struct lexeme {
	std::string type;
	std::string value;
	unsigned int line_number;
	
	// overload the == operator so we can compare two lexemes
	bool operator==(const lexeme& b);

	lexeme();
	lexeme(std::string type, std::string value, unsigned int line_number);
};

class Lexer
{
	std::istream* stream;
	int position;
	bool exit_flag;

	lexeme current_lexeme;
	unsigned int current_line;	// track what line we are on in the file

	static const std::vector<std::string> keywords;	// our keyword vector

	// Strings for our longer/more complex regular expressions; this will make it easier to edit them
	static const std::string punc_exp;
	static const std::string op_exp;
	static const std::string id_exp;
	static const std::string bool_exp;

	// character access functions
	char peek();
	char next();

	static bool match_character(char ch, std::string expression);	// match a single character with regex (including an exception handler)

	/*

	Character test functions
	The following functions test to see whether a character is of a particular type to help evaluate the type of the whole token.
	Tokens all have a type and value, and are a member of one of the following classes:
		float	-	a floating point decimal number
		int		-	an integer
		string	-	any string of characters that is not an identifier or a keyword
			must be enclosed in double quotes for it to be read as a string
		bool	-	a boolean type
			values can be True or False
		ident	-	identifier
			variable names, function names, etc.
		kwd		-	keyword
			let, alloc, if, etc.
		punc	-	punctuation
			. , : ; () {} []
		op		-	mathematical, logical, and other important operators
			+ * - / & | ^ % < > = ! @ ? $
		escape	-	escaped characters
			\n, \t, etc.

	*/

	static bool is_whitespace(char ch);	// tests if a character is \n, \t, or a space
										// DELETE?
	static bool is_newline(char ch);	// tests for a newline character
										//
	static bool is_not_newline(char ch);
	static bool is_digit(char ch);		// tests whether a character is a digit; used for the first digit in a number
	static bool is_letter(char ch);
	static bool is_number(char ch);		// includes a decimal point; for reading a number, not just a digit

	static bool is_id_start(char ch);	// determine starting character of an identifier (cannot start with a number)
	static bool is_id(char ch);			// determine if the current character is a valid id character (can contain numbers within)

	static bool is_punc(char ch);
	static bool is_op_char(char ch);

	static bool is_boolean(std::string candidate);

	static bool is_keyword(std::string candidate);	// test whether the string is a keyword (such as alloc or let) or an identifier (such as a variable name)

	std::string read_while(bool(*predicate)(char));

	void read_lexeme();

	std::string read_string();
	std::string read_ident();	// read the full identifier

public:
	bool eof();		// check to see if we are at the end of the file
	bool exit_flag_is_set();	// check to see the status of the exit flag

	std::ostream& write(std::ostream& os) const;	// allows a lexeme to be written to an ostream

	// read the next lexeme
	lexeme read_next();

	Lexer(std::istream& input);
	~Lexer();
};

// overload the << operator
std::ostream& operator<<(std::ostream& os, const Lexer& lexer);


class LexerException : public std::exception {
	std::string message_;
	int position_;
	char ch_;
public:
	explicit LexerException(const std::string& err_message, const int& err_position, const char& ch);
	virtual const char* what() const noexcept;
	char get_char();
	int get_pos();
};
