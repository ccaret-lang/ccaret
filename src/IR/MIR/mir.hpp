// Mid-level IR (MIR) — the second stage of the planned multi-IR
// pipeline. The MIR will model the program as a control-flow graph
// of basic blocks, suitable for dataflow analysis (live ranges,
// reachability) and for borrow checking. It is target-independent:
// no registers, no calling convention, no machine word size.
//
// Not wired into the driver yet. Header kept so the file is part of
// the include graph from day one.
#pragma once
#include "IR/HIR/hir.hpp"
#include <vector>
namespace caret::ir {
struct MirFunction;
std::vector<MirFunction> lower_to_mir(const std::vector<HirFunction>& hir);
}
