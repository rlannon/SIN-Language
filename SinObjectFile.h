#pragma once

#include <tuple>
#include <vector>
#include <list>
#include <string>
#include <iostream>
#include <fstream>

#include "BinaryIO.h"
#include "Assembler.h"

/*

This file contains the definition for the SinObjectFile class, a class to hold all of the necessary data contained within a SIN Object File (.sinc). It allows the user to easily access various data about the file/program through using its methods.

*/

class Assembler;	// forward declare "Assembler" so we can use it here

// to maintain the .sinc file standard
const uint8_t sinc_version = 2;

// numeric constants to represent our data types
namespace symbol_constants {
	const uint8_t _void_t = 0x00;
	const uint8_t _bool_t = 0x01;
	const uint8_t _int_t = 0x02;
	const uint8_t _float_t = 0x03;
	const uint8_t _string_t = 0x04;

	const uint8_t _ptr_t = 0x10;
	const uint8_t _array_t = 0x20;
}

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
	void write_sinc_file(std::string output_file_name, Assembler* assembler_obj);

	// get data about the file/program
	uint8_t get_wordsize();
	std::vector<uint8_t> get_program_data();

	SinObjectFile();
	SinObjectFile(std::istream& file);
	~SinObjectFile();
};
