// this is the caretc a compiler for the C^
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
constexpr const char *kReset = "\x1b[0m";
constexpr const char *kBold = "\x1b[1m";
constexpr const char *kDim = "\x1b[2m";
constexpr const char *kRed = "\x1b[31m";
constexpr const char *kYellow = "\x1b[33m";
constexpr const char *kGreen = "\x1b[32m";
constexpr const char *kBlue = "\x1b[34m";
constexpr const char *kCyan = "\x1b[36m";
constexpr const char *kBright = "\x1b[97m";
// Middle dot (U+00B7) — the C^ range operator used as a visual filler.
constexpr const char kDot = '\xC2';
constexpr const char kDotTail = '\xB7';

bool stdout_is_tty() { return ::isatty(STDERR_FILENO) != 0; }

const char *sev_label(Severity s) {
  switch (s) {
  case Severity::Note:
    return "note";
  case Severity::Warning:
    return "warning";
  case Severity::Error:
    return "error";
  case Severity::Fatal:
    return "fatal";
  }
  return "error";
}

// Returns the ANSI prefix for the severity tag (e.g. "C^:error"). For
// Error / Warning / Fatal we want BOLD + the matching foreground colour.
std::string sev_head_style(Severity s, bool color) {
  if (!color)
    return "";
  switch (s) {
  case Severity::Note:
    return std::string(kBright);
  case Severity::Warning:
    return std::string(kBold) + kYellow;
  case Severity::Error:
    return std::string(kBold) + kRed;
  case Severity::Fatal:
    return std::string(kBold) + kRed;
  }
  return "";
}

// Returns the ANSI prefix used to colour the `^` carets under the source.
const char *sev_caret_style(Severity s, bool color) {
  if (!color)
    return "";
  switch (s) {
  case Severity::Note:
    return kBright;
  case Severity::Warning:
    return kYellow;
  case Severity::Error:
    return kRed;
  case Severity::Fatal:
    return kRed;
  }
  return kReset;
}

std::string repeat(const std::string &s, std::size_t n) {
  std::string r;
  r.reserve(s.size() * n);
  while (n--)
    r += s;
  return r;
}

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

std::string pad_left_uint(std::uint32_t n, std::size_t w) {
  std::ostringstream s;
  s << n;
  std::string str = s.str();
  if (str.size() < w)
    str = std::string(w - str.size(), ' ') + str;
  return str;
}

// Compute the visual width used to lay out a line. The diagnostic header
// is plain ASCII plus a couple of UTF-8 middle dots. We treat the 2-byte
// "·" sequence as width 1 (matching the design's table layout).
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
    loc = d.file + ":" + std::to_string(d.line) + ":" +
          std::to_string(d.column);
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
      const std::string dots = leader >= 2
                                   ? repeat(std::string("\xC2\xB7"), leader)
                                   : "  ";
      os << head_style << tag << reset << " " << dim << ".." << reset << " "
         << body << " " << dim << dots << reset << " " << cyan << loc
         << reset << "\n";
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
