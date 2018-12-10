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
#include <string>
#include <exception>

// Custom headers
#include "Lexer.h"
#include "Parser.h"
#include "Interpreter.h"
#include "SINVM.h"
#include "Assembler.h"


int main(int argc, char** argv) {
	/*
	
	The main function currently serves to test the tokenizer and parser. As the program gets more advanced, the main function will be modified, and eventually accept command-line arguments so that the program may be used in the command line to parse and interpret/compile files. Currently, however, everything is hard-coded and will remain that way for the forseeable future.
	
	*/

	// Program mode -- interpret by default; compile only if we set the flag
	bool compile = false;

	// Create the path for our source file
	std::string src_file_path;

	// First, check for command-line arguments beyond the name of the program
	// If we only have one argument, then we need to get other information from the user
	if (argc == 1) {
		// Prompt the user for a file
		// This allows us to use the program without the command line
		std::cout << "No input file specified. Enter a filename:" << std::endl;
		std::getline(std::cin, src_file_path);
	}
	else {
		// if we have 2 command-line arguments, then the second one should be a filename
		if (argc == 2) {
			// The path to our file should be our first argument, should it be executed from the command line with arguments
			src_file_path = argv[1];
		}
		// if we have 3, then we are supplying another keyword -- "interpret" or "compile"
		// note: use std::regex::icase to ignore case
		else if (argc == 3) {
			// check to see if we want to interpret or compile
			if (std::regex_search(argv[1], std::regex("(interpret)|(-i)", std::regex::icase))) {
				compile = false;
			}
			else if (std::regex_search(argv[1], std::regex("(compile)|(-c)", std::regex::icase))) {
				compile = true;
			}
			// if the user entered nonsense
			else {
				// print an error message
				std::cerr << "Error: '" << argv[1] << "' is an invalid operation mode. Options are 'interpret' or 'compile' (or '-i' and '-c', respectively)" << std::endl;
				// quit
				exit(2);
			}
			// the third argument should be a filename
			src_file_path = argv[2];
		}
	}

	// Check to make sure our file is a valid type; quit if it isn't
	if (!std::regex_search(src_file_path, std::regex("(.sin)|(.sinc)|(.sinasm)"))) {
		std::cerr << std::endl << "File must be either a .sin, .sina, or .snc file!" << std::endl << "Press enter to exit." << std::endl;
		std::cin.get();
		exit(1);
	}

	// Open our file
	std::ifstream src_file;

	// try opening the file; if we can't, then we will handle the exception
	src_file.open(src_file_path, std::ios::in);

	// Create an object for our abstract syntax tree
	StatementBlock prog;

	if (src_file.is_open()) {
		// if it is precompiled
		if (std::regex_search(src_file_path, std::regex(".sinc"))) {
			try {
				//SINVM sinVM(src_file, true);	// we don't need to disassemble; is a .sinc file
				//sinVM.run_program();

				std::cout << std::endl;
			}
			catch (std::exception& e) {
				std::cerr << e.what() << std::endl;
				std::cerr << "Press enter to exit" << std::endl;
				std::cin.get();
			}
		}
		else if (std::regex_search(src_file_path, std::regex(".sina"))) {
			try {
				//SINVM sinVM(src_file, false);	// we need to disassemble; is a .sina file
				//sinVM.run_program();

				//sinVM._debug_values();

				try {
					Assembler assemble(&src_file);
					SINVM myVM(assemble);
					myVM.run_program();
					myVM._debug_values();
				}
				catch (std::exception& e) {
					std::cerr << e.what() << std::endl;
					std::cerr << "Press enter to exit." << std::endl;
					std::cin.get();
					exit(1);
				}

				std::cout << "Press enter to exit" << std::endl;
				std::cin.get();
			}
			catch (std::exception& e) {
				std::cerr << e.what() << std::endl;
				std::cerr << "Press enter to exit" << std::endl;
				std::cin.get();
			}
		}
		// if it isn't precompiled
		else {
			// Parse the token file to create the abstract syntax tree
			try {
				std::cout << "> Tokenizing..." << std::endl;
				Lexer lex(&src_file);
				Parser parser(lex);

				src_file.close();

				std::cout << "> Creating AST..." << std::endl;

				prog = parser.createAST();
			}
			catch (ParserException& pe) {
				std::cerr << std::endl << "Parser Error:" << std::endl;
				std::cerr << "\t" << pe.what() << std::endl;
				std::cerr << "\tCode: " << pe.get_code() << std::endl << std::endl;
			}
			catch (LexerException& le) {
				std::cerr << std::endl << "Lexer Error:" << std::endl;
				std::cerr << "\t" << le.what() << "\n\tViolating Character: " << le.get_char() << "\n\tPosition: " << le.get_pos() << std::endl << std::endl;
			}
			catch (std::exception& e) {
				std::cerr << std::endl << e.what() << std::endl << std::endl;
			}

			// if we want to interpret
			if (!compile) {
				// Create the interpreter object
				Interpreter interpreter;

				std::cout << "> Executing program..." << std::endl << std::endl;
				// Parse our abstract syntax tree
				try {
					interpreter.interpretAST(prog);
				}
				catch (int e) {
					std::cout << "Exception code: " << e << std::endl;
				}

				// print a "done" message and wait until user hits enter before closing the program
				std::cout << std::endl << "> Done. Press enter to exit." << std::endl;
				std::cin.get();
			}
			// if we want to compile
			else if (compile) {
				// currently, we don't support it, so abort
				std::cout << "Program compilation currently not supported. Exiting..." << std::endl;
				// wait for the user to hit enter
				std::cin.get();
			}

			// exit the program
			return 0;
		}
	}
		// if we can't open the file, print an error and quit
	else {
		std::cerr << "Could not open the file specified." << std::endl << "Press enter to exit." << std::endl;
		std::cin.get();
		exit(2);
	}
}