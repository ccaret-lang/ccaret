// this is the caretc a compiler for the C^
#include "diagnostics.hpp"
#include <algorithm>
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

const char *sev_color(Severity s, bool color) {
  if (!color)
    return "";
  switch (s) {
  case Severity::Note:
    return kBright;
  case Severity::Warning:
    return kBold; // bold yellow applied below
  case Severity::Error:
    return kBold; // bold red applied below
  case Severity::Fatal:
    return kBold;
  }
  return "";
}
const char *sev_color2(Severity s, bool color) {
  if (!color)
    return "";
  switch (s) {
  case Severity::Note:
    return kReset;
  case Severity::Warning:
    return kYellow;
  case Severity::Error:
    return kRed;
  case Severity::Fatal:
    return kRed;
  }
  return kReset;
}

std::string repeat(std::string s, std::size_t n) {
  std::string r;
  while (n--)
    r += s;
  return r;
}

std::string get_line(const std::string &src, std::uint32_t line) {
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

std::string pad_left(std::uint32_t n, std::size_t w) {
  std::ostringstream s;
  s << n;
  std::string str = s.str();
  if (str.size() < w)
    str = std::string(w - str.size(), ' ') + str;
  return str;
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
  const std::string green = color ? kGreen : "";
  const std::string blue = color ? kBlue : "";
  const std::string cyan = color ? kCyan : "";
  const char *col1 = sev_color(d.severity, color);
  const char *col2 = sev_color2(d.severity, color);

  // Head: "C^:error .. message ······ file:line:col"
  const std::string head = std::string("C^:") + sev_label(d.severity);
  const std::string loc =
      d.file + ":" + std::to_string(d.line) + ":" + std::to_string(d.column);

  // Build the message portion.
  std::string body = d.message;
  // Append evidence (the offending token) if non-empty.
  if (!d.evidence.empty()) {
    body += " `" + d.evidence + "`";
  }

  // Layout: target line length of 80; the location is right-aligned-ish.
  const std::size_t target = 80;
  std::string left = head + " .. " + body;
  if (left.size() + 2 + loc.size() > target) {
    // Too long — just print with a small gap.
    os << col1 << head << reset << " " << dim << ".." << reset << " " << body
       << "  " << dim << "··" << reset << " " << cyan << loc << reset << "\n";
  } else {
    std::size_t gap = target - left.size() - loc.size();
    std::string dots = gap > 2 ? repeat("·", gap) : "  ";
    os << col1 << head << reset << " " << dim << ".." << reset << " " << body
       << " " << dim << dots << reset << " " << cyan << loc << reset << "\n";
  }
  (void)col2;

  // Source excerpt with caret underline
  if (!source_text.empty() && d.line > 0) {
    std::string line_text = get_line(source_text, d.line);
    // Render the line with line number prefix.
    const std::string num = pad_left(d.line, 3);
    // Detect the column range to underline. Default: length of evidence.
    std::size_t col0 = d.column == 0 ? 0 : d.column - 1;
    std::size_t span = d.evidence.empty()
                           ? 1
                           : std::min<std::size_t>(d.evidence.size(),
                                                   line_text.size() > col0
                                                       ? line_text.size() - col0
                                                       : 1);
    if (span == 0)
      span = 1;
    std::string carets(span, '^');
    const char *caret_color = (d.severity == Severity::Warning)
                                  ? (color ? kYellow : "")
                                  : (color ? kRed : "");

    os << dim << num << reset << " " << dim << "|" << reset << " " << line_text
       << "\n";
    std::string gutter = "   " + dim + "|" + reset + " ";
    os << gutter;
    if (col0 > line_text.size())
      col0 = line_text.size();
    os << std::string(col0, ' ') << caret_color << carets << reset;
    if (!d.hint.empty()) {
      const char *hint_col = d.hint_kind == SuggestionKind::Tip
                                 ? (color ? kBlue : "")
                                 : (color ? kGreen : "");
      const std::string label =
          d.hint_kind == SuggestionKind::Tip ? "tip:" : "try:";
      os << " " << hint_col << label << reset << " " << d.hint;
    }
    os << "\n";
  }
}

int DiagnosticsEngine::render_all(std::ostream &os,
                                  const std::string &source_text) const {
  for (const auto &d : items_) {
    render(os, d, source_text);
  }
  return has_errors() ? 1 : 0;
}
} // namespace caret
