#include "Lexer.h"


// implement "lexeme" data type
// allow direct comparisons of two lexemes using the == operator
bool lexeme::operator==(const lexeme& b) {
	// note: we don't care about the line number; we only want to know if they have the same type/value pair
	return ((this->type == b.type) && (this->value == b.value));
}

lexeme::lexeme() {
	this->line_number = 0;	// initialize to 0 by default
}

lexeme::lexeme(std::string type, std::string value, int line_number) : type(type), value(value), line_number(line_number) {
}


// keywords is an alphabetized list of the keywords in SIN
// it must be alphabetized in order to use the find algorithm from the standard library
const std::vector<std::string> Lexer::keywords{ "alloc", "and", "asm", "bool", "const", "def", "else", "float", "if", "include", "int", "let", "or", "ptr", "raw", "return", "string", "void", "while", "xor"};

// Our regular expressions
const std::string Lexer::punc_exp = "[\\.',:;\\[\\]\\{\\}\\(\\)]";	// expression for punctuation
const std::string Lexer::op_exp = "[\\+\\-\\*/%=\\&\\|\\^<>\\$\\?!@#]";	// expression for operations
const std::string Lexer::id_exp = "[_0-9a-zA-Z]";	// expression for interior id letters
const std::string Lexer::bool_exp = "[(True)|(False)]";


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
		
		// increment the line number if we hit a newline character
		if (ch == '\n') {
			this->current_line += 1;
		}

		return ch;
	}
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
		std::cerr << "REGEX ERROR:" << std::endl << e.what() << std::endl;	// because these functions are all static, we cannot use current_line to display the error unless it is added as a parameter for all of the static functions; do they really need to be static?
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

bool Lexer::is_boolean(std::string candidate) {
	if (candidate == "true" || candidate == "True" || candidate == "False" || candidate == "false") {
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
		std::cout << "Note from Lexer: end of file reached" << std::endl;
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


/*

Reads the next character in the stream to determine what to do next.

*/

lexeme Lexer::read_next() {
	std::string type = "";
	std::string value = "";
	lexeme next_lexeme;

    this->read_while(&this->is_whitespace);	// continue reading through any whitespace

	char ch = this->peek();	// peek to see if we are still within the file

	if (this->stream->eof()) {
		next_lexeme = lexeme("", "", NULL);	// return an empty tuple if we have reached the end of the file
		this->exit_flag = true;	// set our exit flag
		return next_lexeme;
	}

	if (ch == '/') {	// if we have a slash, we may have a comment
		ch = this->next();	// advance one character

		if (this->peek() == '/') {	// if the next character is a slash, it's a line comment
			bool is_comment = true;

			while (is_comment) {
				this->next();	// eat the comment character
				this->read_while(&this->is_not_newline);	// skip characters until we hit a newline character
				this->next();	// eat the newline character

				// if we have whitespace (e.g., a tab), skip it
				this->read_while(&this->is_whitespace);

				// get the next character
				ch = this->peek();

				if (this->stream->eof() || this->eof()) {	// check to make sure we haven't gone past the end of the file
					next_lexeme = lexeme("", "", NULL);	// if we are, set the exit flag return an empty tuple
					this->exit_flag = true;	// set our exit flag
					return next_lexeme;	// return the empty lexeme
				}
				else if (ch == '/') {	// if not, check to see if we have another comment
					ch = this->next();	// eat the potential comment character

					// check to see if the next character is a slash; if so, we have another comment
					if (this->peek() == '/') {
						is_comment = true;
					}
					// if it is not a comment
					else {
						// set is_comment to false to terminate the loop
						is_comment = false;
						// use "unget" so that the position is in the correct place and "peek" reveals a slash
						this->stream->unget();
						ch = this->peek();
					}
				}
				else {
					is_comment = false;
				}
			}
		}
		// if the next character is not '/', just treat it as an op_char
		else {
			// use "unget" to move back one place so "peek" reveals a slash
			this->stream->unget();
			ch = this->peek();
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
			else if (this->is_boolean(value)) {
				type = "bool";
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
			char next_ch = this->peek();
			if (next_ch != '*') {
				value = this->read_while(&this->is_op_char);
			}
			else {
				value = next_ch;
				this->next();
			}
		}
		else if (ch == '\n') {	// if we encounter a newline character
			this->peek();
			if (this->stream->eof()) {
				next_lexeme = lexeme("", "", NULL);	// if we have reached the end of file, set the exit flag and return an empty tuple
				this->exit_flag = true;
				return next_lexeme;
			}
			else {
				this->next();	// otherwise, continue by getting the next character
			}
		}
		else {	// if the character in the file is not recognized, print an error message and quit lexing
			throw LexerException("Unrecognized character!", position, ch);
			this->exit_flag = true;
		}

		next_lexeme = lexeme(type, value, this->current_line);	// create our lexeme with our information
		return next_lexeme;

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
	this->current_lexeme = this->read_next();
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
	return os << "{ \"" << this->current_lexeme.type << "\" : \"" << this->current_lexeme.value << "\" }";
}

// Constructor and Destructor

Lexer::Lexer(std::istream& input)
{
	Lexer::stream = &input;
	Lexer::position = 0;
	Lexer::current_line = 1;
	Lexer::exit_flag = false;
}


Lexer::~Lexer()
{
}

std::ostream& operator<<(std::ostream& os, const Lexer& lexer) {
	return lexer.write(os);
}



LexerException::LexerException(const std::string& err_message, const int& err_position, const char& ch) : message_(err_message), position_(err_position), ch_(ch) {

}

const char* LexerException::what() const {
	return LexerException::message_.c_str();
}

char LexerException::get_char() {
	return ch_;
}

int LexerException::get_pos() {
	return position_;
}
