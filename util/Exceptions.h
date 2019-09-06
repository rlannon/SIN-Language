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
#include <iostream>
#include <bitset>

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

// sometimes, we want to print an error message, but we don't need to stop compilation
void compiler_warning(std::string message, unsigned int line = 0);


class ParserException : public std::exception
{
protected:
	std::string message_;
	unsigned int code_;
	unsigned int line_;
public:
	explicit ParserException(const std::string& message, const unsigned int& code, const unsigned int& line = 0);
	virtual const char* what() const noexcept;
};

class MissingSemicolonError : public ParserException
{
public:
	explicit MissingSemicolonError(const unsigned int& line = 0);
};

// like in the compiler, we sometimes want to print warnings without stopping parsing
void parser_warning(std::string message, unsigned int line_number = 0);


class VMException : public std::exception {
	std::string message;	// the message associated with the error
	uint16_t address;	// the address of the program counter when the error occurred
	uint16_t status;	// the STATUS register at the time of the error
public:
	explicit VMException(const std::string& message, const uint16_t& address = 0x0000, const uint16_t& status = 0x00);
	virtual const char* what() const noexcept;
};


class SymbolTableException : public std::exception {
	std::string message;
	unsigned int line;
public:
	explicit SymbolTableException(const std::string& message, const unsigned int& line = 0);
	virtual const char* what() const noexcept;
};


class AssemblerException : public std::exception {
	std::string message;
	unsigned int line;
public:
	explicit AssemblerException(const std::string& message, const unsigned int& line = 0);
	virtual const char* what() const noexcept;
};
