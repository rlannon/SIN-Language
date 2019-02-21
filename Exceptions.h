#pragma once

#include <exception>
#include <string>
#include <sstream>

/*

	Exceptions.h
	Copyright 2019 Riley Lannon

	The purpose of the Exceptions files is to implement the various custom exceptions we use in the program.

*/

class CompilerException : public std::exception
{
	std::string message;
	unsigned int code;
	unsigned int line;
public:
	explicit CompilerException(const std::string& message, unsigned int code = 0, unsigned int line = 0);
	virtual const char* what() const noexcept;

	CompilerException();
};


class ParserException : public std::exception {
	std::string message;
	unsigned int code;
	unsigned int line_number;
public:
	explicit ParserException(const std::string& err_message, const int& err_code, const int& line_number = 0);
	virtual const char* what() const noexcept;
	int get_code();
};


class VMException : public std::exception {
	std::string message;	// the message associated with the error
	uint16_t address;	// the address of the program counter when the error occurred
public:
	explicit VMException(const std::string& message, uint16_t address = 0x0000);
	virtual const char* what() const noexcept;
};
