#include "LoadSINC.h"

std::tuple<uint8_t, std::vector<uint8_t>> load_sinc_file(std::istream & file)
{
	// a vector to hold our program data
	std::vector<uint8_t> program_data;

	// first, read the magic number
	char header[4];
	char * buffer = &header[0];

	file.read(buffer, 4);

	// if our magic number is valid
	if (header[0, 1, 2, 3] == *"s", "i", "n", "C") {
		// must have the correct version
		uint8_t version = readU8(file);

		if (version == 1) {
			// get the word size
			uint8_t word_size = readU8(file);
			// get the program size
			uint16_t program_size = readU16(file);

			// use program_size to read the proper number of bytes
			int i = 0;
			while (!file.eof() && i < program_size) {
				program_data.push_back(readU8(file));
				i++;
			}

			// return word size and the program data vector
			return std::make_tuple(word_size, program_data);
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
