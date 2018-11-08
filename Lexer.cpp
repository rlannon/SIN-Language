#include "Lexer.h"


// keywords is an alphabetized list of the keywords in SIN
// it must be alphabetized in order to use the find algorithm from the standard library
const std::vector<std::string> Lexer::keywords{ "alloc", "and", "bool", "else", "float", "if", "int", "let", "or", "print", "string", "while", "xor"};

// Our regular expressions
const std::string Lexer::punc_exp = "[\\.',;\\[\\]\\{\\}\\(\\)]";	// expression for punctuation
const std::string Lexer::op_exp = "[\\+\\-\\*/%=\\&\\|\\^<>\\$\\?!@]";	// expression for operations
const std::string Lexer::id_exp = "[_0-9a-zA-Z]";	// expression for interior id letters


// Our stream access and test functions

bool Lexer::eof() {
	char eof_test = this->stream->peek();
	if (eof_test == EOF) {
		return true;
	}
	else {
		return false;
	}
}

char Lexer::peek() {
	if (!this->eof()) {
		char ch = this->stream->peek();
		return ch;
	}
}

char Lexer::next() {
	if (!this->eof()) {
		char ch = this->stream->get();
		return ch;
	}
}

void Lexer::croak(char token, int position) {
	// executes if there is a tokenizer error
	std::cout << "Could not understand character/token \"" << token << "\" at position " << this->stream->tellg() << "!" << std::endl;
	std::cout << "Aborting lexer." << std::endl;
}



/*

Our equivalency functions.
These are used to test whether a character is of a certain type.

*/

bool Lexer::match_character(char ch, std::string expression) {
	try {
		return (std::regex_match(&ch, std::regex(expression)));
	}
	catch (const std::regex_error &e) {
		std::cout << "REGEX ERROR:" << std::endl << e.what() << std::endl;
	}
}

bool Lexer::is_whitespace(char ch) {
	if (match_character(ch, "[ \n\t]")) {
		return true;
	}
	else {
		return false;
	}
}

bool Lexer::is_newline(char ch) {
	if (match_character(ch, "(\n)")) {
		return true;
	}
	else {
		return false;
	}
}

bool Lexer::is_not_newline(char ch) {
	if (ch != '\n') {
		return true;
	}
	else {
		return false;
	}
}

bool Lexer::is_digit(char ch) {
	if (match_character(ch, "[0-9]")) {
		return true;
	}
	else {
		return false;
	}
}

bool Lexer::is_letter(char ch) {
	if (match_character(ch, "[a-zA-Z]")) {
		return true;
	}
	else {
		return false;
	}
}

bool Lexer::is_number(char ch) {
	if (match_character(ch, "[0-9\\.]")) {
		return true;
	}
	else {
		return false;
	}
}

bool Lexer::is_id_start(char ch) {

	/*  Returns true if the character is the start of an ID; that is, if it starts with a letter or an underscore  */

	if (match_character(ch, "[_a-zA-Z]")) {
		return true;
	}
	else {
		return false;
	}
}

bool Lexer::is_id(char ch) {
	/*  Returns true if the character is a valid id character  */

	if (match_character(ch, id_exp)) {
		return true;
	}
	else {
		return false;
	}
}

bool Lexer::is_punc(char ch) {
	if (match_character(ch, punc_exp)) {
		return true;
	}
	else {
		return false;
	}
}

bool Lexer::is_op_char(char ch) {
	if (match_character(ch, op_exp)) {
		return true;
	}
	else {
		return false;
	}
}

bool Lexer::is_keyword(std::string candidate) {
	if (std::binary_search(keywords.begin(), keywords.end(), candidate)) {
		return true;
	}
	else {
		return false;
	}
}

/*

Our read functions.
These will read out data in the stream and return it as a string with proper formatting.

*/

std::string Lexer::read_while(bool (*predicate)(char)) {
	std::string msg = "";

	this->peek();
	if (this->stream->eof()) {
		std::cout << "EOF in 'read while'" << std::endl;
		return msg;
	}

	while (!this->eof() && predicate(this->peek())) {
		char next_char = this->peek();
		if (!this->eof()) {
			msg += this->next();
		}
	}

	return msg;
}

std::tuple<std::string, std::string> Lexer::read_next() {
	/*
		Reads the next character in the stream to determine what to do next.
	*/
	std::string type = "";
	std::string value = "";
	std::tuple<std::string, std::string> lexeme;

    this->read_while(&this->is_whitespace);	// continue reading through any whitespace

	char ch = this->peek();	// peek to see if we are still within the file

	if (this->stream->eof()) {
		lexeme = std::make_tuple(NULL, NULL);	// return an empty tuple if we have reached the end of the file
		this->exit_flag = true;	// set our exit flag
		return lexeme;
	}

	if (ch == '#') {	// if we have a comment, skip down to the next line
		bool is_comment = true;

		while (is_comment) {
			this->next();	// eat the comment character
			this->read_while(&this->is_not_newline);
			this->next();	// eat the newline character

			ch = this->peek();

			if (this->stream->eof()) {	// check to make sure we haven't gone past the end of the file
				lexeme = std::make_tuple(NULL, NULL);	// if we are, set the exit flag return an empty tuple
				this->exit_flag = true;
				return lexeme;
			}
			else if (ch == '#') {	// if not, check to see if we have another comment
				is_comment = true;
			}
			else {
				is_comment = false;
			}
		}
	}

	// If ch is not the end of the file and it is also not null
	if (ch != EOF && ch != NULL) {
		// test our various data types
		if (ch == '"') {
			type = "string";
			value = this->read_string();
		}
		else if (this->is_id_start(ch)) {
			value = this->read_while(&this->is_id);
			if (this->is_keyword(value)) {
				type = "kwd";
			}
			else {
				type = "ident";
			}
		}
		else if (this->is_digit(ch)) {
			value = this->read_while(&this->is_number);	// get the number

			// Now we must test whether the number we got is an int or a float
			if (std::regex_search(value, std::regex("\\."))) {
				type = "float";
			}
			else {
				type = "int";
			}
		}
		else if (this->is_punc(ch)) {
			type = "punc";
			value = this->next();	// we only want to read one punctuation mark at a time, so do not use "read_while"; they are to be kept separate
		}
		else if (this->is_op_char(ch)) {
			type = "op_char";
			value = this->read_while(&this->is_op_char);
		}
		else if (ch == '\n') {	// if we encounter a newline character
			this->peek();
			if (this->stream->eof()) {
				lexeme = std::make_tuple(NULL, NULL);	// if we have reached the end of file, set the exit flag and return an empty tuple
				this->exit_flag = true;
				return lexeme;
			}
			else {
				this->next();	// otherwise, continue by getting the next character
			}
		}
		else {	// if the character in the file is not recognized, print an error message and quit lexing
			croak(ch, position);
			this->exit_flag = true;
		}

	lexeme = std::make_tuple(type, value);	// create our lexeme with out (type, value) tuple
	return lexeme;

	}
	else if (ch == NULL) {	// if there is a NULL character
		std::cout << ch << "   (NULL)" << std::endl;
		std::cout << "ch == NULL; done." << std::endl;
		this->exit_flag = true;
	}
	else if (ch == EOF) {	// if the end of file was reached
		std::cout << ch << "   (EOF)" << std::endl;
		std::cout << "end of file reached." << std::endl;
		this->exit_flag = true;
	}
}

void Lexer::read_lexeme() {
	this->lexeme = this->read_next();
}

std::string Lexer::read_string() {
	std::string str = "";	// start with an empty string for the message
	this->stream->get();	// skip the initial quote in the string

	bool escaped = false;	// initialized our "escaped" identifier to false
	bool string_done = false;

	while (!this->eof() && !string_done) {
		char ch = this->stream->get();	// get the character

		if (escaped) {	// if we have escaped the character
			str += ch;
			escaped = false;
		}
		else if (ch == '\\') {	// if we have not escaped the character and it's a backslash
			escaped = true;	// escape the next one
		}
		else if (!escaped && ch == '"') {	// if we have not escaped it and the character is a double quote
			string_done = true;	// we are done with the string
		}
		else {	// otherwise, if we haven't escaped it and we didn't need to,
			str += ch;	// add it to the string
		}
	}

	return str;
}

std::string Lexer::read_ident() {
	std::string identifier = "";

	identifier += this->read_while(&this->is_id);

	return identifier;
}

// A function to check whether our exit flag is set or not
bool Lexer::exit_flag_is_set() {
	return exit_flag;
}

// allow a lexeme to be written to an ostream

std::ostream& Lexer::write(std::ostream& os) const {
	return os << "{ \"" << std::get<0>(this->lexeme) << "\" : \"" << std::get<1>(this->lexeme) << "\" }";
}

// Constructor and Destructor

Lexer::Lexer(std::ifstream* input)
{
	Lexer::stream = input;
	Lexer::position = 0;
	Lexer::exit_flag = false;
}


Lexer::~Lexer()
{
}

std::ostream& operator<<(std::ostream& os, const Lexer& lexer) {
	return lexer.write(os);
}

