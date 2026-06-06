// Lexer implementation.
//
// Design notes:
//   * Single forward scan over the source buffer; no backtracking.
//   * Lexemes are returned as std::string_view slices into `source`,
//     so the caller's buffer MUST outlive every token (see lexer.hpp).
//   * Whitespace and comments are skipped silently — they do not appear
//     in the token stream. Newlines bump the line counter and reset col.
//   * Unknown bytes become a TokenKind::Unknown token rather than an
//     immediate error: this keeps recovery cheap and lets the parser
//     report the problem with surrounding context.
//   * The hot loop dispatches on the first character; multi-char
//     operators peek up to two bytes ahead via `b` / `cc`.
#include "lexer.hpp"
#include <cctype>
// #include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace caret::frontend {

// Return the 1-based `line` from `text`, without the trailing newline.
// Used by the diagnostic renderer; the lexer itself doesn't call this.
// Linear in the file length but only invoked when a diagnostic actually
// fires, so the cost is bounded by the number of errors.
std::string line_text(const std::string &text, std::uint32_t line) {
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
  // Last line of a file with no trailing newline.
  if (cur == line && start < text.size()) {
    return text.substr(start);
  }
  return {};
}

namespace {

// Keyword table. Built once via a function-local static so it survives
// for the program lifetime without a global ctor. The unordered_map
// gives us O(1) lookup keyed on a string_view (so no allocation on the
// hot path). Keep this list in sync with TokenKind in token.hpp; the
// lexer is the single point of truth for what an identifier becomes
// when it matches a known keyword.
const std::unordered_map<std::string_view, TokenKind> &keyword_table() {
  static const std::unordered_map<std::string_view, TokenKind> kTable = {
      // Primitive types — see syntax/fundamental.md §2.
      {"i8", TokenKind::KwI8},
      {"i16", TokenKind::KwI16},
      {"i32", TokenKind::KwI32},
      {"i64", TokenKind::KwI64},
      {"i128", TokenKind::KwI128},
      {"isize", TokenKind::KwISize},
      {"u8", TokenKind::KwU8},
      {"u16", TokenKind::KwU16},
      {"u32", TokenKind::KwU32},
      {"u64", TokenKind::KwU64},
      {"u128", TokenKind::KwU128},
      {"usize", TokenKind::KwUSize},
      {"f32", TokenKind::KwF32},
      {"f64", TokenKind::KwF64},
      {"bool", TokenKind::KwBool},
      {"char", TokenKind::KwChar},
      {"void", TokenKind::KwVoid},
      {"string", TokenKind::KwString},
      {"auto", TokenKind::KwAuto},
      // Storage class / modifiers.
      {"mut", TokenKind::KwMut},
      {"const", TokenKind::KwConst},
      {"packed", TokenKind::KwPacked},
      {"volatile", TokenKind::KwVolatile},
      {"export", TokenKind::KwExport},
      {"extern", TokenKind::KwExtern},
      // Top-level declaration introducers.
      {"struct", TokenKind::KwStruct},
      {"enum", TokenKind::KwEnum},
      {"union", TokenKind::KwUnion},
      {"error", TokenKind::KwError},
      {"type", TokenKind::KwType},
      {"fn", TokenKind::KwFn},
      // Control flow.
      {"if", TokenKind::KwIf},
      {"else", TokenKind::KwElse},
      {"elif", TokenKind::KwElif},
      {"for", TokenKind::KwFor},
      {"while", TokenKind::KwWhile},
      {"loop", TokenKind::KwLoop},
      {"match", TokenKind::KwMatch},
      {"break", TokenKind::KwBreak},
      {"continue", TokenKind::KwContinue},
      {"return", TokenKind::KwReturn},
      // Module / imports.
      {"import", TokenKind::KwImport},
      {"link", TokenKind::KwLink},
      {"as", TokenKind::KwAs},
      // Literal-ish keywords and error-handling sugar.
      {"true", TokenKind::KwTrue},
      {"false", TokenKind::KwFalse},
      {"null", TokenKind::KwNull},
      {"defer", TokenKind::KwDefer},
      {"try", TokenKind::KwTry},
      {"catch", TokenKind::KwCatch},
      {"new", TokenKind::KwNew},
  };
  return kTable;
}

// is_ident_start / is_ident_cont follow the conservative C rule: ASCII
// letter or underscore for the first byte, plus digits for subsequent
// bytes. Unicode identifiers are deferred to a later milestone — none
// of the bootstrap syntax needs them.
bool is_ident_start(char c) {
  return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}
bool is_ident_cont(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}
bool is_digit(char c) { return c >= '0' && c <= '9'; }

// Small constructor helper so the call sites read like
// `tokens.push_back(make(Kind, lex, line, col))` instead of a brace
// initializer.
Token make(TokenKind k, std::string_view lex, std::uint32_t line,
           std::uint32_t col) {
  return Token{k, lex, line, col};
}

// Resolve an identifier-shaped lexeme to a keyword kind if it matches,
// otherwise fall back to TokenKind::Identifier. Lookup is keyed on a
// string_view, so we never allocate on a hit.
TokenKind resolve_keyword(std::string_view text) {
  auto it = keyword_table().find(text);
  if (it != keyword_table().end())
    return it->second;
  return TokenKind::Identifier;
}

} // namespace

std::vector<Token> lex(const std::string &source, const std::string &path) {
  // `path` is informational; the parser carries it in diagnostics.
  (void)path;
  std::vector<Token> tokens;
  // Heuristic reserve: ~one token per 4 source bytes plus a small floor
  // for tiny inputs. Avoids the early reallocation storm.
  tokens.reserve(source.size() / 4 + 16);

  std::uint32_t line = 1;
  std::uint32_t col = 1;
  std::size_t i = 0;
  const std::size_t n = source.size();

  while (i < n) {
    char c = source[i];

    // --- Whitespace / line tracking ---------------------------------
    // Newline first because it has side effects (line++, col reset).
    if (c == '\n') {
      ++line;
      col = 1;
      ++i;
      continue;
    }
    if (c == ' ' || c == '\t' || c == '\r') {
      ++i;
      ++col;
      continue;
    }

    // --- Comments ----------------------------------------------------
    // Line comment `// ...` — consume up to (but not including) the \n.
    // The newline handling above will then bump the line counter.
    if (c == '/' && i + 1 < n && source[i + 1] == '/') {
      while (i < n && source[i] != '\n')
        ++i;
      continue;
    }
    // Block comment `/* ... */` — does NOT nest in v0.1.0 (mirroring C).
    // We track line/col through the body so a syntax error inside the
    // block reports a sensible location.
    if (c == '/' && i + 1 < n && source[i + 1] == '*') {
      std::size_t start_line = line;
      std::size_t start_col = col;
      i += 2;
      col += 2;
      while (i < n &&
             !(source[i] == '*' && i + 1 < n && source[i + 1] == '/')) {
        if (source[i] == '\n') {
          ++line;
          col = 1;
        } else {
          ++col;
        }
        ++i;
      }
      if (i >= n) {
        // Unterminated /* ... */ — emit an Unknown token at the start
        // of the comment so the parser can report it with location.
        tokens.push_back(
            make(TokenKind::Unknown,
                 std::string_view(source.data() + start_line * 0, 0),
                 start_line, start_col));
      } else {
        i += 2;
        col += 2;
      }
      continue;
    }

    // --- Identifiers and keywords -----------------------------------
    // First byte must be alpha/underscore; subsequent bytes may include
    // digits. Final lexeme is looked up in the keyword table.
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

    // --- Non-decimal integer literals: 0x.., 0b.., 0o.. -------------
    // This branch MUST run before the generic digit scanner, otherwise
    // the `x` / `b` / `o` would be eaten as a (nonexistent) integer
    // suffix by the decimal path below.
    if (c == '0' && i + 1 < n &&
        (source[i + 1] == 'x' || source[i + 1] == 'X' || source[i + 1] == 'b' ||
         source[i + 1] == 'B' || source[i + 1] == 'o' ||
         source[i + 1] == 'O')) {
      std::size_t start = i;
      std::uint32_t sc = col;
      i += 2;
      col += 2;
      // Accept hex digits plus underscore separators; the parser
      // validates the actual digit set per base (`0b101_010` etc.).
      while (i < n &&
             (is_digit(source[i]) || (source[i] >= 'a' && source[i] <= 'f') ||
              (source[i] >= 'A' && source[i] <= 'F') || source[i] == '_')) {
        ++i;
        ++col;
      }
      std::string_view text(source.data() + start, i - start);
      tokens.push_back(make(TokenKind::IntegerLiteral, text, line, sc));
      continue;
    }

    // --- Decimal numbers (integer or float) -------------------------
    // Grammar (informal):
    //   number  := digits ('.' digits)? (('e'|'E') ('+'|'-')? digits)? suffix?
    //   suffix  := one of {f F d D u U i I l L}
    // We promote to FloatLiteral the moment we see a fractional or
    // exponent part. `_` is allowed as a digit separator everywhere.
    if (is_digit(c)) {
      std::size_t start = i;
      std::uint32_t sc = col;
      bool is_float = false;
      while (i < n && (is_digit(source[i]) || source[i] == '_')) {
        ++i;
        ++col;
      }
      // Fractional part: only if there's at least one digit after the dot.
      // `1.` standalone could be ambiguous with `1 .. 5` style ranges in
      // the future; this rule keeps things predictable today.
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
      // Exponent part: 1e10, 1.5e-3, etc.
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
      // Single-character type suffix (f/F/d/D/u/U/i/I/l/L). We accept
      // it eagerly and let the parser decide what it means in context.
      if (i < n && (source[i] == 'f' || source[i] == 'F' || source[i] == 'd' ||
                    source[i] == 'D' || source[i] == 'u' || source[i] == 'U' ||
                    source[i] == 'i' || source[i] == 'I' || source[i] == 'l' ||
                    source[i] == 'L')) {
        ++i;
        ++col;
      }
      std::string_view text(source.data() + start, i - start);
      tokens.push_back(
          make(is_float ? TokenKind::FloatLiteral : TokenKind::IntegerLiteral,
               text, line, sc));
      continue;
    }

    // --- String literal `"..."` -------------------------------------
    // Backslash escapes are passed through verbatim; the parser
    // unescapes them. Strings may span lines today — the line counter
    // is updated as we walk through them so error locations stay sane.
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

    // --- Char literal `'x'` -----------------------------------------
    // Same escape-passthrough rule as strings. We do not validate the
    // body length here; the parser owns char-literal semantics.
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

    // --- Punctuation and operators ----------------------------------
    // Peek up to two bytes ahead so we can resolve 2- and 3-character
    // operators (`==`, `<<=`, `...`) in a single decision.
    std::uint32_t sc = col;
    char a = c;
    char b = i + 1 < n ? source[i + 1] : '\0';
    char cc = i + 2 < n ? source[i + 2] : '\0';

    // Local helper: push the token, advance the cursor by `len` bytes
    // (assumes the matched lexeme is ASCII and contains no newlines —
    // true for every operator in the language).
    auto push2 = [&](TokenKind k, std::size_t len) {
      tokens.push_back(
          make(k, std::string_view(source.data() + i, len), line, sc));
      i += len;
      col += len;
    };

    if (a == '(') {
      push2(TokenKind::LParen, 1);
      continue;
    }
    if (a == ')') {
      push2(TokenKind::RParen, 1);
      continue;
    }
    if (a == '{') {
      push2(TokenKind::LBrace, 1);
      continue;
    }
    if (a == '}') {
      push2(TokenKind::RBrace, 1);
      continue;
    }
    if (a == '[') {
      push2(TokenKind::LBracket, 1);
      continue;
    }
    if (a == ']') {
      push2(TokenKind::RBracket, 1);
      continue;
    }
    if (a == ',') {
      push2(TokenKind::Comma, 1);
      continue;
    }
    if (a == ';') {
      push2(TokenKind::Semicolon, 1);
      continue;
    }
    if (a == '?') {
      push2(TokenKind::Question, 1);
      continue;
    }
    if (a == '@') {
      push2(TokenKind::At, 1);
      continue;
    }
    if (a == '~') {
      push2(TokenKind::Tilde, 1);
      continue;
    }
    if (a == '^') {
      // `^` is bitwise-xor; `^=` is compound assignment. There is no
      // `^^` operator in C^.
      if (b == '=')
        push2(TokenKind::CaretAssign, 2);
      else
        push2(TokenKind::Caret, 1);
      continue;
    }
    if (a == ':') {
      // `::` is the module path separator (e.g. `std::io`).
      if (b == ':')
        push2(TokenKind::DoubleColon, 2);
      else
        push2(TokenKind::Colon, 1);
      continue;
    }
    if (a == '.') {
      // `.`  -> field access / postfix-deref half (the `*` half is
      //         consumed by the parser).
      // `..` -> range / diagnostic separator.
      // `...`-> ellipsis (variadic args).
      if (b == '.') {
        if (cc == '.')
          push2(TokenKind::Ellipsis, 3);
        else
          push2(TokenKind::DoubleDot, 2);
        continue;
      }
      push2(TokenKind::Dot, 1);
      continue;
    }
    if (a == ',') {
      // Defensive duplicate of the `,` arm above. Cheap and removes
      // any doubt for future refactors that might move the earlier arm.
      push2(TokenKind::Comma, 1);
      continue;
    }
    if (a == '+') {
      if (b == '+')
        push2(TokenKind::PlusPlus, 2);
      else if (b == '=')
        push2(TokenKind::PlusAssign, 2);
      else
        push2(TokenKind::Plus, 1);
      continue;
    }
    if (a == '-') {
      // `->` is the function-type arrow used in trait signatures.
      if (b == '-')
        push2(TokenKind::MinusMinus, 2);
      else if (b == '=')
        push2(TokenKind::MinusAssign, 2);
      else if (b == '>')
        push2(TokenKind::Arrow, 2);
      else
        push2(TokenKind::Minus, 1);
      continue;
    }
    if (a == '*') {
      // `*` is also the pointer sigil and the multiplication operator;
      // disambiguation happens in the parser based on grammar context.
      if (b == '=')
        push2(TokenKind::StarAssign, 2);
      else
        push2(TokenKind::Star, 1);
      continue;
    }
    if (a == '/') {
      // Note: `//` and `/*` are handled above (comment forms) before
      // we ever reach this arm, so we only see division here.
      if (b == '=')
        push2(TokenKind::SlashAssign, 2);
      else
        push2(TokenKind::Slash, 1);
      continue;
    }
    if (a == '%') {
      if (b == '=')
        push2(TokenKind::PercentAssign, 2);
      else
        push2(TokenKind::Percent, 1);
      continue;
    }
    if (a == '&') {
      // `&` is bitwise-and, address-of, AND the reference sigil. The
      // parser picks the right meaning per context.
      if (b == '&')
        push2(TokenKind::AmpAmp, 2);
      else if (b == '=')
        push2(TokenKind::AmpAssign, 2);
      else
        push2(TokenKind::Amp, 1);
      continue;
    }
    if (a == '|') {
      if (b == '|')
        push2(TokenKind::PipePipe, 2);
      else if (b == '=')
        push2(TokenKind::PipeAssign, 2);
      else
        push2(TokenKind::Pipe, 1);
      continue;
    }
    if (a == '!') {
      // `!` is logical-not AND the error-union sigil (e.g. `i32!Err`).
      if (b == '=')
        push2(TokenKind::NotEq, 2);
      else
        push2(TokenKind::Bang, 1);
      continue;
    }
    if (a == '=') {
      // `=>` is the match-arm arrow.
      if (b == '=')
        push2(TokenKind::Eq, 2);
      else if (b == '>')
        push2(TokenKind::FatArrow, 2);
      else
        push2(TokenKind::Assign, 1);
      continue;
    }
    if (a == '<') {
      // `<<` shift, `<<=` compound shift-assign, `<=` compare,
      // bare `<` opens an angle bracket / less-than.
      if (b == '=')
        push2(TokenKind::LtEq, 2);
      else if (b == '<') {
        if (cc == '=')
          push2(TokenKind::LtLtAssign, 3);
        else
          push2(TokenKind::LtLt, 2);
        continue;
      }
      push2(TokenKind::LAngle, 1);
      continue;
    }
    if (a == '>') {
      if (b == '=')
        push2(TokenKind::GtEq, 2);
      else if (b == '>') {
        if (cc == '=')
          push2(TokenKind::GtGtAssign, 3);
        else
          push2(TokenKind::GtGt, 2);
        continue;
      }
      push2(TokenKind::RAngle, 1);
      continue;
    }

    // Anything we did not classify above becomes an Unknown token with
    // a single-byte lexeme. The parser will turn it into a diagnostic
    // pointing at this column.
    tokens.push_back(make(TokenKind::Unknown,
                          std::string_view(source.data() + i, 1), line, sc));
    ++i;
    ++col;
  }

  // Sentinel EOF token — the parser relies on this never being absent.
  tokens.push_back(make(TokenKind::Eof, std::string_view{}, line, col));
  return tokens;
}
} // namespace caret::frontend
