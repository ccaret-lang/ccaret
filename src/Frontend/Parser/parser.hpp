// this is the caretc a compiler for the C^
#pragma once
#include "AST/ast.hpp"
#include "Diagnostics/diagnostics.hpp"
#include "Frontend/Token/token.hpp"
#include <string>
#include <vector>
namespace caret::frontend {
struct ParseResult {
    ast::TranslationUnit unit;
    bool ok{false};
};
ParseResult parse(const std::vector<Token>& tokens, DiagnosticsEngine& diags,
                  const std::string& source_path);
}
