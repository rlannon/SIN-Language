#include "SinObjectFile.h"

// TODO: return the symbol table and relocation table as well...

void SinObjectFile::load_sinc_file(std::istream & file)
{
	// first, read the magic number
	char header[4];
	char * buffer = &header[0];
	file.read(buffer, 4);

	// if our magic number is valid
	if (header[0, 1, 2, 3] == *"s", "i", "n", "C") {

		// get the word size
		this->_wordsize = readU8(file);

		// get the endianness
		uint8_t _text_end = readU8(file);
		uint8_t _sinc_end = readU8(file);

		// get the file version
		uint8_t _file_version = readU8(file);	// to be used by the file reader -- currently not needed by linker
		this->_sinvm_version = readU8(file);

		// get the entry point
		this->_text_start = readU16(file);

		// TODO: validate the file using the file header data


		if (_file_version == 2) {
			
			// get the size of the program
			size_t _prog_size = (size_t)readU32(file);


			// symbol data:

			// number of elements in symbol table
			size_t _st_size = (size_t)readU32(file);

			// iterate for each item in _st_size
			for (size_t i = 0; i < _st_size; i++) {
				// get the value of the symbol
				int _sy_val = (int)readU16(file);
				uint8_t _class = readU8(file);
				std::string _sy_name = readString(file);	// _sn_len is automatically retrieved from BianryIO::readString

				// get the symbol class as a string from the uint8_t we read in
				std::string symbol_class;
				if (_class == 1) {
					symbol_class = "U";
				}
				else if (_class == 2) {
					symbol_class = "D";
				}
				else if (_class == 3) {
					symbol_class = "C";
				}
				else if (_class == 4) {
					symbol_class = "R";
				}
				else if (_class == 5) {
					symbol_class = "M";
				}
				else {
					throw std::exception("**** Error: bad number in symbol class specifier.");
				}

				// create the tuple and push it onto "symbol_table"
				this->symbol_table.push_back(std::make_tuple(_sy_name, _sy_val, symbol_class));
			}


			// relocation data:

			// number of elements in relocation table
			size_t _rt_size = (size_t)readU32(file);

			// iterate for each item in _rt_size
			for (size_t i = 0; i < _rt_size; i++) {
				// get the address where the symbol occurs in the code
				int _addr = (int)readU16(file);
				std::string _sy_name = readString(file);	// _rs_len is automatically retrieved by BinaryIO::readString

				// create the tuple and push it onto "relocation_table"
				this->relocation_table.push_back(std::make_tuple(_sy_name, _addr));
			}


			// .text:

			// iterate for each byte in _prog_size
			for (size_t i = 0; i < _prog_size; i++) {
				// read a byte from the file and push it onto program_data
				this->program_data.push_back(readU8(file));
			}

			// .data:

			// read the number of constants to read
			size_t num_data_entries = (size_t)readU32(file);

			// the data section immediately follows the program data
			size_t data_position_offset = this->program_data.size();

			for (size_t i = 0; i < num_data_entries; i++) {
				size_t num_bytes = readU16(file);

				std::string macro_name = readString(file);
				std::vector<uint8_t> data_bytes;

				for (size_t j = 0; j < num_bytes; j++) {
					data_bytes.push_back(readU8(file));
				}

				// add the data to our data table
				this->data_table.push_back(std::make_tuple(macro_name, data_position_offset, data_bytes));

				// increment our data_position_offset by the number of data_bytes we have
				data_position_offset += data_bytes.size();
			}

			// .bss:
		}
		// cannot handle any other versions right now because they don't exist yet
		else {
			throw std::exception("Other .sinc file versions not supported at this time.");
		}
	}
	else {
		throw std::exception("Invalid magic number in file header.");
	}
}



uint8_t SinObjectFile::get_wordsize() {
	return this->_wordsize;
}

std::vector<uint8_t> SinObjectFile::get_program_data() {
	return this->program_data;
}


// Class constructor and destructor

SinObjectFile::SinObjectFile() {

}

SinObjectFile::SinObjectFile(std::istream& file) {
	this->load_sinc_file(file);
}

SinObjectFile::~SinObjectFile() {

}
