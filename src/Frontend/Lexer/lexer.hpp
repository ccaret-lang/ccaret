// this is the caretc a compiler for the C^
#pragma once
#include "Frontend/Token/token.hpp"
#include <string>
#include <vector>
namespace caret::frontend {
std::string line_text(const std::string& text, std::uint32_t line);
std::vector<Token> lex(const std::string& source, const std::string& path = "<input>");
}
