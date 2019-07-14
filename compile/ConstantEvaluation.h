/*

SIN Toolchain
ConstantEvaluation.h
Copyright 2019 Riley Lannon

The purpose of this class is to evaluate constant expressions at compile-time -- if these values can be precomputed, it makes sense to do so.
This is a very basic optimization, and it will increase compile time, but it is an optimization nonetheless.

*/

#pragma once

#include "../parser/Expression.h"

class ConstantEvaluation
{
	
public:
	ConstantEvaluation();
	~ConstantEvaluation();
};

