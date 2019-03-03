#pragma once

#include <exception>
#include <string>

class CompilerException : public std::exception
{
	const std::string message;
	unsigned int code;
	unsigned int line;
public:
	explicit CompilerException(const std::string& message, unsigned int code = 0, unsigned int line = 0);
	virtual const char* what() const noexcept;

	CompilerException();
	~CompilerException();
};

