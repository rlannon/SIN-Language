/*

SIN Compiler
main.cpp
Copyright 2018 Riley Lannon (truffly)

This is the main file for the SIN Compiler application. SIN is a small programming language that I will be using
as a way to get practice in compiler writing.

For a documentation of the language, see the doc folder in this project.

*/

// Pre-made libraries
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>

// Custom headers
#include "Lexer.h"

int main() {
	std::ifstream myfile;
	myfile.open("parser_test.txt", std::ios::in);
	Lexer stream(&myfile);
	std::tuple<std::string, std::string> lexeme;

	while (!stream.eof() && !stream.exit_flag_is_set()) {
		lexeme = stream.read_next();
		std::cout << "Lexeme:" << std::endl;
		std::cout << "{ type: " << std::get<0>(lexeme);
		std::cout << ", value: " << std::get<1>(lexeme) << " }" << std::endl << std::endl;
	}

	std::cin.get();	// wait until user hits enter before closing the program
	myfile.close();

	return 0;
}