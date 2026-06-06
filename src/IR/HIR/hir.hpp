// this is the caretc a compiler for the C^
#pragma once
#include "AST/ast.hpp"
#include <vector>
namespace caret::ir {
struct HirFunction;
std::vector<HirFunction> lower_to_hir(const std::vector<ast::AstNode>& ast);
}
