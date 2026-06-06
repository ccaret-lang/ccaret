// this is the caretc a compiler for the C^
#pragma once
#include <string>
#include <vector>
namespace caret::frontend {
struct Token;
std::vector<Token> lex(const std::string& source);
}
