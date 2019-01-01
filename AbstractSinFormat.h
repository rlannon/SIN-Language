#pragma once

#include "Statement.h"

/*

	The AbstractSinFormat class implements the .absn file format, or the Abstract-SIN type. These files contain an AST produced by the parser.

*/

class AbstractSinFormat
{

public:
	void produce_absn_file(std::ostream& output_file, StatementBlock AST);
	StatementBlock load_absn_file(std::istream& input_file);

	AbstractSinFormat();
	~AbstractSinFormat();
};

