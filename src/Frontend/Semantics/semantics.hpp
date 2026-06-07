// Semantics pass.
//
// Reserved for the type checker, mutability/borrow checker, and
// resolution of user types to their declarations. The driver calls
// into this header (when wired up); v0.1.0 leaves it as a no-op stub
// because the C backend accepts untyped AST.
#pragma once
#include "AST/ast.hpp"
#include "Diagnostics/diagnostics.hpp"
namespace caret::frontend {
// Stand-in signature kept for source compatibility with the planned
// semantic analyser. The current implementation is a no-op; the real
// entry point will take an `ast::TranslationUnit` (not the legacy
// `std::vector<ast::AstNode>` placeholder) once that type lands.
bool analyze(const std::vector<ast::AstNode>& ast, DiagnosticsEngine& diags);
}
