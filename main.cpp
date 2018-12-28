/*

SIN Compiler
main.cpp
Copyright 2018 Riley Lannon (truffly)

This is the main file for the SIN Compiler application. SIN is a small programming language that I will be using
as a way to get practice in compiler writing.

For a documentation of the language, see either the doc folder in this project or the Github wiki pages.

*/

// Pre-made libraries
// These are all included in other included headers, but leaving them here to show we will be using them
//#include <iostream>
//#include <fstream>
//#include <string>
//#include <vector>
//#include <exception>
//#include <algorithm>	// for std::find

// Custom headers
//#include "Lexer.h"
//#include "Parser.h"		// Lexer.h and Parser.h are also included in Interpreter.h, but commenting here to denote they are being used
#include "Interpreter.h"
#include "SINVM.h"
//#include "Assembler.h"	// included by SINVM.h, but included here as a comment to denote that those functions are being used in this file
#include "Compiler.h"
#include "Linker.h"
#include "SinObjectFile.h"



void file_error(std::string filename) {
	std::cerr << "**** Cannot open file '" + filename + "'!" << "\n\n" << "Press enter to exit..." << std::endl;
	std::cin.get();
	return;
}



int main (int argc, char** argv[]) {
	// first, make a vector of strings to hold **argv 
	std::vector<std::string> program_arguments;
	// create a vector of SinObjectFile to hold any filenames that are passed as parameters
	std::vector<SinObjectFile> objects_vector;

	// if we didn't pass in any command-line parameters, we need to make sure we get the necessary information from the user
	if (argc == 1) {
		std::string parameters;
		std::cout << "Parameters: ";
		std::getline(std::cin, parameters);	// get the parameters as a string

		// now, turn that into a vector of strings using a delimiter (in this case, a space)
		// split the line into parts, if we can
		std::vector<std::string> string_delimited;	// to hold the parts of our string
		size_t position = 0;
		while ((position = parameters.find(' ')) != std::string::npos) {
			string_delimited.push_back(parameters.substr(0, parameters.find(' ')));	// push the string onto the vector
			parameters.erase(0, position + 1);	// erase the string from the beginning up to the next token
		}
		string_delimited.push_back(parameters);	// add the last thing (we iterated one time too few so we didn't try to erase more than what existed)

		// and set program_arguments equal to that new vector
		program_arguments = string_delimited;
	}
	// otherwise, just push all of **argv onto program_arguments
	else {
		for (int i = 0; i < argc; i++) {
			program_arguments.push_back(*argv[i]);
		}
	}

	// now, create booleans for our various flags and set them if necessary
	bool compile = false;
	bool disassemble = false;
	bool assemble = false;	// will automatically assemble files with the -c flag; if --no-assemble is set, it will clear it
	bool link = false;
	bool execute = false;
	bool debug_values = false;	// if we want "SINVM::_debug_values()" after execution

	// wordsize will default to 16, but we can set it with the --wsxx flag
	uint8_t wordsize = 16;

	// our file name should be the zeroth element in the vector (syntax is "SIN file_name flags")
	std::string filename = program_arguments[0];
	std::string file_extension;
	std::string filename_no_extension;

	// get the file extension using "std::find"
	size_t extension_position = filename.find(".");
	if (extension_position != filename.npos) {
		file_extension = filename.substr(extension_position);	// using substr() we can get the extension from the position we just found
		// create a copy of the filename without an extension
		filename_no_extension = filename.substr(0, extension_position);
	}
	else {
		std::cerr << "**** First argument must be a filename." << std::endl;
		std::cerr << "Press enter to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// iterate through the vector and set flags appropriately
	for (std::vector<std::string>::iterator arg_iter = program_arguments.begin(); arg_iter != program_arguments.end(); arg_iter++) {
		// only check for flags if the first character is '-'
		if ((*arg_iter)[0] == '-') {

			if (std::regex_match(*arg_iter, std::regex("-[a-zA-Z0-9]*c.*", std::regex_constants::icase)) || (*arg_iter == "--compile")) {
				compile = true;
			}

			if (std::regex_match(*arg_iter, std::regex("-[a-zA-Z0-9]*s.*", std::regex_constants::icase)) || (*arg_iter == "--assemble")) {
				assemble = true;
			}

			if (std::regex_match(*arg_iter, std::regex("-[a-zA-Z0-9]*d.*", std::regex_constants::icase)) || (*arg_iter == "--disassemble")) {
				disassemble = true;
			}

			if (std::regex_match(*arg_iter, std::regex("-[a-zA-Z0-9]*l.*", std::regex_constants::icase)) || (*arg_iter == "--link")) {
				link = true;
			}

			if (std::regex_match(*arg_iter, std::regex("-[a-zA-Z0-9]*e.*", std::regex_constants::icase)) || (*arg_iter == "--execute")) {
				execute = true;
			}

			// if the compile-only flag is set
			if ((*arg_iter == "--compile-only")) {
				compile = true;	// set only the compile flag; clear all others regardless if we set them or not
				assemble = false;
				disassemble = false;
				link = false;
				execute = false;
			}

			// if the debug flag is set
			if ((*arg_iter == "--debug")) {
				debug_values = true;
			}

			// if we explicitly set word size
			if (std::regex_match(*arg_iter, std::regex("--ws.+", std::regex_constants::icase))) {
				std::string wordsize_string = arg_iter->substr(4);
				wordsize = (uint8_t)std::stoi(wordsize_string);
			}
		}
		// if we have a .sinc file as a parameter
		else if (std::regex_match(*arg_iter, std::regex("[a-zA-Z0-9_]+\.sinc"))) {
			std::ifstream sinc_file;
			sinc_file.open(*arg_iter, std::ios::in | std::ios::binary);
			if (sinc_file.is_open()) {
				// initialize the object file
				SinObjectFile obj_file(sinc_file);
				objects_vector.push_back(obj_file);

				sinc_file.close();
			}
			else {
				file_error(filename);
				exit(1);
			}
		}
	}

	// TODO: validate flags against file extension

	// Now that we have the flags all proper, we can execute things in the correct order
	// The functions are called in this order: compile, disassemble, assemble, link, execute
	
	// TODO: update "filename" and "extension" after we run various functions; this is how we will validate
	try {
		// compile a .sin file
		if (compile) {
			// validate file type
			if (file_extension == ".sin") {
				// open the file
				std::ifstream sin_file;
				sin_file.open(filename, std::ios::in);

				if (sin_file.is_open()) {
					// create a vector of file names
					std::vector<std::string> object_file_names;
					// compile the file
					Compiler compiler(sin_file, wordsize, &object_file_names);
					
					// create SinObjectFile objects for each item in object_file_names
					for (std::vector<std::string>::iterator it = object_file_names.begin(); it != object_file_names.end(); it++) {
						// open the file of the name stored in object_file_names at the iterator's current position
						std::ifstream sinc_file;
						sinc_file.open(*it, std::ios::in | std::ios::binary);

						if (sinc_file.is_open()) {
							// if we can find the file, then create an object from it and add it to the object vector
							SinObjectFile obj(sinc_file);
							objects_vector.push_back(obj);
						}
						else {
							file_error(*it);
							exit(1);
						}

						// close the file before the next iteration
						sinc_file.close();
					}

					// update the filename
					file_extension = ".sina";
					filename = filename_no_extension + file_extension;

					sin_file.close();
				} else {
					file_error(filename);
					exit(1);
				}
			}
			else {
				throw std::exception("**** To compile, file type must be 'sin'.");
			}
		}
		
		// use an Assembler object for assembly and disassembly alike
		if (disassemble || assemble) {
			if (disassemble) {
				// validate file type
				if ((file_extension == ".sinc") || file_extension == ".sml") {
					std::ifstream to_disassemble;
					to_disassemble.open(filename);
					if (to_disassemble.is_open()) {
						// initialize the assembler with the file and our "wordsize" variable, which defaults to 16
						Assembler disassembler(to_disassemble, wordsize);
						disassembler.disassemble(to_disassemble, filename_no_extension);

						// update the filename
						file_extension = ".sina";
						filename = filename_no_extension + file_extension;

						to_disassemble.close();
					}
					else {
						file_error(filename);
						exit(1);
					}
				}
				else {
					throw std::exception("**** To disassemble, file type must be 'sinc' or 'sml'.");
				}
			}
			else if (assemble) {
				if (file_extension == ".sina") {
					std::ifstream sina_file;
					sina_file.open(filename);
					if (sina_file.is_open()) {
						// assemble the file
						Assembler assemble(sina_file, wordsize);
						assemble.create_sinc_file(filename_no_extension);

						// update our object files list
						std::vector<std::string> obj_filenames = assemble.get_obj_files_to_link();	// so we don't need to execute a function in every iteration of the loop
						for (std::vector<std::string>::iterator it = obj_filenames.begin(); it != obj_filenames.end(); it++) {
							// create the SinObjectFile object as well as the file stream for the sinc file
							SinObjectFile obj_file_from_assembler;
							std::ifstream sinc_file;
							sinc_file.open(*it);

							if (sinc_file.is_open()) {
								// open the file
								obj_file_from_assembler.load_sinc_file(sinc_file);
								// add it to our objects vector
								objects_vector.push_back(obj_file_from_assembler);
								// close the file
								sinc_file.close();
							}
							else {
								throw std::exception(("**** Could not load object file '" + *it + "' after assembly.").c_str());
							}
						}

						// update the filename
						file_extension = ".sinc";
						filename = filename_no_extension + file_extension;

						sina_file.close();
					}
					else {
						file_error(filename);
						exit(1);
					}
				}
				else {
					throw std::exception("**** To assemble, file type must be 'sina'.");
				}
			}
		}

		// link files
		if (link) {
			// if we have object files to linke
			if (objects_vector.size() != 0) {
				// create a linker object using our objects vector
				Linker linker(objects_vector);
				linker.create_sml_file(filename_no_extension);

				// TODO: update the file name after linking

				// TODO: pass "filename_no_extension" into the objects so they produce files of the proper name?
				// TODO: add functionality to compiler to return a list of file names that are needed at link time; add to assembler as well

				// update the filename
				file_extension = ".sml";
				filename = filename_no_extension + file_extension;
			}
			else {
				throw std::exception("**** You must supply object files to link.");
			}
		}

		// execute a file
		if (execute) {
			// validate file extension
			if (file_extension == ".sml") {
				std::ifstream sml_file;
				sml_file.open(filename, std::ios::in | std::ios::binary);
				if (sml_file.is_open()) {
					// create an instance of the SINVM with our SML file and run it
					SINVM vm(sml_file);
					vm.run_program();

					if (debug_values) {
						vm._debug_values();
					}

					sml_file.close();
				}
				else {
					file_error(filename);
					exit(1);
				}

			}
			else {
				throw std::exception("**** The SIN VM may only run SIN VM executable files (.sml).");
			}
		}
	}
	// if an exception was thrown, catch it
	catch (std::exception& e) {
		// print the error message
		std::cerr << "The program had to abort because the following exception occurred:" << std::endl;
		std::cerr << "\t" << e.what() << std::endl;
		std::cerr << "\nPress enter to exit..." << std::endl;

		// wait for user input before exiting
		std::cin.get();

		// exit with code 2 (an execption occurred)
		exit(2);
	}
}