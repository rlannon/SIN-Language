#include "Exceptions.h"

/*

	Exceptions.cpp

	The implementation of the classes/methods defined in Exceptions.h

*/

// Compiler Exceptions

const char* CompilerException::what() const noexcept {
	std::string error_message = "**** Compiler Error " + std::to_string(this->code) + ": " + this->message + " (line " + std::to_string(this->line) + ")";
	return error_message.c_str();
}

CompilerException::CompilerException(const std::string& message, unsigned int code, unsigned int line) : message(message), code(code), line(line) {

}

CompilerException::CompilerException()
{
	this->message = "";
	this->code = 0;
	this->line = 0;
}



// Parser Exceptions

const char* ParserException::what() const noexcept {
	std::string error_message = "**** Parser error: " + this->message + "(Error occurred on line " + std::to_string(this->line_number) + ")";
	return error_message.c_str();
}

int ParserException::get_code() {
	return ParserException::code;
}

ParserException::ParserException(const std::string& err_message, const int& err_code, const int& line_number) : code(err_code), line_number(line_number) {
	
}



// SINVM Exceptions

const char* VMException::what() const noexcept {
	std::stringstream err_ss;
	err_ss << "**** SINVM Error: " << this->message << std::endl << "Error was encountered at memory location " << std::hex << this->address << std::dec << std::endl;
	std::string err_msg = err_ss.str();

	return err_msg.c_str();
}

VMException::VMException(const std::string& message, uint16_t address) : message(message), address(address) {

}
