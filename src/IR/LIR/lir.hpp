// Low-level IR (LIR) — the final planned stage before machine code
// emission. The LIR will carry target-specific concerns: register
// classes, calling convention, calling-convention lowering, and the
// actual instruction selection. For v0.1.0 we skip the LIR and go
// straight from AST to C; the header is reserved for the v0.2 native
// backend (x86_64 first, aarch64 after).
#pragma once
#include "IR/MIR/mir.hpp"
#include <vector>
namespace caret::ir {
struct LirModule;
LirModule lower_to_lir(const std::vector<MirFunction>& mir);
}
