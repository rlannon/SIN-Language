#pragma once

#include <vector>
#include <list>
#include <string>
#include <tuple>

#include "Parser.h"
// #include "Expression.h"	// included in "Parser.h"
// #include "Statement.h"	// included in "Parser.h"

/*

The Compiler for the SIN language. Given an AST produced by the Parser, will produce a .sina file that can execute the given code in the SIN VM.

*/

class Compiler
{
	StatementBlock AST;
	int _DATA_PTR;	// holds the next memory address to use for a variable in the _DATA section
public:
	Compiler(StatementBlock AST);	// initialize the AST to be parsed by the compiler
	~Compiler();
};

