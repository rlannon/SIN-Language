#pragma once

#include <tuple>
#include <vector>
#include <list>
#include <string>
#include <iostream>
#include <fstream>

#include "BinaryIO.h"

/*

This file contains the definition for the SinObjectFile class, a class to hold all of the necessary data contained within a SIN Object File (.sinc). It allows the user to easily access various data about the file/program through using its methods.

*/

// to maintain the .sinc file standard
const uint8_t sinc_version = 2;

// create a data type that will hold all the data for an Assembler object that we will need to write a .sinc file; this is to avoid circular dependencies
typedef struct AssemblerData
{
	uint8_t _wordsize;
	std::vector<uint8_t> _text;
	std::list<std::tuple<std::string, int, std::string>> _symbol_table;
	std::list<std::tuple<std::string, int>> _relocation_table;
	std::list<std::tuple<std::string, std::vector<uint8_t>>> _data_table;

	AssemblerData(uint8_t _wordsize, std::vector<uint8_t> _text, std::list<std::tuple<std::string, int, std::string>> _symbol_table, std::list<std::tuple<std::string, int>> _relocation_table, std::list<std::tuple<std::string, std::vector<uint8_t>>> _data_table);
	AssemblerData();
	~AssemblerData();
};

class SinObjectFile
{
	// allow the Linker to access these private members
	friend class Linker;

	// a vector to hold our program data
	std::vector<uint8_t> program_data;

	// a list of tuples for our symbol and relocation tables
	std::list<std::tuple<std::string, int, std::string>> symbol_table;
	std::list<std::tuple<std::string, int, std::vector<uint8_t>>> data_table;	// includes an int to tell the offset from the end of the .text section
	std::list<std::tuple<std::string, int>> relocation_table;

	// the wordsize of our program
	uint8_t _wordsize;

	// the sinVM version
	uint8_t _sinvm_version;

	// the entry point
	uint16_t _text_start;
public:
	// load a .sinc file and populate our class data accordingly
	void load_sinc_file(std::istream& file);
	void write_sinc_file(std::string output_file_name, AssemblerData assembler_obj);

	// get data about the file/program
	uint8_t get_wordsize();
	std::vector<uint8_t> get_program_data();

	SinObjectFile();
	SinObjectFile(std::istream& file);
	~SinObjectFile();
};
