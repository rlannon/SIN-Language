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

VMException::VMException(const std::string& message, const uint16_t& address) : message(message), address(address) {
	// we must construct the message here, in the constructor
	std::stringstream err_ss;
	err_ss << "**** SINVM Error: " << this->message << std::endl << "Error was encountered at memory location " << std::hex << this->address << std::dec << std::endl;
	this->message = err_ss.str();
}
