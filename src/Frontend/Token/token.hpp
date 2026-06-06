// this is the caretc a compiler for the C^
#pragma once
#include <cstdint>
#include <string>
#include <string_view>
namespace caret::frontend {
enum class TokenKind : std::uint16_t {
    Unknown,
    Identifier,
    Integer,
    Float,
    String,
    Char,
    Keyword,
    Operator,
    Punct,
    Eof,
};
struct Token {
    TokenKind kind{TokenKind::Unknown};
    std::string_view lexeme;
    std::uint32_t line{0};
    std::uint32_t column{0};
};
}
