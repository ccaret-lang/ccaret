// Parser — token stream -> AST translation unit.
//
// The parser is hand-written recursive descent with one-token lookahead
// (and a couple of two-token peeks where C^ grammar demands it, e.g.
// `T mut*` vs `T mut name`). It never throws; every error is funneled
// through the DiagnosticsEngine, and the parser tries to recover at the
// next statement / declaration boundary so a single typo doesn't bury
// the rest of the file.
//
// Return contract:
//   ParseResult.ok      — true iff no errors were emitted to `diags`.
//   ParseResult.unit    — partial AST is preserved even when ok==false,
//                         so downstream tools can still introspect what
//                         was salvageable.
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

// Drive the parser over `tokens`. `diags` is borrowed (must outlive
// the call) and receives every error/warning; `source_path` is the
// filename used in those diagnostics.
ParseResult parse(const std::vector<Token>& tokens, DiagnosticsEngine& diags,
                  const std::string& source_path);
}
