// this is the caretc a compiler for the C^
#pragma once
#include <cstdint>
#include <string>
#include <string_view>
namespace caret::frontend {
enum class TokenKind : std::uint16_t {
    Unknown,
    Eof,

    // Literals
    Identifier,
    IntegerLiteral,
    FloatLiteral,
    StringLiteral,
    CharLiteral,

    // Punctuation
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
    FatArrow,      // =>
    DoubleDot,     // ..
    Ellipsis,      // ...
    Question,      // ?
    At,            // @

    // Operators
    Assign,        // =
    Plus,          // +
    Minus,         // -
    Star,          // *
    Slash,         // /
    Percent,       // %
    PlusPlus,      // ++
    MinusMinus,    // --

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

    Amp,           // &
    AmpAmp,        // &&
    Pipe,          // |
    PipePipe,      // ||
    Caret,         // ^
    Tilde,         // ~
    Bang,          // !

    Eq,            // ==
    NotEq,         // !=
    LtEq,          // <=
    GtEq,          // >=
    LtLt,          // <<
    GtGt,          // >>

    // Keywords — primitive types
    KwI8, KwI16, KwI32, KwI64, KwI128, KwISize,
    KwU8, KwU16, KwU32, KwU64, KwU128, KwUSize,
    KwF32, KwF64,
    KwBool, KwChar, KwVoid, KwString, KwAuto,

    // Keywords — storage / modifiers
    KwMut, KwConst, KwPacked, KwVolatile, KwExport, KwExtern,

    // Keywords — declarations
    KwStruct, KwEnum, KwUnion, KwError, KwType, KwFn,

    // Keywords — control flow
    KwIf, KwElse, KwElif,
    KwFor, KwWhile, KwLoop,
    KwMatch, KwBreak, KwContinue, KwReturn,

    // Keywords — module / import
    KwImport, KwLink, KwAs,

    // Keywords — misc
    KwTrue, KwFalse, KwNull, KwDefer, KwTry, KwCatch, KwNew, KwReturnBang,
};

struct Token {
    TokenKind kind{TokenKind::Unknown};
    std::string_view lexeme;
    std::uint32_t line{1};
    std::uint32_t column{1};
};

const char* token_kind_name(TokenKind k);
const char* token_kind_punct(TokenKind k);
}
