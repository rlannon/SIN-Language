#pragma once

#include <vector>
#include <tuple>
#include <string>
#include <iostream>
#include <fstream>

#include "SinObjectFile.h"
#include "VMMemoryMap.h"	// the memory map for SINVM version 1

class Linker
{
	/*
	
	The Linker takes a series of object files and links them together so that they can be used in one executable. It searches through all of the object files to resolve references, sets the proper memory offsets for all control flow instructions, and modifies the binary instructions to use the correct addresses.
	
	*/

	// a vector to hold all of the object files we will be working with; a vector will allow us to index
	std::vector<SinObjectFile> object_files;

	// our program metadata
	uint8_t _wordsize;	// the wordsize of the program
	size_t _start_offset;	// the start address of the program; in 16-bit VM version 1, it is 0x2600
	size_t _rs_start;	// the start address for macros/variables using the @rs directive

	// get the word size, start address, etc. based on the info in our .sinc files
	void get_metadata();
public:
	// entry function; creates an sml file; this will use Linker::object_files
	void create_sml_file(std::string file_name);

	Linker();
	Linker(std::vector<SinObjectFile> object_files);
	~Linker();
};

