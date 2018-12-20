#pragma once

#include <tuple>
#include <vector>
#include <iostream>

#include "BinaryIO.h"

// load our file (if there is one) and populate our instructions list
std::tuple<uint8_t, std::vector<uint8_t>> load_sinc_file(std::istream& file);
