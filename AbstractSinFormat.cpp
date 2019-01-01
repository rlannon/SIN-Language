#include "AbstractSinFormat.h"



void AbstractSinFormat::produce_absn_file(std::ostream& output_file, StatementBlock AST)
{
	// iterate through the AST and prodce the correct code statement by statement
	for (std::vector<std::shared_ptr<Statement>>::iterator statement_iter = AST.statements_list.begin(); statement_iter != AST.statements_list.end(); statement_iter++) {
		// get the current statement
		std::string statement_type = statement_iter->get()->get_type();
		
		// decide what to do based on the type
		if (statement_type == "include") {

		}
		else if (statement_type == "alloc") {

		}
		else if (statement_type == "assign") {

		}
		else if (statement_type == "return") {

		}
		else if (statement_type == "ite") {

		}
		else if (statement_type == "while") {

		}
		else if (statement_type == "def") {

		}
		else if (statement_type == "call") {

		}
	}
}

AbstractSinFormat::AbstractSinFormat()
{
}


AbstractSinFormat::~AbstractSinFormat()
{
}
