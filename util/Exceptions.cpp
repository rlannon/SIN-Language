/*

SIN Toolchain
Exceptions.cpp
Copyright 2019 Riley Lannon

The implementation of the classes/methods defined in Exceptions.h

Note that all what() messages MUST be constructed in the constructor for the exception (unless we have a const char that does not rely on any variable data).

*/

#include "Exceptions.h"


// Compiler Exceptions

const char* CompilerException::what() const noexcept {
	return message.c_str();
}

CompilerException::CompilerException(const std::string& message, unsigned int code, unsigned int line) : message(message), code(code), line(line) {
	this->message = "**** Compiler Error " + std::to_string(this->code) + ": " + this->message + " (line " + std::to_string(this->line) + ")";
}


// Compiler Warning

void compiler_warning(std::string message, unsigned int line_number) {
	std::cout << "**** Compiler Warning: " << message << " (line " << line_number << ")" << std::endl;
}

void parser_warning(std::string message, unsigned int line_number)
{
	std::cout << "**** Parser Warning: " << message << " (line " << line_number << ")" << std::endl;
}



// Parser Exceptions

const char* ParserException::what() const noexcept {
	return message_.c_str();
}

ParserException::ParserException(const std::string& message, const unsigned int& code, const unsigned int& line) : message_(message), code_(code), line_(line) {
	message_ = "**** Parser Error " + std::to_string(code_) + ": " + message_ + " (line " + std::to_string(line_) + ")";
}



// SINVM Exceptions

const char* VMException::what() const noexcept {
	return message.c_str();
}

VMException::VMException(const std::string& message, const uint16_t& address, const uint16_t& status) : message(message), address(address), status(status) {
	// we must construct the message here, in the constructor
	std::stringstream err_ss;
	err_ss << "**** SINVM Error: " << this->message << std::endl << "Error was encountered at memory location " << std::hex << this->address << std::endl << "STATUS register was " << std::hex << this->status << std::dec << std::endl;
	this->message = err_ss.str();
}


// SymbolTable Exceptions

const char* SymbolTableException::what() const noexcept {
	return message.c_str();
}

SymbolTableException::SymbolTableException(const std::string& message, const unsigned int& line) {
	// construct the message
	std::string err_message = "**** SymbolTable Error: " + message;
	// if we have line number data, append it to the error message
	if (line > 0) {
		err_message += " (line " + std::to_string(line);
	}
	this->line = line;
	this->message = err_message;
}


// Assembler Exceptions

const char* AssemblerException::what() const noexcept {
	return message.c_str();
}

AssemblerException::AssemblerException(const std::string& message, const unsigned int& line) {
	std::string err_message = "**** Assembler Error: " + message + " (line " + std::to_string(line) + ")";
	this->line = line;
	this->message = err_message;
}
