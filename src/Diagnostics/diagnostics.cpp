// Diagnostic renderer.
//
// Implements the C^ error modal format defined in
// syntax/errors_Design.md. The contract is small and explicit:
//
//   1. Each diagnostic is rendered independently. Two consecutive
//      diagnostics are separated by a single blank line in render_all.
//   2. The header is `C^:error .. <message> <dot-leader> <file:line:col>`.
//      Width is bounded by `target` (80 cols) and we collapse the
//      leader to a fixed "··" pair when the head overflows.
//   3. The source excerpt shows line number, a `|`, the line text, then
//      a gutter line with `^^^^` carets. A trailing `try:` / `tip:`
//      hint sits on the caret line, never on a new line.
//
// The implementation leans on a small set of TTY-aware helpers below;
// all the colour logic is local to this file so a future --no-color
// flag only needs to wire stdout_is_tty() through.
#include "diagnostics.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <unistd.h>

namespace caret {

namespace {
// ANSI escape fragments. Kept as raw `const char*` so we never go
// through std::string constructors on the render hot path. Sequences
// follow the ECMA-48 SGR table.
constexpr const char *kReset  = "\x1b[0m";
constexpr const char *kBold   = "\x1b[1m";
constexpr const char *kDim    = "\x1b[2m";
constexpr const char *kRed    = "\x1b[31m";
constexpr const char *kYellow = "\x1b[33m";
constexpr const char *kGreen  = "\x1b[32m";
constexpr const char *kBlue   = "\x1b[34m";
constexpr const char *kCyan   = "\x1b[36m";
constexpr const char *kBright = "\x1b[97m";
// Middle dot (U+00B7) — the C^ range operator used as a visual filler.
// We split the two UTF-8 bytes so the literal stays a printable char.
constexpr const char kDot = '\xC2';
constexpr const char kDotTail = '\xB7';

// Decide whether to emit colour codes. We probe the stderr stream
// because that's where diagnostics go. Piped output is left
// uncoloured so `caretc foo.cca > out.log` produces a clean file.
bool stdout_is_tty() { return ::isatty(STDERR_FILENO) != 0; }

// Map a Severity to the lower-case label used in `C^:<label>`.
const char *sev_label(Severity s) {
  switch (s) {
  case Severity::Note:    return "note";
  case Severity::Warning: return "warning";
  case Severity::Error:   return "error";
  case Severity::Fatal:   return "fatal";
  }
  return "error";
}

// ANSI prefix for the severity tag (e.g. "C^:error"). Error / Warning /
// Fatal are bold; Note is bright white. Returns the empty string when
// colour is disabled so the prefix concatenation is a no-op.
std::string sev_head_style(Severity s, bool color) {
  if (!color)
    return "";
  switch (s) {
  case Severity::Note:    return std::string(kBright);
  case Severity::Warning: return std::string(kBold) + kYellow;
  case Severity::Error:   return std::string(kBold) + kRed;
  case Severity::Fatal:   return std::string(kBold) + kRed;
  }
  return "";
}

// ANSI prefix for the `^` carets drawn under the source. Same palette
// as the head line, but without the bold attribute so the underline
// reads as a softer highlight.
const char *sev_caret_style(Severity s, bool color) {
  if (!color)
    return "";
  switch (s) {
  case Severity::Note:    return kBright;
  case Severity::Warning: return kYellow;
  case Severity::Error:   return kRed;
  case Severity::Fatal:   return kRed;
  }
  return kReset;
}

// Repeat `s` `n` times. Used for the dot leader in the header.
std::string repeat(const std::string &s, std::size_t n) {
  std::string r;
  r.reserve(s.size() * n);
  while (n--)
    r += s;
  return r;
}

// Extract the 1-based `line`-th line of `src`, without the trailing
// newline. The result is a fresh string the renderer can mutate (we
// don't today, but `get_line` stays a copy for safety).
std::string get_line(const std::string &src, std::uint32_t line) {
  if (line == 0)
    return {};
  std::uint32_t cur = 1;
  std::size_t start = 0;
  for (std::size_t i = 0; i < src.size(); ++i) {
    if (src[i] == '\n') {
      if (cur == line)
        return src.substr(start, i - start);
      ++cur;
      start = i + 1;
    }
  }
  if (cur == line && start < src.size())
    return src.substr(start);
  return {};
}

// Left-pad `n` to `w` columns with spaces. The gutter is fixed at 3
// characters today, so this only matters for line numbers > 999
// (where the gutter just shifts naturally — no crash).
std::string pad_left_uint(std::uint32_t n, std::size_t w) {
  std::ostringstream s;
  s << n;
  std::string str = s.str();
  if (str.size() < w)
    str = std::string(w - str.size(), ' ') + str;
  return str;
}

// Visual width of a string for layout purposes. ASCII bytes count 1;
// UTF-8 lead bytes (2/3/4-byte sequences) count 1 and skip the right
// number of continuation bytes. The diagnostic header contains only
// the 2-byte "·" sequence, which this treats as width 1 to match the
// design's table layout. Anything past that we treat as width 1 too;
// wide-glyph source code is a future problem.
std::size_t visual_size(const std::string &s) {
  std::size_t n = 0;
  for (std::size_t i = 0; i < s.size(); ++i) {
    unsigned char c = static_cast<unsigned char>(s[i]);
    if (c < 0x80) {
      ++n;
    } else if ((c & 0xE0) == 0xC0) {
      ++n;
      ++i; // skip continuation byte
    } else if ((c & 0xF0) == 0xE0) {
      ++n;
      i += 2; // skip 2 continuation bytes
    } else {
      ++n;
    }
  }
  return n;
}

} // namespace

bool DiagnosticsEngine::has_errors() const {
  for (const auto &d : items_) {
    if (d.severity == Severity::Error || d.severity == Severity::Fatal) {
      return true;
    }
  }
  return false;
}

std::size_t DiagnosticsEngine::error_count() const {
  std::size_t n = 0;
  for (const auto &d : items_) {
    if (d.severity == Severity::Error || d.severity == Severity::Fatal)
      ++n;
  }
  return n;
}

void DiagnosticsEngine::render(std::ostream &os, const Diagnostic &d,
                               const std::string &source_text) const {
  const bool color = stdout_is_tty();
  const std::string reset = color ? kReset : "";
  const std::string dim = color ? kDim : "";
  const std::string cyan = color ? kCyan : "";
  const std::string head_style = sev_head_style(d.severity, color);
  const char *caret_style = sev_caret_style(d.severity, color);

  // Header tag: "C^:error" / "C^:warning" / etc.
  const std::string tag = std::string("C^:") + sev_label(d.severity);

  // Location: "file:line:col". We omit the location entirely if line or
  // column is unset, to keep the output clean for non-source diagnostics
  // (e.g. argv / IO errors).
  std::string loc;
  if (d.line > 0 && d.column > 0) {
    loc =
        d.file + ":" + std::to_string(d.line) + ":" + std::to_string(d.column);
  }

  // The body is the message exactly as the caller supplied it. Per the
  // design we do NOT append the evidence; the message already names the
  // relevant token in backticks, and the carets below show it visually.
  const std::string &body = d.message;

  // ---- Head line ---------------------------------------------------------
  // Layout: "<tag> .. <body> <dot-leader> <loc>"
  //
  // The dot leader adjusts to keep the location right-aligned within the
  // target width. If the head is too long we collapse the leader to a
  // pair of middle dots and let the location flow naturally.
  const std::size_t target = 80;
  const std::string left = tag + " .. " + body;

  if (loc.empty()) {
    // No location: just print the tag and body.
    os << head_style << tag << reset << " " << dim << ".." << reset << " "
       << body << "\n";
  } else {
    const std::size_t left_w = visual_size(left);
    const std::size_t loc_w = visual_size(loc);
    if (left_w + 1 + 1 + loc_w + 1 > target) {
      // No room for a real leader — collapse to "··".
      os << head_style << tag << reset << " " << dim << ".." << reset << " "
         << body << "  " << dim << "\xC2\xB7\xC2\xB7" << reset << " " << cyan
         << loc << reset << "\n";
    } else {
      const std::size_t leader =
          target - left_w - 1 - loc_w - 1; // 1 space + loc + 1 trailing
      const std::string dots =
          leader >= 2 ? repeat(std::string("\xC2\xB7"), leader) : "  ";
      os << head_style << tag << reset << " " << dim << ".." << reset << " "
         << body << " " << dim << dots << reset << " " << cyan << loc << reset
         << "\n";
    }
  }

  // ---- Source excerpt with caret underline ------------------------------
  // Per design rule 6: line numbers appear only when source is displayed.
  if (source_text.empty() || d.line == 0)
    return;

  const std::string line_text = get_line(source_text, d.line);
  if (line_text.empty())
    return;

  // We pad the line number to 3 columns so carets line up for files with
  // up to 999 lines; for larger files the gutter just shifts naturally.
  const std::string num = pad_left_uint(d.line, 3);

  // Column 0 in the diagnostic means "no column"; treat it as 1 so we
  // still draw a caret at the start of the line.
  const std::size_t col0 = d.column == 0 ? 0 : d.column - 1;

  // Caret length: width of the evidence token, clamped so we never run
  // off the end of the line. Fall back to a single caret if we have
  // nothing better to go on.
  std::size_t span = 1;
  if (!d.evidence.empty()) {
    span = std::min<std::size_t>(
        d.evidence.size(),
        col0 <= line_text.size() ? line_text.size() - col0 : 1);
  }
  if (span == 0)
    span = 1;
  const std::string carets(span, '^');

  os << dim << num << reset << " " << dim << "|" << reset << " " << line_text
     << "\n";

  // Gutter for the caret line: "   | " (matches the width of "<num> | ").
  const std::string gutter = "   " + dim + "|" + reset + " ";
  os << gutter;

  // Clamp the column to the line length so we never print past the end
  // of the source line.
  const std::size_t safe_col =
      col0 > line_text.size() ? line_text.size() : col0;
  os << std::string(safe_col, ' ') << caret_style << carets << reset;

  if (!d.hint.empty()) {
    const bool is_tip = d.hint_kind == SuggestionKind::Tip;
    const char *hint_col = color ? (is_tip ? kBlue : kGreen) : "";
    const std::string label = is_tip ? "tip:" : "try:";
    os << " " << hint_col << label << reset << " " << d.hint;
  }
  os << "\n";
}

int DiagnosticsEngine::render_all(std::ostream &os,
                                  const std::string &source_text) const {
  for (std::size_t i = 0; i < items_.size(); ++i) {
    if (i > 0)
      os << "\n";
    render(os, items_[i], source_text);
  }
  return has_errors() ? 1 : 0;
}
} // namespace caret
