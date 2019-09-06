/*

SIN Toolchain
EnumeratedTypes.h
Copyright 2019 Riley Lannon

The purpose of this file is to have all of the enumerated types defined in a single place so they can be referred to be multiple files without creating any circular dependencies.
These are to be used so that this code might be more maintainable and less error-prone; while all of these could be replaced with std::string, using an enum centralizes the definitions and makes it much more difficult to have an error hidden somewhere in the code because you used "Dynamic" instead of "dynamic" as a symbol quality, for example. Further, using enumerated types also makes it more clear in the code what is what instead of having set codes for statement types or symbol qualities. This way, it is very clear what the type or quality is without needing to look up anything else.

*/

#pragma once

enum stmt_type {
	// The various types of statements we can have in SIN
	STATEMENT_GENERAL,
	INCLUDE,
	DECLARATION,
	ALLOCATION,
	ASSIGNMENT,
	RETURN_STATEMENT,
	IF_THEN_ELSE,
	WHILE_LOOP,
	DEFINITION,
	CALL,
	INLINE_ASM,
	FREE_MEMORY
};


enum exp_type {
	// So that we can list all of the various expression types in one place
	EXPRESSION_GENERAL,
	LITERAL,	// can be a literal int (e.g., 5), a literal float (e.g., 1.2), a literal string (e.g., "hello"), a literal bool (true/false) ...
	LVALUE,		// any named data
	LIST,	// initializer-lists, mostly; syntax is { ... , ... }
	INDEXED,
	ADDRESS_OF,
	DEREFERENCED,
	BINARY,
	UNARY,
	VALUE_RETURNING_CALL,
	SIZE_OF
};

enum SymbolType {
	// So that we know whether a symbol is a variable, function definition, struct definition...
	VARIABLE,
	FUNCTION_DEFINITION,
	STRUCT_DEFINITION
};


const size_t num_qualities = 5;	// we can have five symbol qualities, enumerated in 'enum SymbolQuality'
enum SymbolQuality {
	// So that the symbol's quality does not need to be stored as a string
	NO_QUALITY,
	CONSTANT,
	STATIC,
	DYNAMIC,
	SIGNED,
	UNSIGNED
};


enum exp_operator {
	// So that we have a clear list of operators
	PLUS,
	MINUS,
	MULT,
	DIV,
	EQUAL,
	NOT_EQUAL,
	GREATER,
	LESS,
	GREATER_OR_EQUAL,
	LESS_OR_EQUAL,
	AND,	// 'AND' is equivalent to C++ &&
	NOT,
	OR,
	MODULO,
	BIT_AND,	// 'BIT_AND' is bitwise-AND (C++ &)
	BIT_OR,
	NO_OP
};


enum Type {
	// So that our types are all clearly defined
	NONE,
	INT,
	FLOAT,
	STRING,
	BOOL,
	VOID,
	PTR,
	RAW,
	ARRAY,
	STRUCT
};
