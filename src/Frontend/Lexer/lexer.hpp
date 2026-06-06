// Lexer — turn a source buffer into a flat token stream.
//
// The lexer is single-pass, allocation-light (one std::vector<Token>),
// and never throws. Errors are recorded as TokenKind::Unknown tokens;
// the parser turns those into diagnostics with proper context. This
// keeps the lexer's contract trivial: bytes in, tokens out, always.
//
// The returned token stream always ends with a TokenKind::Eof sentinel.
#pragma once
#include "Frontend/Token/token.hpp"
#include <string>
#include <vector>
namespace caret::frontend {
// Extract the Nth source line (1-based) for diagnostic rendering.
// Returns an empty string if `line` is out of range. The result is a
// copy; callers may mutate it freely (typically: trim, expand tabs).
std::string line_text(const std::string& text, std::uint32_t line);

// Tokenise `source`. `path` is informational only — the lexer never
// touches the filesystem; the path is propagated through to the parser
// purely so diagnostics can report a `file:line:col` triple. The
// returned vector's Token::lexeme views point into `source`, which
// therefore must outlive the returned tokens.
std::vector<Token> lex(const std::string& source, const std::string& path = "<input>");
}
