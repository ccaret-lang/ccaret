// this is the caretc a compiler for the C^
#pragma once
#include "AST/ast.hpp"
#include "Diagnostics/diagnostics.hpp"
namespace caret::frontend {
bool analyze(const std::vector<ast::AstNode>& ast, DiagnosticsEngine& diags);
}
