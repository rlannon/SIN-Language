#pragma once

#include <memory>
#include <iostream>
#include <vector>

#include "Statement.h"
#include "Expression.h"

std::shared_ptr<Binary> parse_bin_op(Expression left, int my_prec);