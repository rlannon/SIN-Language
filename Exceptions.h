/*

SIN Toolchain
Exceptions.h
Copyright 2019 Riley Lannon

A class to contain all of our custom exceptions.

*/


#pragma once

#include <stdexcept>
#include <string>
#include <sstream>
#include <cinttypes>	// use this instead of iostream for uint_t types

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
};


class ParserException : public std::exception
{
	std::string message_;
	unsigned int code_;
	unsigned int line_;
public:
	explicit ParserException(const std::string& message, const unsigned int& code, const unsigned int& line = 0);
	virtual const char* what() const noexcept;
};


class VMException : public std::exception {
	std::string message;	// the message associated with the error
	uint16_t address;	// the address of the program counter when the error occurred
public:
	explicit VMException(const std::string& message, const uint16_t& address = 0x0000);
	virtual const char* what() const noexcept;
};
