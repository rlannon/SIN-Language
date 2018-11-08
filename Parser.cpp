#include "Parser.h"

typedef std::tuple<std::string, std::string> lexeme;
typedef std::tuple<std::string, std::string> ASTNode;

// Define our symbols and their precedences as a vector of tuples
const std::vector<std::tuple<std::string, int>> Parser::precedence{ std::make_tuple("=", 1), std::make_tuple("&&", 2), std::make_tuple("||", 3), \
	std::make_tuple("<", 4), std::make_tuple(">", 7), std::make_tuple("<", 7), std::make_tuple(">=", 7), std::make_tuple("<=", 7), std::make_tuple("==", 7),\
	std::make_tuple("!=", 7), std::make_tuple("+", 10), std::make_tuple("-", 10), std::make_tuple("*", 20), std::make_tuple("/", 20), std::make_tuple("%", 20),\
	std::make_tuple("(", 30), std::make_tuple(")", 30)} ;

// Iterate through the vector and find the tuple that matches our symbol; if found, return its precedence; if not, return -1
int Parser::get_precedence(std::string symbol) {
	std::vector<std::tuple<std::string, int>>::const_iterator it = Parser::precedence.begin();
	bool match = false;
	int precedence;

	while(it != Parser::precedence.end()) {
		if (std::get<0>(*it) == symbol && !match) {
			match = true;
			precedence = std::get<1>(*it);
		}
		else {
			it++;
		}
	}

	if (match) {
		return precedence;
	}
	else {
		std::cerr << "No such operator \"" << symbol << "\" found in operator precedence vector!" << std::endl;
		return -1;	// -1 will be our error value
	}
}

// Tells us whether we have run out of tokens
bool Parser::is_at_end() {
	if (this->current_token >= this->num_tokens) {	// the last element is list.size() - 1;  if we are at list.size(), we have gone over
		return true;
	}
	else {
		return false;
	}
}

// peek to the next position
lexeme Parser::peek() {
	if (current_token + 1 <= this->tokens.size()) {
		return this->tokens[this->current_token + 1];
	}
	else {
		this->error("No more lexemes to parse!", 1);
	}
}

lexeme Parser::next() {
//	std::cout << std::endl << this->current_token << std::endl << std::get<0>(this->tokens[this->current_token]) << std::endl;
	this->current_token += 1;
	if (current_token <= this->tokens.size()) {
		return this->tokens[this->current_token];
	}
	else {
		this->error("No more lexemes to parse!", 1);
	}
}

lexeme Parser::current() {
	return this->tokens[this->current_token];
}

lexeme Parser::previous() {
	return this->tokens[this->current_token - 1];
}

lexeme Parser::back() {
	this->current_token -= 1;
	return this->tokens[this->current_token];
}

void Parser::error(std::string message, int code) {
	std::cerr << std::endl << "ERROR:" << "\n\t" << message << std::endl;
	std::cerr << "\tError occurred at position: " << this->current_token << std::endl << std::endl;
	throw code;
}

// Skip a punctuation mark
void Parser::skip_punc(char punc) {
	if (std::get<0>(this->current()) == "punc") {
		if (std::get<1>(this->current()) == &punc) {
			this->current_token += 1;
			return;
		}
		else {
			return;
		}
	}
	else {
		return;
	}
}


/*********************	GET TYPE FROM STRING	*********************/

Type Parser::get_type(std::string candidate) {
	// if it can, this function gets the proper type of an input string
	// an array of the valid types as strings
	std::string string_types[] = { "int", "float", "string", "bool" };
	Type _types[] = { INT, FLOAT, STRING, BOOL };

	// for test our candidate against each item in the array of string_types; if we have a match, return the Type at the same position
	for (int i = 0; i < 4; i++) {
		if (candidate == string_types[i]) {
			// if we have a match, return it
			return _types[i];
		}
		else {
			continue;
		}
	}

	// hitting this portion of the code means we have not yet returned
	// if we cannot find a match, we return the type 'NONE'
	this->error("No proper match in 'Type' for variable type '" + candidate + "'!", 211);
	return NONE;
}



/*********************	PARSING STATEMENTS	*********************/



void Parser::parse_top() {
	// Used to parse a whole syntax tree
	StatementBlock prog;	// for our statements
	int index = 0;

	std::cout << "Parsing 'prog'..." << std::endl;
	while (!this->is_at_end() && !this->quit) {
		this->skip_punc(';');
		this->skip_punc('\n');

		prog.StatementsList.push_back(this->parse_statement());	// push a Statement into the vector
		std::cout << prog.StatementsList[index]->get_type() << std::endl;

		if (prog.StatementsList[index]->get_type() == "assign") {
			Assignment* child_ptr = dynamic_cast<Assignment*>(prog.StatementsList[index].get());	// get "Statement" from vector back as type "Assignment"
			std::cout << "LValue name: " << child_ptr->getLvalueName() << std::endl << "statement type: " << child_ptr->get_type() << std::endl;
			std::cout << "RValue type: " << child_ptr->getRValueType() << std::endl;
		}
		else if (prog.StatementsList[index]->get_type() == "alloc") {
			Allocation* child_ptr = dynamic_cast<Allocation*>(prog.StatementsList[index].get());
			std::cout << "Allocated variable:" << std::endl;
			std::cout << "\tType allocated: " << child_ptr->getVarType() << std::endl;
			std::cout << "\tVariable name: " << child_ptr->getVarName() << std::endl;
		}

		std::cout << "\n********************" << std::endl;

		index++;
		this->next();
	}
	std::cout << "Done parsing 'prog'" << std::endl;
}



std::unique_ptr<Statement> Parser::parse_statement() {
	std::cout << "Parsing statement..." << std::endl;
	lexeme next_token = this->current();
	if (std::get<0>(next_token) == "kwd") {
		std::cout << "Parsing kwd..." << std::endl;
		if (std::get<1>(next_token) == "alloc") {
			std::cout << "Parsing declaration/allocation..." << std::endl;
			std::tuple<Type, std::string> allocate = this->parse_allocation();
			return std::make_unique<Allocation>(std::get<0>(allocate), std::get<1>(allocate));
		}
		else if (std::get<1>(next_token) == "let") {
			std::cout << "Parsing assignment..." << std::endl;
			std::tuple<LValue, std::shared_ptr<Expression>> assign = this->parse_assignment();
			return std::make_unique<Assignment>(std::get<0>(assign), std::get<1>(assign));	// get our Assignment
		}
		else if (std::get<1>(next_token) == "if") {
			std::cout << "Parsing conditional..." << std::endl;

			// TODO: write ITE parser

		}
	}
	else if (std::get<0>(next_token) == "op_char") {
		if (std::get<1>(next_token) == "@") {
			std::cout << "Parsing function call..." << std::endl;

			// TODO: Write function call parser

		}
	}
	else if (std::get<0>(next_token) == "punc") {
		if (std::get<1>(next_token) == ";") {
			this->next();
		}
	}
}


std::tuple<Type, std::string> Parser::parse_allocation() {
	// placeholder code...
	Type _type = NONE;
	std::string _value = "";

	// parse the allocation
	// the token after 'alloc' must be a keyword
	if (std::get<0>(this->peek()) == "kwd") {
		// if good, get the nex token
		lexeme var_type = this->next();
		std::string type_s = std::get<1>(var_type);
		// get the proper type
		_type = this->get_type(type_s);

		// the next token must be an identifier
		if (std::get<0>(this->peek()) == "ident") {
			lexeme var_name = this->next();
			_value = std::get<1>(var_name);
		}
		else {
			// if it is not, then return an empty tuple -- type NONE, value ""
			this->next();	// we must still advance the token otherwise we will be all messed up
			_type = NONE;
			_value = "";
		}
	}
	else {
		this->next();	// we must still advance the token otherwise we will be all messed up
		this->error("No KWD detected after token 'alloc' !", 301);	// print error
	}

	// Return our tuple. Note it will be NONE/NULL if invalid, which we will handler in the compiler/interpreter
	std::tuple<Type, std::string> _Allocation = std::make_tuple(_type, _value);
	return _Allocation;
}


std::tuple<LValue, std::shared_ptr<Expression>> Parser::parse_assignment() {
	// Make objects for our LValue and Expression
	LValue _LValue;
	std::shared_ptr<Expression> _Expression;

	lexeme left_token = this->next();	// the LValue will be the next token after 'let'
	if (std::get<0>(left_token) == "ident") {	// LValue MUST be an identifier
		_LValue.setValue(std::get<1>(left_token));	// set the new value
	}

	// We must ensure that the punctuation is correct
	lexeme op = this->next();
	if (std::get<0>(op) == "op_char" && std::get<1>(op) == "=") {
		// Now, we can parse the assignment expression
		_Expression = this->parse_expression();

		// check what kind of expression it is so we know how to return it
		std::string exp_type = _Expression.get()->getExpType();


		// First, check to see if we have a literal
		if (exp_type == "literal") {
			std::cout << "\t_Expression->getExpType == a literal" << std::endl;
			Literal* literal_exp = dynamic_cast<Literal*>(_Expression.get());
			return std::make_tuple(_LValue, std::make_shared<Literal>(literal_exp->get_type(), literal_exp->get_value()));
		}
		
		// Else, check if it's a unary expression
		else if (exp_type == "unary") {
			Unary* unary_op = dynamic_cast<Unary*>(_Expression.get());
			return std::make_tuple(_LValue, std::make_shared<Unary>(unary_op->get_operand(), unary_op->get_operator()));
		}
		
		// Else, check if it is a variable
		else if (exp_type == "LValue") {
			LValue* var = dynamic_cast<LValue*>(_Expression.get());
			return std::make_tuple(_LValue, std::make_shared<LValue>(var->getValue()));
		}

		// Else, check if it is a binary expression
		else if (exp_type == "binary") {
			Binary* bin_exp = dynamic_cast<Binary*>(_Expression.get());
			return std::make_tuple(_LValue, _Expression);
		}

		else {
			this->error("Invalid token following assignment operator.", 322);
		}
	}

	// If the character after "let" is NOT =
	else {
		this->error("Punctuation in assignment was not '=' !", 303);
		_LValue.setValue("");	// return an empty assignment expression
		return std::make_tuple(_LValue, _Expression);
	}
}



/*********************	PARSING EXPRESSIONS		*********************/


std::shared_ptr<Expression> Parser::parse_expression() {
	std::cout << "Parsing expression..." << std::endl;

	// get the first lexeme from the expression
	lexeme exp_lexeme = this->next();
	std::string lex_type = std::get<0>(exp_lexeme);	// get the type so we don't have to type it out every time

	// first, check if we have a literal -- they are easy to parse
	if (lex_type == "int" || lex_type == "float" || lex_type == "string" || lex_type == "bool") {
		// check to see we don't have a binary
		// The only characters that may follow a literal are a semicolon, an operator, or a closing paren
		if (std::get<0>(this->peek()) == "punc" && (std::get<1>(this->peek()) == ";" || std::get<1>(this->peek()) == ")")) {
			Type literal_type = this->get_type(lex_type);
			std::string value = std::get<1>(exp_lexeme);
			return std::make_shared<Literal>(literal_type, value);
		}
		else if (std::get<0>(this->peek()) == "op_char") {
			// We may have a binary expression beginning with a literal

			// Check out our binary expression at our current position, using it to set up a conditional
			//if (this->is_binary(this->current_token)) {
			//	std::cout << "Make the left operand a binary expression!" << std::endl;

			//	// First, we will make a literal using the leftmost operand of our expression
			//	std::shared_ptr<Literal> _leftmost_op = std::make_shared<Literal>(this->get_type(lex_type), std::get<1>(exp_lexeme));

			//	// Next, make a binary expression with it
			//	std::tuple<std::shared_ptr<Expression>, std::shared_ptr<Expression>, exp_operator> left_bin_exp = this->parse_binary(_leftmost_op);
			//	std::shared_ptr<Binary> left_op = std::make_shared<Binary>(std::get<0>(left_bin_exp), std::get<1>(left_bin_exp), std::get<2>(left_bin_exp));

			//	// Make a binary expression and return it
			//	std::tuple<std::shared_ptr<Expression>, std::shared_ptr<Expression>, exp_operator> bin_exp = this->parse_binary(left_op);
			//	return std::make_shared<Binary>(std::get<0>(bin_exp), std::get<1>(bin_exp), std::get<2>(bin_exp));
			//}
			//else {
			//	std::cout << "Do not make the left operand a binary expression!" << std::endl;
			//	
			//	// First, make a literal using the left operand
			//	std::shared_ptr<Literal> _leftmost_op = std::make_shared<Literal>(this->get_type(lex_type), std::get<1>(exp_lexeme));

			//	// Next, turn it into a binary expression
			//	std::tuple<std::shared_ptr<Expression>, std::shared_ptr<Expression>, exp_operator> left_bin_exp = this->parse_binary(_leftmost_op);

			//	// Return that binary expression as a pointer
			//	return std::make_shared<Binary>(std::get<0>(left_bin_exp), std::get<1>(left_bin_exp), std::get<2>(left_bin_exp));
			//}
			return this->maybe_binary();
		}
		else {
			// If we have something else, i.e. a literal followed by anything other than a semicolon, closing paren, or op_char, it is invalid
			this->error("Invalid character following literal operand!", 321);
		}
	}
	// Next, check if it is a lone variable
	else if (lex_type == "ident") {
		lexeme _peek = this->peek();
		if (std::get<0>(_peek) == "punc" && (std::get<1>(_peek) == ";" || std::get<1>(_peek) == ")")) {
			// if the next character is a semicolon, and the only thing is a lone variable, then return the variable as an LValue
			return std::make_shared<LValue>(std::get<1>(this->current()));
		}
		else if (std::get<0>(_peek) == "op_char") {
			// We may have a binary beginning with a variable
			
			// get the variable as a shared ptr
			// the value of the LValue is contained in the current lexeme, which is an ident
			std::shared_ptr<LValue> left_op = std::make_shared<LValue>(std::get<1>(this->current()));

			// parse the binary expression as such
			std::tuple<std::shared_ptr<Expression>, std::shared_ptr<Expression>, exp_operator> bin_exp = this->parse_binary(left_op);
			
			// return it
			return std::make_shared<Binary>(std::get<0>(bin_exp), std::get<1>(bin_exp), std::get<2>(bin_exp));
		}
	}
	// Next, check whether it is an expression inside parens
	else if (lex_type == "punc") {
		if (std::get<1>(exp_lexeme) == "(") {
			std::shared_ptr<Expression> in_parens = this->parse_expression();
			if (std::get<1>(this->peek()) == ")") {
				this->next();
			}

			// We can only return if the next character is the end of the statement 
			if (std::get<1>(this->peek()) == ";") {
				return in_parens;
			}

			// If we have an op_char, we must parse a binary expression with the expression we just parsed as the left operand
			else if (std::get<0>(this->peek()) == "op_char") {
				std::tuple<std::shared_ptr<Expression>, std::shared_ptr<Expression>, exp_operator> bin_exp = this->parse_binary(in_parens);
				std::shared_ptr<Binary> bin_op = std::make_shared<Binary>(std::get<0>(bin_exp), std::get<1>(bin_exp), std::get<2>(bin_exp));
				return bin_op;
			}
		}
		else {
			this->error("I don't know what to do with this character '" + std::get<1>(exp_lexeme) + "'!", 325);
		}
	}
	// Next, determine if the expression is a unary expression, as that is also easy
	// A unary will be an op_char (such as -, !, or +) followed by an int, float, bool, or var (currently, we won't support string unary operations)
	else if (lex_type == "op_char") {
		std::string type_peek = std::get<0>(this->peek());
		if (type_peek == "int" || type_peek == "float" || type_peek == "bool" || type_peek == "ident") {
			lexeme op_char = this->current();	// get the unary operator

			// Check to make sure this is followed by a semicolon
			// Check to see if our operator is a valid unary operator
			if (std::get<1>(op_char) == "!" || std::get<1>(op_char) == "-") {
				std::tuple<std::shared_ptr<Expression>, exp_operator> _u_op = this->parse_unary(op_char);
				return std::make_shared<Unary>(std::get<0>(_u_op), std::get<1>(_u_op));	// make our pointer to the expression, whose contents are the
			}
			else {
				this->error("Unknown unary operator '" + std::get<1>(op_char) + "'!", 361);
			}
		}
	}

	// TODO: determine if expression is a binary expression

	else {
		this->error("Unknown token type!", 360);
	}
}

std::tuple<std::shared_ptr<Expression>, exp_operator> Parser::parse_unary(lexeme op_char) {
	exp_operator unary_operator = translate_operator(std::get<1>(op_char));
	std::shared_ptr<Expression> unary_operand = this->parse_expression();
	return std::make_tuple(unary_operand, unary_operator);
}


bool Parser::is_binary(int position) {
	// returns TRUE if the left token of the binary expression should itself be a binary expression
	// returns FALSE if the current token should be the left operand in the binary expression
	lexeme left_operand = this->tokens[position];
	lexeme my_operator = this->tokens[position + 1];

	if (std::get<0>(my_operator) == "op_char") {
		int my_precedence = this->get_precedence(std::get<1>(my_operator));	// get the precedence of the first operator
		lexeme right_operand = this->tokens[position + 2];

		// if the third token / right operand / token after the op_char is a punctuation symbol:
		if (std::get<0>(right_operand) == "punc") {
			// The only valid punctuation mark to follow an operator is an opening paren
			if (std::get<1>(right_operand) == "(") {
				return false;
			}
			// If it's not an opening paren, it's a syntax error
			else {
				this->error("Unexpected punctuation '" + std::get<1>(right_operand) + "'", 410);
			}
		}
		// if it is either an int, float, bool, or var:
		else if (std::get<0>(right_operand) == "int" || std::get<0>(right_operand) == "float" || std::get<0>(right_operand) == "bool" || std::get<0>(right_operand) == "var") {
			// get the token after the right operand
			lexeme proceeding_token = this->tokens[position + 3];
			
			// if that token is a semicolon, we are done
			if (std::get<1>(proceeding_token) == ";") {
				return false;
			}

			// if that token is another op_char, check its precedence
			if (std::get<0>(proceeding_token) == "op_char") {
				// get the precedence
				int next_precedence = this->get_precedence(std::get<1>(proceeding_token));

				// compare
				// if the precedence of the second operator is higher than the first, return false; the left operand can stand alone
				if (next_precedence > my_precedence) {
					return false;
				}
				// if the precedence of the second operator is less than or equal to the first, return true; we will make the left argument a binary expression
				else {
					return true;
				}
			}
		}
		// if it is some other token (i.e. not a literal, var, or punctuation symbol), throw an error
		else {
			this->error("Unexpected token type after op_char in binary expression", 460);
		}
	}
	else {
		this->error("Expected op_char; encountered '" + std::get<0>(my_operator) + "'.", 320);
	}
}


std::shared_ptr<Binary> Parser::maybe_binary() {
	lexeme left_operand = this->current();
	lexeme my_op = this->next();
	int my_precedence = this->get_precedence(std::get<1>(my_op));

	// Just to be safe, ensure we are still within the file
	if (this->current_token + 1 <= this->num_tokens) {
		lexeme right_operand = this->next();
		lexeme his_op = this->peek();

		// First, check if right_operand is a paren
		if (std::get<0>(right_operand) == "punc" && std::get<1>(right_operand) == "(") {
			
			int his_precedence = this->get_precedence(std::get<1>(right_operand));
			
			if (his_precedence > my_precedence) {
				// left operand is not binary

				// left operand = left_operand;
				// right operand = maybe_binary(his_op)

				std::tuple<std::shared_ptr<Expression>, std::shared_ptr<Expression>, exp_operator> _bin_exp_tuple;
				// First, check to see if it is a variable or a literal
				if (is_literal(std::get<0>(left_operand))) {
					// if one of the literal types, then we will construct a literal as our left operator
					std::shared_ptr<Literal> _left = std::make_shared<Literal>(this->get_type(std::get<0>(left_operand)), std::get<1>(left_operand));

					// Next, we will skip the paren
					this->next();

					// now, we will construct our right by performing "maybe binary" on his_op
					std::shared_ptr<Binary> _right = this->maybe_binary();

					// make the tuple
					_bin_exp_tuple = std::make_tuple(_left, _right, translate_operator(std::get<1>(my_op)));
					return std::make_shared<Binary>(std::get<0>(_bin_exp_tuple), std::get<1>(_bin_exp_tuple), std::get<2>(_bin_exp_tuple));
				}
			}
		}
		// If not, check to see if it's an op_char
		else if (std::get<0>(his_op) == "op_char") {
			int his_precedence = this->get_precedence(std::get<1>(his_op));

			if (his_precedence > my_precedence) {

				// left operand is not binary

				// left operand = left_operand;
				// right operand = maybe_binary(his_op)

				std::tuple<std::shared_ptr<Expression>, std::shared_ptr<Expression>, exp_operator> _bin_exp_tuple;
				// First, check to see if it is a variable or a literal
				if (is_literal(std::get<0>(left_operand))) {
					// if one of the literal types, then we will construct a literal as our left operator
					std::shared_ptr<Literal> _left = std::make_shared<Literal>(this->get_type(std::get<0>(left_operand)), std::get<1>(left_operand));

					// now, we will construct our right by performing "maybe binary" on his_op
					std::shared_ptr<Binary> _right = this->maybe_binary();

					// make the tuple
					_bin_exp_tuple = std::make_tuple(_left, _right, translate_operator(std::get<1>(my_op)));
				}
				else if (std::get<0>(left_operand) == "var") {
					// If type is "var", it is simply an LValue (a variable), so we will construct our left operand for our binary expression
					std::shared_ptr<LValue> _left = std::make_shared<LValue>(std::get<1>(left_operand));

					// now, we will construct our right by performing "maybe binary" on his_op
					std::shared_ptr<Binary> _right = this->maybe_binary();
					_bin_exp_tuple = std::make_tuple(_left, _right, translate_operator(std::get<1>(my_op)));
				}

				return std::make_shared<Binary>(std::get<0>(_bin_exp_tuple), std::get<1>(_bin_exp_tuple), std::get<2>(_bin_exp_tuple));
			}
			else {

				// left operand is a binary

				// left operand = construct a binary with mine and his
				// right = maybe binary (his op)
				
				// First, create our shared ptrs
				std::shared_ptr<Expression> _left_exp;
				std::shared_ptr<Expression> _right_exp;

				// First, test the left operand
				// If the left operand is a literal
				if (is_literal(std::get<0>(left_operand))) {
					_left_exp = std::make_shared<Literal>(this->get_type(std::get<0>(left_operand)), std::get<1>(left_operand));
				}
				// If the left operand is a variable / LValue
				else if (std::get<0>(left_operand) == "var") {
					_left_exp = std::make_shared<LValue>(std::get<1>(left_operand));
				}
				// Just in case
				else {
					this->error("Invalid type for binary operand; type must be a literal expression or an LValue expression (a variable)", 430);
				}

				// If the right operand is a literal
				if (is_literal(std::get<0>(right_operand))) {
					_right_exp = std::make_shared<Literal>(this->get_type(std::get<0>(right_operand)), std::get<1>(right_operand));
				}
				// If the right operand is a variable / LValue
				else if (std::get<0>(right_operand) == "var") {
					_right_exp = std::make_shared<LValue>(std::get<1>(right_operand));
				}
				// Just in case
				else {
					this->error("Invalid type for binary operand; type must be a literal expression or an LValue expression (a variable)", 430);
				}

				// Now, construct a binary expression using what we just created
				std::shared_ptr<Binary> left_operand = std::make_shared<Binary>(_left_exp, _right_exp, translate_operator(std::get<1>(my_op)));

				// Move our pointer ahead from right_operand to the operand after
				this->next();
				this->next();

				// If the next character is a semicolon or end paren, return a binary with operands of a binary and a literal or LValue
				lexeme _peek = this->peek();
				if (std::get<0>(_peek) == "punc" && (std::get<1>(_peek) == ";" || std::get<1>(_peek) == ")")) {
					// get the current operand
					lexeme last_operand = this->current();
					if (is_literal(std::get<0>(last_operand))) {
						std::shared_ptr<Literal> literal_op = std::make_shared<Literal>(this->get_type(std::get<0>(last_operand)), std::get<1>(last_operand));
						return std::make_shared<Binary>(left_operand, literal_op, translate_operator(std::get<1>(his_op)));
					}
					else if (std::get<0>(last_operand) == "var") {
						std::shared_ptr<LValue> lvalue_op = std::make_shared<LValue>(std::get<1>(last_operand));
						return std::make_shared<Binary>(left_operand, lvalue_op, translate_operator(std::get<1>(his_op)));
					}
					else {
						this->error("Expected expression.", 430);
					}
				}
				// Else, parse another binary expression
				else {
					// Now, use maybe_binary() to construct the right operand for this binary expression
					return std::make_shared<Binary>(left_operand, this->maybe_binary(), translate_operator(std::get<1>(his_op)));
				}
			}
		}

		// If "his_op" is not an opchar, but rather punctuation
		else if (std::get<0>(his_op) == "punc" && std::get<1>(his_op) != "(") {

			// If the next character is a semicolon
			if ((std::get<1>(his_op) == ";") || (std::get<1>(his_op) == ")")) {
				std::tuple<std::shared_ptr<Expression>, std::shared_ptr<Expression>, exp_operator> _bin_exp_tuple;
				std::shared_ptr<Expression> _left_operand, _right_operand;
				
				if (is_literal(std::get<0>(left_operand))) {
					_left_operand = std::make_shared<Literal>(this->get_type(std::get<0>(left_operand)), std::get<1>(left_operand));
				}
				else if (std::get<0>(left_operand) == "var") {
					_left_operand = std::make_shared<LValue>(std::get<1>(left_operand));
				}

				if (is_literal(std::get<0>(left_operand))) {
					_right_operand = std::make_shared<Literal>(this->get_type(std::get<0>(right_operand)), std::get<1>(right_operand));
				}
				else if (std::get<0>(left_operand) == "var") {
					_right_operand = std::make_shared<LValue>(std::get<1>(right_operand));
				}

				std::shared_ptr<Binary> binary_operation = std::make_shared<Binary>(_left_operand, _right_operand, translate_operator(std::get<1>(my_op)));
				return binary_operation;
			}
		}
		else {
			// his op is invalid
			this->error("Invalid token type in binary operation", 350);
		}
	}
	// If we arrive here, we have unexpectedly hit EOF
	else {
		this->error("End of file error!", 501);
	}
}


std::tuple<std::shared_ptr<Expression>, std::shared_ptr<Expression>, exp_operator> Parser::parse_binary(std::shared_ptr<Expression> left_exp) {
	std::cout << "\tParsing binary expression..." << std::endl;
	// Parse a binary expression
	
	lexeme _op_lex = this->next();

	if (std::get<0>(_op_lex) != "op_char") {
		// If the character is NOT actually an op_char, we must throw an exception
		this->error("Invalid operand type; must be type 'op_char'", 350);
	}
	else {
		// The next character is an op_char
		// We need to use translate_operator because _op_lex<1> is a string, and we need type exp_operator
		exp_operator op = translate_operator(std::get<1>(_op_lex));

		// use parse_expression to get our right operand
		std::shared_ptr<Expression> right_exp = this->parse_expression();

		// finally, return the tuple with all the info
		return std::make_tuple(left_exp, right_exp, op);
	}
}



// Populate our tokens list
void Parser::populate_tokens_list() {
	this->token_stream->peek();	// to make sure we haven't gone beyond the end of the file

	while (!this->token_stream->eof()){
		lexeme current_token;
		std::string type;
		std::string value;

		if (this->token_stream->peek() != '\n') {
			*this->token_stream >> type;
			*this->token_stream >> value;
		}
		else {
			this->token_stream->get();
		}

		current_token = std::make_tuple(type, value);

		if (std::get<0>(current_token) == "") {
			continue;
		}
		else {
			this->tokens.push_back(current_token);
		}
	}
}


std::istream& Parser::read(std::istream& is) {
	char garbage = ' ';
	std::string type, value = "";
	is >> garbage >> garbage >> type >> value;

	//std::cout << type << std::endl << value << std::endl;

	return is;
}


Parser::Parser(std::ifstream* stream)
{
	Parser::token_stream = stream;
	Parser::populate_tokens_list();
	Parser::current_token = 0;
	Parser::num_tokens = Parser::tokens.size() - 1;
	Parser::ASTNode{};	// initialize to empty vector
	Parser::quit = false;
}


Parser::~Parser()
{
}

std::istream& operator>>(std::istream& is, Parser& parser) {
	return parser.read(is);
}
