// this is the caretc a compiler for the C^
#include "token.hpp"

namespace caret::frontend {
const char* token_kind_name(TokenKind k) {
    switch (k) {
    case TokenKind::Unknown: return "unknown";
    case TokenKind::Eof: return "eof";
    case TokenKind::Identifier: return "identifier";
    case TokenKind::IntegerLiteral: return "integer literal";
    case TokenKind::FloatLiteral: return "float literal";
    case TokenKind::StringLiteral: return "string literal";
    case TokenKind::CharLiteral: return "char literal";
    case TokenKind::LParen: return "`(`";
    case TokenKind::RParen: return "`)`";
    case TokenKind::LBrace: return "`{`";
    case TokenKind::RBrace: return "`}`";
    case TokenKind::LBracket: return "`[`";
    case TokenKind::RBracket: return "`]`";
    case TokenKind::LAngle: return "`<`";
    case TokenKind::RAngle: return "`>`";
    case TokenKind::Comma: return "`,`";
    case TokenKind::Semicolon: return "`;`";
    case TokenKind::Colon: return "`:`";
    case TokenKind::DoubleColon: return "`::`";
    case TokenKind::Dot: return "`.`";
    case TokenKind::Arrow: return "`->`";
    case TokenKind::FatArrow: return "`=>`";
    case TokenKind::DoubleDot: return "`..`";
    case TokenKind::Ellipsis: return "`...`";
    case TokenKind::Question: return "`?`";
    case TokenKind::At: return "`@`";
    case TokenKind::Assign: return "`=`";
    case TokenKind::Plus: return "`+`";
    case TokenKind::Minus: return "`-`";
    case TokenKind::Star: return "`*`";
    case TokenKind::Slash: return "`/`";
    case TokenKind::Percent: return "`%`";
    case TokenKind::PlusPlus: return "`++`";
    case TokenKind::MinusMinus: return "`--`";
    case TokenKind::PlusAssign: return "`+=`";
    case TokenKind::MinusAssign: return "`-=`";
    case TokenKind::StarAssign: return "`*=`";
    case TokenKind::SlashAssign: return "`/=`";
    case TokenKind::PercentAssign: return "`%=`";
    case TokenKind::AmpAssign: return "`&=`";
    case TokenKind::PipeAssign: return "`|=`";
    case TokenKind::CaretAssign: return "`^=`";
    case TokenKind::LtLtAssign: return "`<<=`";
    case TokenKind::GtGtAssign: return "`>>=`";
    case TokenKind::Amp: return "`&`";
    case TokenKind::AmpAmp: return "`&&`";
    case TokenKind::Pipe: return "`|`";
    case TokenKind::PipePipe: return "`||`";
    case TokenKind::Caret: return "`^`";
    case TokenKind::Tilde: return "`~`";
    case TokenKind::Bang: return "`!`";
    case TokenKind::Eq: return "`==`";
    case TokenKind::NotEq: return "`!=`";
    case TokenKind::LtEq: return "`<=`";
    case TokenKind::GtEq: return "`>=`";
    case TokenKind::LtLt: return "`<<`";
    case TokenKind::GtGt: return "`>>`";
    default: return "keyword";
    }
}

const char* token_kind_punct(TokenKind k) {
    switch (k) {
    case TokenKind::LParen: return "(";
    case TokenKind::RParen: return ")";
    case TokenKind::LBrace: return "{";
    case TokenKind::RBrace: return "}";
    case TokenKind::LBracket: return "[";
    case TokenKind::RBracket: return "]";
    case TokenKind::LAngle: return "<";
    case TokenKind::RAngle: return ">";
    case TokenKind::Comma: return ",";
    case TokenKind::Semicolon: return ";";
    case TokenKind::Colon: return ":";
    case TokenKind::DoubleColon: return "::";
    case TokenKind::Dot: return ".";
    case TokenKind::Arrow: return "->";
    case TokenKind::FatArrow: return "=>";
    case TokenKind::DoubleDot: return "..";
    case TokenKind::Ellipsis: return "...";
    case TokenKind::Question: return "?";
    case TokenKind::At: return "@";
    case TokenKind::Assign: return "=";
    case TokenKind::Plus: return "+";
    case TokenKind::Minus: return "-";
    case TokenKind::Star: return "*";
    case TokenKind::Slash: return "/";
    case TokenKind::Percent: return "%";
    case TokenKind::PlusPlus: return "++";
    case TokenKind::MinusMinus: return "--";
    case TokenKind::PlusAssign: return "+=";
    case TokenKind::MinusAssign: return "-=";
    case TokenKind::StarAssign: return "*=";
    case TokenKind::SlashAssign: return "/=";
    case TokenKind::PercentAssign: return "%=";
    case TokenKind::AmpAssign: return "&=";
    case TokenKind::PipeAssign: return "|=";
    case TokenKind::CaretAssign: return "^=";
    case TokenKind::LtLtAssign: return "<<=";
    case TokenKind::GtGtAssign: return ">>=";
    case TokenKind::Amp: return "&";
    case TokenKind::AmpAmp: return "&&";
    case TokenKind::Pipe: return "|";
    case TokenKind::PipePipe: return "||";
    case TokenKind::Caret: return "^";
    case TokenKind::Tilde: return "~";
    case TokenKind::Bang: return "!";
    case TokenKind::Eq: return "==";
    case TokenKind::NotEq: return "!=";
    case TokenKind::LtEq: return "<=";
    case TokenKind::GtEq: return ">=";
    case TokenKind::LtLt: return "<<";
    case TokenKind::GtGt: return ">>";
    default: return "";
    }
}
}
