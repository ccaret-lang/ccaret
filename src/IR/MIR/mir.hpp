// this is the caretc a compiler for the C^
#pragma once
#include "IR/HIR/hir.hpp"
#include <vector>
namespace caret::ir {
struct MirFunction;
std::vector<MirFunction> lower_to_mir(const std::vector<HirFunction>& hir);
}
