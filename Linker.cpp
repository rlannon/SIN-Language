#include "Linker.h"



void Linker::get_metadata() {
	// iterate through the .sinc files to get the wordsize
	// if they don't agree, throw an exception

	// we also need to get the VM version from each file so the start address can be set properly
	// they should all be the same, but we could devise an algorithm that will work for different versions or memory configurations
	
	// first, we need to get the wordsize and version of the 0th file; this will be our reference
	uint8_t wordsize_compare = this->object_files[0]._wordsize;
	uint8_t version_compare = this->object_files[0]._sinvm_version;

	for (std::vector<SinObjectFile>::iterator file_iter = this->object_files.begin(); file_iter != this->object_files.end(); file_iter++) {
		if (file_iter->_wordsize != wordsize_compare) {
			throw std::exception("**** Word sizes in all object files must match.");
		}

		if (file_iter->_sinvm_version != version_compare) {
			throw std::exception("**** SINVM Version must be the same between all object files.");
		}
	}

	// now that we know everything matches, set the start address and word size to be used by the linker in creating a .sml file
	this->_wordsize = wordsize_compare;

	// the offset is different between memory configurations
	if (version_compare == 1) {
		this->_start_offset = 0x2600;
	}
	else {
		throw std::exception("**** Currently, only SINVM version 1 is supported (it is the highest version!).");
	}

	// TODO: devise better algorithm for object file validation
}


// create a .sml file from all of our objects
void Linker::create_sml_file(std::string file_name) {

	// first, update all the offsets
	size_t current_offset = this->_start_offset;	// current offset starts at the start address of VM memory

	// iterate over the whole vector
	for (std::vector<SinObjectFile>::iterator file_iter = this->object_files.begin(); file_iter != this->object_files.end(); file_iter++) {
		// _text_start holds the start address, from which all control flow addresses in the file are offset
		file_iter->_text_start = current_offset;

		// add this offset to every "value" field in the symbol table where the symbol class is "D", "C", or "R"
		for (std::list<std::tuple<std::string, int, std::string>>::iterator symbol_iter = file_iter->symbol_table.begin(); symbol_iter != file_iter->symbol_table.end(); symbol_iter++) {
			
			// if our symbol is defined
			if ((std::get<2>(*symbol_iter) == "D") || (std::get<2>(*symbol_iter) == "C") || (std::get<2>(*symbol_iter) == "R")) {
				// update the value stored in the symbol table to be the current value plus the offset
				std::get<1>(*symbol_iter) = std::get<1>(*symbol_iter) + current_offset;
			}

			/*
			if the symbol table is "C" or "R", we also need to add the offsets from both:
				a) the size of the .text section
				b) the offset from the /end/ of the .text section
			*/

			if (std::get<2>(*symbol_iter) == "C") {
				// iterate through the file's constants list to find the constant's data
				std::list<std::tuple<std::string, int, std::vector<uint8_t>>>::iterator data_iter = file_iter->data_table.begin();
				bool found_constant = false;
				while (!found_constant && (data_iter != file_iter->data_table.end())) {
					if (std::get<0>(*data_iter) == std::get<0>(*symbol_iter)) {
						found_constant = true;
					}
					else {
						data_iter++;
					}
				}
				if (found_constant) {
					// get the total offset
					int offset_from_text_end = std::get<1>(*data_iter);

					// add the total offset to <1>(symbol_iter), which is the offset for the current symbol in the symbol table; this symbol is class "C"
					std::get<1>(*symbol_iter) += offset_from_text_end;
				}
				else {
					throw std::exception("Could not find the constant specified in the constants table!");
				}
			}

			// otherwise
			else {
				continue;	// continue on; "U" class will be updated later
			}
		}

		// update current_offset to add the size of the program just added
		current_offset += file_iter->program_data.size();

		// we then need to update current_offset to add the size of the .data section, as these will get added onto the program sections at the end
		size_t data_section_offset = 0;

		for (std::list<std::tuple<std::string, int, std::vector<uint8_t>>>::iterator data_it = file_iter->data_table.begin(); data_it != file_iter->data_table.end(); data_it++) {
			size_t constant_size = std::get<2>(*data_it).size();
			data_section_offset += constant_size;
		}

		current_offset += data_section_offset;
	}

	// now that our initial offsets and defined label offsets have been adjusted, we can construct the master symbol table

	// first, create the vector to hold the table
	std::vector<std::tuple<std::string, int, std::string>> master_symbol_table;
	// iterate through our object files' symbol tables to add to the master table

	for (std::vector<SinObjectFile>::iterator file_iter = this->object_files.begin(); file_iter != this->object_files.end(); file_iter++) {
		// only add defined references to the table
		// find them by iterating through each symbol table and adding the defined symbols to the master table
		for (std::list<std::tuple<std::string, int, std::string>>::iterator symbol_iter = file_iter->symbol_table.begin(); symbol_iter != file_iter->symbol_table.end(); symbol_iter++) {
			if ((std::get<2>(*symbol_iter) == "D") || (std::get<2>(*symbol_iter) == "C") || (std::get<2>(*symbol_iter) == "R")) {
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
		for (std::list<std::tuple<std::string, int, std::string>>::iterator symbol_iter = file_iter->symbol_table.begin(); symbol_iter != file_iter->symbol_table.end(); symbol_iter++) {
			// if it is a constant, the symbol table should now have the proper address
			/*if (std::get<2>(*symbol_iter) == "C") {
				// TODO: resolve constants references

			}
			// if it is a macro (using rs directive), look in the data from the .bss section for a resolution
			else if (std::get<2>(*symbol_iter) == "R") {
				// TODO: .bss data...
			}*/
			// if it's undefined
			if ((std::get<2>(*symbol_iter) == "U") || (std::get<2>(*symbol_iter) == "C") || (std::get<2>(*symbol_iter) == "R")) {
				// iterate through the master table and find the symbol referenced
				std::vector<std::tuple<std::string, int, std::string>>::iterator master_table_iter = master_symbol_table.begin();
				bool found = false;
				while ((master_table_iter != master_symbol_table.end()) && !found) {
					// if the symbol names are the same
					if (std::get<0>(*master_table_iter) == std::get<0>(*symbol_iter)) {
						// copy over the value from the master table iterator to our local symbol table
						std::get<1>(*symbol_iter) = std::get<1>(*master_table_iter);

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
					throw std::exception(("**** Symbol table error: Could not find '" + std::get<0>(*symbol_iter) + "' in symbol table!").c_str());
				}
			}
			else {
				continue;	// skip defined references
			}
		}

		// now, go through each file individually and update all values pointed to in the relocations table
		for (std::vector<SinObjectFile>::iterator file_iter = this->object_files.begin(); file_iter != this->object_files.end(); file_iter++) {
			// iterate through the relocation table
			for (std::list<std::tuple<std::string, int>>::iterator relocation_iter = file_iter->relocation_table.begin(); relocation_iter != file_iter->relocation_table.end(); relocation_iter++) {
				// if the symbol is "_NONE", we add the file's offset to the value pointed to by the relocator
				if (std::get<0>(*relocation_iter) == "_NONE") {
					// the start address is the value in the relocation iter
					size_t value_start_address = std::get<1>(*relocation_iter);

					int data = 0;
					size_t wordsize_bytes = (size_t)file_iter->_wordsize / 8;

					// the value to write is located at value_start_address
					for (int i = 0; i < (wordsize_bytes - 1); i++) {
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
					std::vector<std::tuple<std::string, int, std::string>>::iterator master_table_iter = master_symbol_table.begin();
					bool found = false;
					int value = 0;
					while ((master_table_iter != master_symbol_table.end()) && !found) {
						// if the names are the same
						if (std::get<0>(*master_table_iter) == std::get<0>(*relocation_iter)) {
							// get the value
							value = std::get<1>(*master_table_iter);
							// set the found variable and exit the loop
							found = true;
						}
						else {
							master_table_iter++;
						}
					}

					// if we didn't find it, throw an error
					if (!found) {
						throw std::exception(("**** Relocation error: Could not find '" + std::get<0>(*relocation_iter) + "' in symbol table!").c_str());
					}

					// finally, write "value" to the relocation table
					// get the start address and the number of bytes in our wordsize
					size_t value_start_address = std::get<1>(*relocation_iter);
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
		for (std::list<std::tuple<std::string, int, std::vector<uint8_t>>>::iterator data_table_iter = file_iter->data_table.begin(); data_table_iter != file_iter->data_table.end(); data_table_iter++) {
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

	writeU8(sml_file, this->object_files[0]._wordsize);	// TODO: get a better wordsize deciding algorithm

	writeU32(sml_file, sml_data.size());

	// write the byte to the file
	for (std::vector<uint8_t>::iterator it = sml_data.begin(); it != sml_data.end(); it++) {
		writeU8(sml_file, *it);
	}

	sml_file.close();
}

Linker::Linker(std::vector<SinObjectFile> object_files)
{
	this->object_files = object_files;	// initialize our object file vector
	this->get_metadata();	// get our metadata so we can form the sml file
}


Linker::~Linker()
{
}
