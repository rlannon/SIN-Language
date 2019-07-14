#include "Linker.h"



void Linker::get_metadata() {
	// iterate through the .sinc files to get the file metadata and ensure they are compatible
	// if they aren't, throw an exception
	
	// first, we need to get the wordsize and version of the 0th file; this will be our reference
	uint8_t wordsize_compare = this->object_files[0]._wordsize;
	uint8_t version_compare = this->object_files[0]._sinvm_version;

	// then, iterate through our file vector and compare the word size to the 0th file
	// if they match, continue; if they don't, throw an error and die
	for (std::vector<SinObjectFile>::iterator file_iter = this->object_files.begin(); file_iter != this->object_files.end(); file_iter++) {
		if (file_iter->_wordsize != wordsize_compare) {
			throw std::runtime_error("**** Word sizes in all object files must match.");
		}

		if (file_iter->_sinvm_version != version_compare) {
			throw std::runtime_error("**** SINVM Version must be the same between all object files.");
		}
	}

	// now that we know everything matches, set the start address and word size to be used by the linker in creating a .sml file
	this->_wordsize = wordsize_compare;

	// set our memory location information based on the version of the SIN VM
	if (version_compare == 1) {
		this->_start_offset = _PRG_BOTTOM;
		this->_rs_start = _RS_START;
	}
	else {
		throw std::runtime_error("**** Specified SIN VM version is not currently supported by this toolchain");
	}
}


// create a .sml file from all of our objects
void Linker::create_sml_file(std::string file_name) {

	// first, update all the memory offsets
	size_t current_offset = this->_start_offset;	// current offset starts at the start address of VM memory
	size_t current_rs_address = this->_rs_start;	// the next available address for a @rs directive starts at Linker::_rs_start

	// iterate over the whole vector
	for (std::vector<SinObjectFile>::iterator file_iter = this->object_files.begin(); file_iter != this->object_files.end(); file_iter++) {
		// _text_start holds the start address, from which all control flow addresses in the file are offset
		file_iter->_text_start = current_offset;

		// add this offset to every "value" field in the symbol table where the symbol class is "D", or "C"
		// symbols with class "M" will not be updated; the address defined will remain
		for (std::list<AssemblerSymbol>::iterator symbol_iter = file_iter->symbol_table.begin(); symbol_iter != file_iter->symbol_table.end(); symbol_iter++) {
			
			// if our symbol is defined
			if ((symbol_iter->symbol_class == D) || (symbol_iter->symbol_class == C)) {
				// update the value stored in the symbol table to be the current value plus the offset
				symbol_iter->value = symbol_iter->value + current_offset;
			}

			/*
			if the symbol table is "C", we also need to add the offsets from both:
				a) the size of the .text section
				b) the offset from the /end/ of the .text section
			*/

			if (symbol_iter->symbol_class == C) {
				// iterate through the file's constants list to find the constant's data
				std::list<std::tuple<std::string, size_t, std::vector<uint8_t>>>::iterator data_iter = file_iter->data_table.begin();
				bool found_constant = false;
				while (!found_constant && (data_iter != file_iter->data_table.end())) {
					if (std::get<0>(*data_iter) == symbol_iter->name) {
						found_constant = true;
					}
					else {
						data_iter++;
					}
				}
				if (found_constant) {
					// get the total offset
					size_t offset_from_text_end = std::get<1>(*data_iter);

					// add the total offset to <1>(symbol_iter), which is the offset for the current symbol in the symbol table; this symbol is class "C"
					symbol_iter->value += offset_from_text_end;
				}
				else {
					throw std::runtime_error("Could not find the constant specified in the constants table!");
				}
			}

			// if the symbol class is "R", we need to allocate space in memory for it
			if (symbol_iter->symbol_class == R) {
				// first, make sure "current_rs_address" is not beyond the memory we are allowed to use; it cannot move beyond the heap
				if (current_rs_address >= _RS_END) {
					throw std::runtime_error("**** Memory Exception: Global variable limit exceeded.");
				}
				// if we are still within our bounds
				else {
					// we will change the value of the symbol to the next available memory address based on "current_rs_address"
					symbol_iter->value = current_rs_address;

					// update current_rs_address; advance by the size of the symbol (usually one word, but not necessarily)
					current_rs_address += symbol_iter->width;
				}
			}

			// the "U" class will be updated later, so ignore it here
		}

		// update current_offset to add the size of the program just added
		current_offset += file_iter->program_data.size();

		// we then need to update current_offset to add the size of the .data section, as these will get added onto the program sections at the end
		size_t data_section_offset = 0;

		for (std::list<std::tuple<std::string, size_t, std::vector<uint8_t>>>::iterator data_it = file_iter->data_table.begin(); data_it != file_iter->data_table.end(); data_it++) {
			size_t constant_size = std::get<2>(*data_it).size();
			data_section_offset += constant_size;
		}

		current_offset += data_section_offset;
	}

	// now that our initial offsets and defined label offsets have been adjusted, we can construct the master symbol table

	// first, create the vector to hold the table
	std::vector<AssemblerSymbol> master_symbol_table;
	// iterate through our object files' symbol tables to add to the master table

	for (std::vector<SinObjectFile>::iterator file_iter = this->object_files.begin(); file_iter != this->object_files.end(); file_iter++) {
		// only add defined references to the table
		// find them by iterating through each symbol table and adding the defined symbols to the master table
		for (std::list<AssemblerSymbol>::iterator symbol_iter = file_iter->symbol_table.begin(); symbol_iter != file_iter->symbol_table.end(); symbol_iter++) {
			if (symbol_iter->symbol_class == D || symbol_iter->symbol_class == C || symbol_iter->symbol_class == R || symbol_iter->symbol_class == M) {
				master_symbol_table.push_back(*symbol_iter);
			}
			else {
				continue;
			}
		}
	}

	// now, go through each object file and locate any undefined symbol references
	for (std::vector<SinObjectFile>::iterator file_iter = this->object_files.begin(); file_iter != this->object_files.end(); file_iter++) {
		// look for undefined symbols
		for (std::list<AssemblerSymbol>::iterator symbol_iter = file_iter->symbol_table.begin(); symbol_iter != file_iter->symbol_table.end(); symbol_iter++) {
			// if it is a constant, the symbol should now have the proper address
			// if it's undefined, a constant, or a reserved macro
			if (symbol_iter->symbol_class == U || symbol_iter->symbol_class == C || symbol_iter->symbol_class == R) {
				// iterate through the master table and find the symbol referenced
				std::vector<AssemblerSymbol>::iterator master_table_iter = master_symbol_table.begin();
				bool found = false;
				while ((master_table_iter != master_symbol_table.end()) && !found) {
					// if the symbol names are the same
					if (master_table_iter->name == symbol_iter->name) {
						// copy over the value from the master table iterator to our local symbol table
						symbol_iter->value = master_table_iter->value;

						// update our found variable so we can abort
						found = true;
					}
					else {
						// increment the master_symbol_table iterator
						master_table_iter++;
					}
				}

				// if the symbol was NOT found in the table
				if (!found) {
					throw std::runtime_error("**** Symbol table error: Could not find '" + symbol_iter->name + "' in symbol table!");
				}
			}
			else {
				continue;	// skip defined references
			}
		}

		// now, go through each file individually and update all values pointed to in the relocations table
		for (std::vector<SinObjectFile>::iterator file_iter = this->object_files.begin(); file_iter != this->object_files.end(); file_iter++) {
			// iterate through the relocation table
			for (std::list<RelocationSymbol>::iterator relocation_iter = file_iter->relocation_table.begin(); relocation_iter != file_iter->relocation_table.end(); relocation_iter++) {
				// if the symbol is "_NONE", we add the file's offset to the value pointed to by the relocator
				if (relocation_iter->name == "_NONE") {
					// the start address is the value in the relocation iter
					size_t value_start_address = relocation_iter->value;

					unsigned int data = 0;
					size_t wordsize_bytes = (size_t)file_iter->_wordsize / 8;

					// the value to write is located at value_start_address
					for (size_t i = 0; i < (wordsize_bytes - 1); i++) {
						data += file_iter->program_data[value_start_address + i];
						data = data << (i * 8);
					}
					data += file_iter->program_data[value_start_address + (wordsize_bytes - 1)];

					// write the big-endian value to the addresses starting at value_start_address
					for (size_t i = wordsize_bytes; i > 0; i--) {
						file_iter->program_data[value_start_address + (wordsize_bytes - i)] = data >> ((i - 1) * 8);
					}
				}
				else {
					// retrieve "value" from the master symbol table
					std::vector<AssemblerSymbol>::iterator master_table_iter = master_symbol_table.begin();
					bool found = false;
					size_t value = 0;
					while ((master_table_iter != master_symbol_table.end()) && !found) {
						// if the names are the same
						if (master_table_iter->name == relocation_iter->name) {
							// get the value
							value = master_table_iter->value;
							// set the found variable and exit the loop
							found = true;
						}
						else {
							master_table_iter++;
						}
					}

					// if we didn't find it, throw an error
					if (!found) {
						throw std::runtime_error("**** Relocation error: Could not find '" + relocation_iter->name + "' in symbol table!");
					}

					// finally, write "value" to the relocation table
					// get the start address and the number of bytes in our wordsize
					size_t value_start_address = relocation_iter->value;
					size_t wordsize_bytes = (size_t)file_iter->_wordsize / 8;
					for (size_t i = wordsize_bytes; i > 0; i--) {
						file_iter->program_data[value_start_address + (wordsize_bytes - i)] = value >> ((i - 1) * 8);
					}
				}
			}
		}
	}

	// Now, we can write all of the program data to a vector of bytes
	std::vector<uint8_t> sml_data;

	for (std::vector<SinObjectFile>::iterator file_iter = this->object_files.begin(); file_iter != this->object_files.end(); file_iter++) {
		// iterate through the program data and push the bytes into sml_data
		for (std::vector<uint8_t>::iterator byte_iter = file_iter->program_data.begin(); byte_iter != file_iter->program_data.end(); byte_iter++) {
			sml_data.push_back(*byte_iter);
		}
		// create a vector of uint8_ts from our file to contain all of the .data information
		std::vector<uint8_t> data_section;
		// iterate through the .data section of the file
		for (std::list<std::tuple<std::string, size_t, std::vector<uint8_t>>>::iterator data_table_iter = file_iter->data_table.begin(); data_table_iter != file_iter->data_table.end(); data_table_iter++) {
			// iterate through the actual data bytes in the section
			for (std::vector<uint8_t>::iterator data_iter = std::get<2>(*data_table_iter).begin(); data_iter != std::get<2>(*data_table_iter).end(); data_iter++) {
				// push those bytes onto data_section
				data_section.push_back(*data_iter);
			}
		}
		// now, push all of the information from data_section to sml_data; the data section should come at the end of each object
		for (std::vector<uint8_t>::iterator data_section_iter = data_section.begin(); data_section_iter != data_section.end(); data_section_iter++) {
			sml_data.push_back(*data_section_iter);
		}
	}


	/************************************************************
	********************	SML FILE		*********************
	************************************************************/


	// Finally, create a .sml file of our linked program
	std::ofstream sml_file;
	sml_file.open(file_name + ".sml", std::ios::out | std::ios::binary);	// TODO: get a final program name

	BinaryIO::writeU8(sml_file, this->object_files[0]._wordsize);	// TODO: get a better wordsize deciding algorithm

	BinaryIO::writeU32(sml_file, sml_data.size());

	// write the byte to the file
	for (std::vector<uint8_t>::iterator it = sml_data.begin(); it != sml_data.end(); it++) {
		BinaryIO::writeU8(sml_file, *it);
	}

	sml_file.close();
}


Linker::Linker() {
	this->_start_offset = 0;	// default to 0
	this->_wordsize = 16;	// default to 16 bit words
	this->_rs_start = _RS_START;	// default to "_RS_START" as defined in "VMMemoryMap.h" 
}


Linker::Linker(std::vector<SinObjectFile> object_files) : object_files(object_files)
{
	this->_start_offset = 0;	// default to 0
	this->_wordsize = 16;	// default to 16 bit words
	this->_rs_start = _RS_START;	// default to "_RS_START" as defined in "VMMemoryMap.h" 

	this->get_metadata();	// get our metadata so we can form the sml file; this may overwrite the initial values established above
}


Linker::~Linker()
{
}
