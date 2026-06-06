// this is the caretc a compiler for the C^
#pragma once
#include "AST/ast.hpp"
#include "Frontend/Token/token.hpp"
#include <vector>
namespace caret::frontend {
std::vector<ast::AstNode> parse(const std::vector<Token>& tokens);
}
