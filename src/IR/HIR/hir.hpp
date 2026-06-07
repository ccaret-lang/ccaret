// High-level IR (HIR) — the first stage of the planned multi-IR
// pipeline. The HIR will desugar syntactic sugar (range-for, defer,
// `?`-unwrap) into a small set of explicit control-flow primitives
// so downstream IRs (MIR, LIR) can reason about it without needing
// to re-derive sugar.
//
// Today this is a stub; the driver goes straight from AST to the C
// backend. The header stays in place so call sites in main.cpp and
// the linker can be wired up incrementally without a build-system
// churn when the lowerings land.
#pragma once
#include "AST/ast.hpp"
#include <vector>
namespace caret::ir {
// Placeholder header kept for the v0.2 multi-IR pipeline. The
// current driver doesn't lower past AST (we go straight to C
// emission), so the HIR data shape is still a TODO.
struct HirFunction;
std::vector<HirFunction> lower_to_hir(const std::vector<ast::AstNode>& ast);
}
