// this is the caretc a compiler for the C^
#include "lexer.hpp"
#include <cctype>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace caret::frontend {

std::string line_text(const std::string& text, std::uint32_t line) {
    std::uint32_t cur = 1;
    std::size_t start = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\n') {
            if (cur == line) {
                return text.substr(start, i - start);
            }
            ++cur;
            start = i + 1;
        }
    }
    if (cur == line && start < text.size()) {
        return text.substr(start);
    }
    return {};
}

namespace {

const std::unordered_map<std::string_view, TokenKind>& keyword_table() {
    static const std::unordered_map<std::string_view, TokenKind> kTable = {
        // Primitive types
        {"i8", TokenKind::KwI8}, {"i16", TokenKind::KwI16},
        {"i32", TokenKind::KwI32}, {"i64", TokenKind::KwI64},
        {"i128", TokenKind::KwI128}, {"isize", TokenKind::KwISize},
        {"u8", TokenKind::KwU8}, {"u16", TokenKind::KwU16},
        {"u32", TokenKind::KwU32}, {"u64", TokenKind::KwU64},
        {"u128", TokenKind::KwU128}, {"usize", TokenKind::KwUSize},
        {"f32", TokenKind::KwF32}, {"f64", TokenKind::KwF64},
        {"bool", TokenKind::KwBool}, {"char", TokenKind::KwChar},
        {"void", TokenKind::KwVoid}, {"string", TokenKind::KwString},
        {"auto", TokenKind::KwAuto},
        // Modifiers
        {"mut", TokenKind::KwMut}, {"const", TokenKind::KwConst},
        {"packed", TokenKind::KwPacked}, {"volatile", TokenKind::KwVolatile},
        {"export", TokenKind::KwExport}, {"extern", TokenKind::KwExtern},
        // Declarations
        {"struct", TokenKind::KwStruct}, {"enum", TokenKind::KwEnum},
        {"union", TokenKind::KwUnion}, {"error", TokenKind::KwError},
        {"type", TokenKind::KwType}, {"fn", TokenKind::KwFn},
        // Control flow
        {"if", TokenKind::KwIf}, {"else", TokenKind::KwElse},
        {"elif", TokenKind::KwElif}, {"for", TokenKind::KwFor},
        {"while", TokenKind::KwWhile}, {"loop", TokenKind::KwLoop},
        {"match", TokenKind::KwMatch}, {"break", TokenKind::KwBreak},
        {"continue", TokenKind::KwContinue}, {"return", TokenKind::KwReturn},
        // Module
        {"import", TokenKind::KwImport}, {"link", TokenKind::KwLink},
        {"as", TokenKind::KwAs},
        // Misc
        {"true", TokenKind::KwTrue}, {"false", TokenKind::KwFalse},
        {"null", TokenKind::KwNull}, {"defer", TokenKind::KwDefer},
        {"try", TokenKind::KwTry}, {"catch", TokenKind::KwCatch},
        {"new", TokenKind::KwNew},
    };
    return kTable;
}

bool is_ident_start(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}
bool is_ident_cont(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}
bool is_digit(char c) { return c >= '0' && c <= '9'; }

Token make(TokenKind k, std::string_view lex, std::uint32_t line,
           std::uint32_t col) {
    return Token{k, lex, line, col};
}

TokenKind resolve_keyword(std::string_view text) {
    auto it = keyword_table().find(text);
    if (it != keyword_table().end()) return it->second;
    return TokenKind::Identifier;
}

}  // namespace

std::vector<Token> lex(const std::string& source, const std::string& path) {
    (void)path;
    std::vector<Token> tokens;
    tokens.reserve(source.size() / 4 + 16);

    std::uint32_t line = 1;
    std::uint32_t col = 1;
    std::size_t i = 0;
    const std::size_t n = source.size();

    auto here = [&](TokenKind k, std::string_view lex, std::size_t start) {
        return make(k, lex, line, col);
    };

    while (i < n) {
        char c = source[i];

        // Newline
        if (c == '\n') {
            ++line;
            col = 1;
            ++i;
            continue;
        }
        // Whitespace
        if (c == ' ' || c == '\t' || c == '\r') {
            ++i;
            ++col;
            continue;
        }

        // Line comment //...
        if (c == '/' && i + 1 < n && source[i + 1] == '/') {
            while (i < n && source[i] != '\n') ++i;
            continue;
        }
        // Block comment /* ... */
        if (c == '/' && i + 1 < n && source[i + 1] == '*') {
            std::size_t start_line = line;
            std::size_t start_col = col;
            i += 2;
            col += 2;
            while (i < n && !(source[i] == '*' && i + 1 < n && source[i + 1] == '/')) {
                if (source[i] == '\n') {
                    ++line;
                    col = 1;
                } else {
                    ++col;
                }
                ++i;
            }
            if (i >= n) {
                tokens.push_back(make(TokenKind::Unknown,
                                      std::string_view(source.data() + start_line * 0, 0),
                                      start_line, start_col));
            } else {
                i += 2;
                col += 2;
            }
            continue;
        }

        // Identifier / keyword
        if (is_ident_start(c)) {
            std::size_t start = i;
            std::uint32_t sc = col;
            while (i < n && is_ident_cont(source[i])) {
                ++i;
                ++col;
            }
            std::string_view text(source.data() + start, i - start);
            TokenKind k = resolve_keyword(text);
            tokens.push_back(make(k, text, line, sc));
            continue;
        }

        // Number literal (integer or float)
        if (is_digit(c)) {
            std::size_t start = i;
            std::uint32_t sc = col;
            bool is_float = false;
            while (i < n && (is_digit(source[i]) || source[i] == '_')) {
                ++i;
                ++col;
            }
            if (i < n && source[i] == '.') {
                if (i + 1 < n && is_digit(source[i + 1])) {
                    is_float = true;
                    ++i;
                    ++col;
                    while (i < n && (is_digit(source[i]) || source[i] == '_')) {
                        ++i;
                        ++col;
                    }
                }
            }
            if (i < n && (source[i] == 'e' || source[i] == 'E')) {
                is_float = true;
                ++i;
                ++col;
                if (i < n && (source[i] == '+' || source[i] == '-')) {
                    ++i;
                    ++col;
                }
                while (i < n && is_digit(source[i])) {
                    ++i;
                    ++col;
                }
            }
            if (i < n && (source[i] == 'f' || source[i] == 'F' ||
                          source[i] == 'd' || source[i] == 'D' ||
                          source[i] == 'u' || source[i] == 'U' ||
                          source[i] == 'i' || source[i] == 'I' ||
                          source[i] == 'l' || source[i] == 'L')) {
                ++i;
                ++col;
            }
            if (i < n && (source[i] == 'x' || source[i] == 'X' ||
                          source[i] == 'b' || source[i] == 'B' ||
                          source[i] == 'o' || source[i] == 'O')) {
                ++i;
                ++col;
            }
            std::string_view text(source.data() + start, i - start);
            tokens.push_back(make(is_float ? TokenKind::FloatLiteral
                                           : TokenKind::IntegerLiteral,
                                  text, line, sc));
            continue;
        }

        // Hex / binary / octal literal: 0x.., 0b.., 0o..
        if (c == '0' && i + 1 < n &&
            (source[i + 1] == 'x' || source[i + 1] == 'X' ||
             source[i + 1] == 'b' || source[i + 1] == 'B' ||
             source[i + 1] == 'o' || source[i + 1] == 'O')) {
            std::size_t start = i;
            std::uint32_t sc = col;
            i += 2;
            col += 2;
            while (i < n && (is_digit(source[i]) ||
                             (source[i] >= 'a' && source[i] <= 'f') ||
                             (source[i] >= 'A' && source[i] <= 'F') ||
                             source[i] == '_')) {
                ++i;
                ++col;
            }
            std::string_view text(source.data() + start, i - start);
            tokens.push_back(make(TokenKind::IntegerLiteral, text, line, sc));
            continue;
        }

        // String literal
        if (c == '"') {
            std::size_t start = i;
            std::uint32_t sc = col;
            ++i;
            ++col;
            while (i < n && source[i] != '"') {
                if (source[i] == '\\' && i + 1 < n) {
                    i += 2;
                    col += 2;
                } else {
                    if (source[i] == '\n') {
                        ++line;
                        col = 1;
                    } else {
                        ++col;
                    }
                    ++i;
                }
            }
            if (i < n) {
                ++i;
                ++col;
            }
            std::string_view text(source.data() + start, i - start);
            tokens.push_back(make(TokenKind::StringLiteral, text, line, sc));
            continue;
        }

        // Char literal
        if (c == '\'') {
            std::size_t start = i;
            std::uint32_t sc = col;
            ++i;
            ++col;
            while (i < n && source[i] != '\'') {
                if (source[i] == '\\' && i + 1 < n) {
                    i += 2;
                    col += 2;
                } else {
                    ++i;
                    ++col;
                }
            }
            if (i < n) {
                ++i;
                ++col;
            }
            std::string_view text(source.data() + start, i - start);
            tokens.push_back(make(TokenKind::CharLiteral, text, line, sc));
            continue;
        }

        // Punctuation / operators — peek for 2 / 3 char sequences
        std::uint32_t sc = col;
        char a = c;
        char b = i + 1 < n ? source[i + 1] : '\0';
        char cc = i + 2 < n ? source[i + 2] : '\0';

        auto push2 = [&](TokenKind k, std::size_t len) {
            tokens.push_back(make(k, std::string_view(source.data() + i, len),
                                  line, sc));
            i += len;
            col += len;
        };

        if (a == '(') { push2(TokenKind::LParen, 1); continue; }
        if (a == ')') { push2(TokenKind::RParen, 1); continue; }
        if (a == '{') { push2(TokenKind::LBrace, 1); continue; }
        if (a == '}') { push2(TokenKind::RBrace, 1); continue; }
        if (a == '[') { push2(TokenKind::LBracket, 1); continue; }
        if (a == ']') { push2(TokenKind::RBracket, 1); continue; }
        if (a == ',') { push2(TokenKind::Comma, 1); continue; }
        if (a == ';') { push2(TokenKind::Semicolon, 1); continue; }
        if (a == '?') { push2(TokenKind::Question, 1); continue; }
        if (a == '@') { push2(TokenKind::At, 1); continue; }
        if (a == '~') { push2(TokenKind::Tilde, 1); continue; }
        if (a == '^') {
            if (b == '=') push2(TokenKind::CaretAssign, 2);
            else push2(TokenKind::Caret, 1);
            continue;
        }
        if (a == ':') {
            if (b == ':') push2(TokenKind::DoubleColon, 2);
            else push2(TokenKind::Colon, 1);
            continue;
        }
        if (a == '.') {
            if (b == '.') {
                if (cc == '.') push2(TokenKind::Ellipsis, 3);
                else push2(TokenKind::DoubleDot, 2);
                continue;
            }
            push2(TokenKind::Dot, 1);
            continue;
        }
        if (a == ',') { push2(TokenKind::Comma, 1); continue; }
        if (a == '+') {
            if (b == '+') push2(TokenKind::PlusPlus, 2);
            else if (b == '=') push2(TokenKind::PlusAssign, 2);
            else push2(TokenKind::Plus, 1);
            continue;
        }
        if (a == '-') {
            if (b == '-') push2(TokenKind::MinusMinus, 2);
            else if (b == '=') push2(TokenKind::MinusAssign, 2);
            else if (b == '>') push2(TokenKind::Arrow, 2);
            else push2(TokenKind::Minus, 1);
            continue;
        }
        if (a == '*') {
            if (b == '=') push2(TokenKind::StarAssign, 2);
            else push2(TokenKind::Star, 1);
            continue;
        }
        if (a == '/') {
            if (b == '=') push2(TokenKind::SlashAssign, 2);
            else push2(TokenKind::Slash, 1);
            continue;
        }
        if (a == '%') {
            if (b == '=') push2(TokenKind::PercentAssign, 2);
            else push2(TokenKind::Percent, 1);
            continue;
        }
        if (a == '&') {
            if (b == '&') push2(TokenKind::AmpAmp, 2);
            else if (b == '=') push2(TokenKind::AmpAssign, 2);
            else push2(TokenKind::Amp, 1);
            continue;
        }
        if (a == '|') {
            if (b == '|') push2(TokenKind::PipePipe, 2);
            else if (b == '=') push2(TokenKind::PipeAssign, 2);
            else push2(TokenKind::Pipe, 1);
            continue;
        }
        if (a == '!') {
            if (b == '=') push2(TokenKind::NotEq, 2);
            else push2(TokenKind::Bang, 1);
            continue;
        }
        if (a == '=') {
            if (b == '=') push2(TokenKind::Eq, 2);
            else if (b == '>') push2(TokenKind::FatArrow, 2);
            else push2(TokenKind::Assign, 1);
            continue;
        }
        if (a == '<') {
            if (b == '=') push2(TokenKind::LtEq, 2);
            else if (b == '<') {
                if (cc == '=') push2(TokenKind::LtLtAssign, 3);
                else push2(TokenKind::LtLt, 2);
                continue;
            }
            push2(TokenKind::LAngle, 1);
            continue;
        }
        if (a == '>') {
            if (b == '=') push2(TokenKind::GtEq, 2);
            else if (b == '>') {
                if (cc == '=') push2(TokenKind::GtGtAssign, 3);
                else push2(TokenKind::GtGt, 2);
                continue;
            }
            push2(TokenKind::RAngle, 1);
            continue;
        }

        // Unknown character
        tokens.push_back(make(TokenKind::Unknown,
                              std::string_view(source.data() + i, 1), line, sc));
        ++i;
        ++col;
    }

    tokens.push_back(make(TokenKind::Eof, std::string_view{}, line, col));
    return tokens;
}
}
