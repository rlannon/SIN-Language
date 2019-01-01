#include "Compiler.h"


std::shared_ptr<Statement> Compiler::get_next_statement()
{
	this->AST_index += 1;	// increment AST_index by one
	std::shared_ptr<Statement> stmt_ptr = this->AST.statements_list[AST_index];	// get the shared_ptr<Statement> at the correct position
	return stmt_ptr; // return the shared_ptr<Statement>
}

std::shared_ptr<Statement> Compiler::get_current_statement()
{
	return this->AST.statements_list[AST_index];	// return the shared_ptr<Statement> at the current position of the AST index
}



void Compiler::include_file(Include include_statement)
{
	/*
	
	Include statements take the included file and add its code in the reserved area in memory
		- If the file is a .sina file, its assembly is added
		- If the file is a .sinc file, it is disassembled before being added
		- If the file is a .sin file, it is compiled and its assembly added

	Handling included files and how they relate to the main executable is all handled by the linker

	*/

	std::string to_include = include_statement.get_filename();
	size_t extension_position = to_include.find(".");
	std::string extension = to_include.substr(extension_position);
	std::string filename_no_extension = to_include.substr(0, extension_position);

	// if the extension is '.sinc', we will simply add that; we don't need to compile or assemble
	if (extension == ".sinc") {
		// push back the file name
		object_file_names->push_back(to_include);
	}
	else if (extension == ".sina") {
		// now, open the compiled include file as an istream object
		std::ifstream compiled_include;
		compiled_include.open(filename_no_extension + ".sina", std::ios::in);
		if (compiled_include.is_open()) {
			// then turn it into an object file
			Assembler assemble(compiled_include, this->_wordsize);
			compiled_include.close();

			// finally, add the object file's name to our vector
			this->object_file_names->push_back(filename_no_extension + ".sinc");
		}
		// if we cannot open the .sina file
		else {
			throw std::exception(("**** Compiled the include file '" + filename_no_extension + ".sina" + "', but could not open the compiled version for assembly.").c_str());
		}
	}
	else if (extension == ".sin") {
		// if the have another SIN file, we need to compile it and then push it back
		std::ofstream compiled_include;
		compiled_include.open(filename_no_extension + ".sina", std::ios::out);
		if (compiled_include.is_open()) {
			// compile to sinasm
			this->compile_to_sinasm(compiled_include);
			compiled_include.close();

			// now, open the compiled include file as an istream object
			std::ifstream compiled_include;
			compiled_include.open(filename_no_extension + ".sina", std::ios::in);
			if (compiled_include.is_open()) {
				// then turn it into an object file
				Assembler assemble(compiled_include, this->_wordsize);
				compiled_include.close();

				// finally, add the object file's name to our vector
				this->object_file_names->push_back(filename_no_extension + ".sinc");
			}
			// if we cannot open the .sina file
			else {
				throw std::exception(("**** Compiled the include file '" + filename_no_extension + ".sina" + "', but could not open the compiled version for assembly.").c_str());
			}
		}
		// if we cannot open the .sin file
		else {
			throw std::exception(("**** Could not open included file '" + to_include + "'!").c_str());
		}
	}
}



// allocate a variable
void Compiler::allocate(Allocation allocation_statement) {
	// first, add the variable to the symbol table
	// next, create a macro for it and assign it to the next available address in the scope; local variables will use pages incrementally
	// next, every time a variable is referenced, make sure it is in the symbol table and within the appropriate scope
	// each scope will
	// after that, simply use that variable's name as a macro in the code
	// the macro will reference its address and hold the proper value

	std::string symbol_name = allocation_statement.getVarName();
	Type symbol_type = allocation_statement.getVarType();
	std::string symbol_scope = allocation_statement.get_scope_name();
	int scope_level = allocation_statement.get_scope_level();

	// the 'insert' function will throw an error if the given symbol is already in the table in the same scope
	// this allows two variables in separate scopes to have different names, but also allows local variables to shadow global ones
	// local variable names are ALWAYS used over global ones
	this->symbol_table.insert(symbol_name, symbol_type, symbol_scope, scope_level);

	// TODO: finish algorithm...

}

// create a function definition
void Compiler::define(Definition definition_statement) {
	// first, add the function to the symbol table
	// next, create a label for it at the bottom of the program -- all function definitions will be placed at the bottom as subroutines
	// next, every time a function is referenced, push the relevant arguments on the stack and use a JSR statement

	std::shared_ptr<Expression> func_name_expr = definition_statement.get_name();
	LValue* lvalue_ptr = dynamic_cast<LValue*>(func_name_expr.get());
	std::string func_name = lvalue_ptr->getValue();
	Type return_type = definition_statement.get_return_type();
	// function definitions have to be in the global scope
	if (definition_statement.get_scope_name() == "global" && definition_statement.get_scope_level() == 0) {
		this->symbol_table.insert(func_name, return_type, "global", 0);

		// TODO: should we change the type so that we have functions as their own type? or is return type enough?
		// Type exists mostly for type checking, so perhaps the return type is enough

	}
	else {
		throw std::exception("**** Compiler Error: Function definitions must be in the global scope.");
	}

	// TODO: finish algorithm...
	// Should we call the actual compilation routine on the code inside the definition?
	// i.e., do it recursively?

}



// the actual compilation routine
// it is separate from "Compile::compile()" so that we can call it recursively
void Compiler::compile_to_sinasm(std::ostream& output_file) {
	// get the pointer to the statement
	Statement* current_statement = this->get_next_statement().get();	// AST_index starts at -1 so we first get item 0

	// convert it to the appropriate statement type by fetching the type and converting appropriately
	std::string statement_type = current_statement->get_type();
	if (statement_type == "include") {
		// dynamic_cast to an Include type
		Include* include_statement = dynamic_cast<Include*>(current_statement);

		// "compile" an include statement
		this->include_file(*include_statement);
	}
}



// our entry function
void Compiler::compile(std::string sina_filename) {
	// first, open the file of our filename
	std::ofstream sina_file;
	sina_file.open(sina_filename, std::ios::out);

	// compile to ASM; create the file
	this->compile_to_sinasm(sina_file);

	// close our output file and return to caller
	sina_file.close();
	return;
}



// If we initialize the compiler with a file, it will automatically lex and parse
Compiler::Compiler(std::istream& sin_file, uint8_t _wordsize, std::vector<std::string>* object_file_names) : _wordsize(_wordsize), object_file_names(object_file_names) {
	// create the parser and lexer objects
	Lexer lex(sin_file);
	Parser parser(lex);
	
	// get the AST from the parser
	this->AST = parser.createAST();

	this->current_scope = 0;	// start at the global scope
								
	// TODO: do we need "current_scope" ?

	this->_DATA_PTR = 0;	// our first address for variables is $00
	this->AST_index = -1;	// we use "get_next_statement()" every time, which increments before returning; as such, start at -1 so we fetch the 0th item, not the 1st, when we first call the compilation function
	symbol_table = SymbolTable();
}


Compiler::~Compiler()
{
}
