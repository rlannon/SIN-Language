#include "CompilerException.h"

const char* CompilerException::what() const noexcept {
	std::string error_message = "Error " + std::to_string(this->code) + ": " + this->message + " (line " + std::to_string(this->line) + ")";
	return error_message.c_str();
}

CompilerException::CompilerException(const std::string& message, unsigned int code, unsigned int line) : message(message), code(code), line(line) {

}

CompilerException::CompilerException()
{
}


CompilerException::~CompilerException()
{
}
