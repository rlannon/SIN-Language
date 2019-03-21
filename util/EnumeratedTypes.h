#pragma once

/*

SIN Toolchain
EnumeratedTypes.h
Copyright 2019 Riley Lannon

The purpose of this file is to have all of the enumerated types defined in a single place so they can be referred to be multiple files without creating any circular dependencies.
These are to be used so that this code might be more maintainable and less error-prone; while all of these could be replaced with std::string, using an enum centralizes the definitions and makes it much more difficult to have an error hidden somewhere in the code because you used "Dynamic" instead of "dynamic" as a symbol quality, for example. Further, using enumerated types also makes it more clear in the code what is what instead of having set codes for statement types or symbol qualities. This way, it is very clear what the type or quality is without needing to look up anything else.

*/


// The various types of statements we can have in SIN
enum stmt_type {
	STATEMENT_GENERAL,
	INCLUDE,
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

// Defined so that we can list all of the various expression types in one place
enum exp_type {
	EXPRESSION_GENERAL,
	LITERAL,
	LVALUE,
	INDEXED,
	ADDRESS_OF,
	DEREFERENCED,
	BINARY,
	UNARY,
	VALUE_RETURNING_CALL,
	SIZE_OF
};

// So that the symbol's quality does not need to be stored as a string
enum SymbolQuality {
	NO_QUALITY,
	CONSTANT,
	STATIC,
	DYNAMIC,
	SIGNED,
	UNSIGNED
};

// Defined so that we have a clear list of operators
enum exp_operator {
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

// Defined so that our types are all clearly defined
enum Type {
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
