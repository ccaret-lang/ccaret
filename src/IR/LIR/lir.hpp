// this is the caretc a compiler for the C^
#pragma once
#include "IR/MIR/mir.hpp"
#include <vector>
namespace caret::ir {
struct LirModule;
LirModule lower_to_lir(const std::vector<MirFunction>& mir);
}
