// Token — the unit produced by the lexer and consumed by the parser.
//
// A Token carries (kind, lexeme, line, column). Lexeme is a non-owning
// std::string_view into the original source buffer, so the source string
// MUST outlive every Token in the stream. The lexer is the only producer
// and main.cpp owns the buffer for the lifetime of one compile, so this
// is safe by construction — change it carefully.
//
// TokenKind is intentionally one big enum: the lexer dispatches on a
// hot switch, and grouping (literals / punctuation / operators /
// keywords) is by source order rather than runtime-checked categories.
#pragma once
#include <cstdint>
// #include <string>
#include <string_view>
namespace caret::frontend {
// uint16_t leaves headroom for new keywords/operators without bloating
// the Token struct beyond 24 bytes on a 64-bit build.
enum class TokenKind : std::uint16_t {
    Unknown,      // any byte the lexer could not classify — surfaces as a parse error
    Eof,          // synthetic sentinel; always the last token in the stream

    // === Literals ===
    Identifier,
    IntegerLiteral,
    FloatLiteral,
    StringLiteral,
    CharLiteral,

    // === Punctuation ===
    LParen,        // (
    RParen,        // )
    LBrace,        // {
    RBrace,        // }
    LBracket,      // [
    RBracket,      // ]
    LAngle,        // <
    RAngle,        // >
    Comma,         // ,
    Semicolon,     // ;
    Colon,         // :
    DoubleColon,   // ::
    Dot,           // .
    Arrow,         // ->
    FatArrow,      // =>     (match arms)
    DoubleDot,     // ..     (range — also doubles as the C^ diagnostic separator)
    Ellipsis,      // ...    (variadic params)
    Question,      // ?      (optional type / ternary)
    At,            // @      (builtin prefix, e.g. @sizeof)

    // === Operators ===
    Assign,        // =
    Plus,          // +
    Minus,         // -
    Star,          // *
    Slash,         // /
    Percent,       // %
    PlusPlus,      // ++
    MinusMinus,    // --

    // Compound assignment. Kept distinct from their bare-op counterparts
    // so the parser/backend never have to peek for a trailing `=`.
    PlusAssign,    // +=
    MinusAssign,   // -=
    StarAssign,    // *=
    SlashAssign,   // /=
    PercentAssign, // %=
    AmpAssign,     // &=
    PipeAssign,    // |=
    CaretAssign,   // ^=
    LtLtAssign,    // <<=
    GtGtAssign,    // >>=

    Amp,           // &      (also: address-of, bitwise-and, reference sigil)
    AmpAmp,        // &&
    Pipe,          // |
    PipePipe,      // ||
    Caret,         // ^
    Tilde,         // ~
    Bang,          // !      (also: error-union sigil)

    Eq,            // ==
    NotEq,         // !=
    LtEq,          // <=
    GtEq,          // >=
    LtLt,          // <<
    GtGt,          // >>

    // === Keywords — primitive types ===
    // See syntax/fundamental.md §2 for the canonical bit-width matrix.
    KwI8, KwI16, KwI32, KwI64, KwI128, KwISize,
    KwU8, KwU16, KwU32, KwU64, KwU128, KwUSize,
    KwF32, KwF64,
    KwBool, KwChar, KwVoid, KwString, KwAuto,

    // === Keywords — storage / modifiers ===
    KwMut, KwConst, KwPacked, KwVolatile, KwExport, KwExtern,

    // === Keywords — declarations ===
    KwStruct, KwEnum, KwUnion, KwError, KwType, KwFn,

    // === Keywords — control flow ===
    KwIf, KwElse, KwElif,
    KwFor, KwWhile, KwLoop,
    KwMatch, KwBreak, KwContinue, KwReturn,

    // === Keywords — module / import ===
    KwImport, KwLink, KwAs,

    // === Keywords — misc ===
    // KwReturnBang is reserved for a future `return!` form; the lexer
    // does not produce it yet but the slot is kept stable for ABI.
    KwTrue, KwFalse, KwNull, KwDefer, KwTry, KwCatch, KwNew, KwReturnBang,
};

// Token is a value type. Copy it freely; the std::string_view inside
// keeps the lexeme as a slice of the source buffer (zero allocation).
struct Token {
    TokenKind kind{TokenKind::Unknown};
    std::string_view lexeme;
    std::uint32_t line{1};   // 1-based, matches diagnostic format
    std::uint32_t column{1}; // 1-based, points at the first byte of `lexeme`
};

// token_kind_name: a short human label used inside diagnostic messages
// (e.g. "expected `;`, found identifier"). Punctuation kinds return the
// literal spelling wrapped in backticks; categories return a generic
// word ("identifier", "integer literal", ...).
const char* token_kind_name(TokenKind k);

// token_kind_punct: the bare textual spelling of a punctuation/operator
// kind (no backticks). Returns the empty string for non-punctuation
// kinds. Useful when reconstructing source or building suggestions.
const char* token_kind_punct(TokenKind k);
}
