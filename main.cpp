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

int main() {
	/*
	
	The main function currently serves to test the tokenizer and parser. As the program gets more advanced, the main function will be modified, and eventually accept command-line arguments so that the program may be used in the command line to parse and interpret/compile files. Currently, however, everything is hard-coded and will remain that way for the forseeable future.
	
	*/
	
	// our source file we want to use the lexer on
	std::ifstream src_file;
	src_file.open("parser_test.txt", std::ios::in);
	Lexer stream(&src_file);
	std::tuple<std::string, std::string> lex;

	src_file.close();

	// our token file
	std::ifstream token_file;
	token_file.open("tokens.txt", std::ios::in);
	Parser parser(&token_file);

	std::cout << "Parsing..." << std::endl << std::endl;

	try {
		parser.parse_top();
	}
	catch (int code) {
		std::cerr << "The parser encountered a fatal error and was forced to abort. Error code: " << code << std::endl;
	}

	std::cout << "Done." << std::endl;

	token_file.close();

	// wait until user hits enter before closing the program
	std::cin.get();

	// exit the program
	return 0;
}