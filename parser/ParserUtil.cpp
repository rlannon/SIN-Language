/*

SIN Toolchain
ParserUtil.cpp
Copyright 2019 Riley Lannon

Contains the implementations of various utility functions for the parser.

*/

#include "Parser.h"

SymbolQuality Parser::get_quality(lexeme quality_token)
{
	// Given a lexeme containing a quality, returns the appropriate member from SymbolQuality

	const SymbolQuality qualities[5] = { CONSTANT, STATIC, DYNAMIC, SIGNED, UNSIGNED };
	const std::string quality_string[5] = { "const", "static", "dynamic", "signed", "unsigned" };
	SymbolQuality to_return = NO_QUALITY;

	// ensure the token is a kwd
	if (quality_token.type == "kwd") {
		// iterate through the quality_string array and compare quality_token.value with it to find the index of the appropriate quality
		bool found = false;
		size_t index = 0;
		while (index < num_qualities && !found) {
			if (quality_token.value == quality_string[index]) {
				found = true;
			}
			else {
				index++;
			}
		}

		if (found) {
			to_return = qualities[index];
		}
		else {
			throw ParserException("Invalid qualifier", 0, quality_token.line_number);
		}
	}
	else {
		throw ParserException("Invalid qualifier", 0, quality_token.line_number);
	}

	return to_return;
}
