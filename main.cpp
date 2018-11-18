/*

SIN Compiler
main.cpp
Copyright 2018 Riley Lannon (truffly)

This is the main file for the SIN Compiler application. SIN is a small programming language that I will be using
as a way to get practice in compiler writing.

For a documentation of the language, see either the doc folder in this project or the Github wiki pages.

*/

// Pre-made libraries
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>

// Custom headers
#include "Lexer.h"
#include "Parser.h"
#include "Interpreter.h"


int main() {
	/*
	
	The main function currently serves to test the tokenizer and parser. As the program gets more advanced, the main function will be modified, and eventually accept command-line arguments so that the program may be used in the command line to parse and interpret/compile files. Currently, however, everything is hard-coded and will remain that way for the forseeable future.
	
	*/
	
	//First, test the lexer
	std::ifstream src_file;
	src_file.open("parser_test.txt", std::ios::in);
	Lexer lex(&src_file);

	std::ofstream token_file;
	token_file.open("tokens.txt", std::ios::out);

	while (!lex.eof() && !lex.exit_flag_is_set()) {
		std::tuple<std::string, std::string> token = lex.read_next();
		token_file << std::get<0>(token) << "\n" << std::get<1>(token) << "\n";
	}

	token_file.close();
	src_file.close();
	// Now, test the parser
	
	std::ifstream infile;
	infile.open("tokens.txt");

	Parser parser(&infile);

	infile.close();

	std::shared_ptr<Statement> some_stmt;
	StatementBlock prog;
	try {
		prog = parser.parse_top();
	}
	catch (int e) {
		std::cout << e << std::endl;
	}

	Interpreter interpreter;
	
	try {
		int i = 0;
		while (i < prog.StatementsList.size()) {
			Statement* statement = dynamic_cast<Statement*>(prog.StatementsList[i].get());
			interpreter.evaluate_statement(statement, interpreter.get_vars_table());
			i++;
		}
	}
	catch (int e) {
		std::cout << "Exception code: " << e << std::endl;
	}

	// wait until user hits enter before closing the program
	std::cin.get();

	// exit the program
	return 0;
}