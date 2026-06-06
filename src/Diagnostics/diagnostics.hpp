// this is the caretc a compiler for the C^
#pragma once
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>
namespace caret {
enum class Severity : std::uint8_t { Note, Warning, Error, Fatal };
enum class SuggestionKind : std::uint8_t { Try, Tip };

struct Suggestion {
    SuggestionKind kind{SuggestionKind::Try};
    std::string text;
};

struct Diagnostic {
    Severity severity{Severity::Error};
    std::string code;
    std::string message;
    std::string file;
    std::uint32_t line{0};
    std::uint32_t column{0};
    std::string evidence;   // the offending token / lexeme
    std::string hint;       // text shown after the carets, e.g. "try: change ..."
    SuggestionKind hint_kind{SuggestionKind::Try};
};

class DiagnosticsEngine {
public:
    void emit(Diagnostic d) { items_.push_back(std::move(d)); }

    // Convenience: build and emit an error with file/line/column from a
    // span-style token reference.
    void error(const std::string& file, std::uint32_t line, std::uint32_t col,
               const std::string& msg, const std::string& evidence = "",
               const std::string& hint = "",
               SuggestionKind hint_kind = SuggestionKind::Try) {
        Diagnostic d;
        d.severity = Severity::Error;
        d.message = msg;
        d.file = file;
        d.line = line;
        d.column = col;
        d.evidence = evidence;
        d.hint = hint;
        d.hint_kind = hint_kind;
        emit(std::move(d));
    }

    void warning(const std::string& file, std::uint32_t line, std::uint32_t col,
                 const std::string& msg, const std::string& evidence = "",
                 const std::string& hint = "",
                 SuggestionKind hint_kind = SuggestionKind::Tip) {
        Diagnostic d;
        d.severity = Severity::Warning;
        d.message = msg;
        d.file = file;
        d.line = line;
        d.column = col;
        d.evidence = evidence;
        d.hint = hint;
        d.hint_kind = hint_kind;
        emit(std::move(d));
    }

    const std::vector<Diagnostic>& items() const { return items_; }
    std::vector<Diagnostic>& items() { return items_; }
    bool has_errors() const;
    std::size_t error_count() const;

    // Print a single diagnostic in the C^ error modal format.
    // If source is non-empty, prints the source line + caret underline.
    void render(std::ostream& os, const Diagnostic& d,
                const std::string& source_text) const;

    // Render all diagnostics to a stream, then return exit code (0 if none).
    int render_all(std::ostream& os, const std::string& source_text) const;

private:
    std::vector<Diagnostic> items_;
};

// Color helpers (ANSI). Returns empty when colors are disabled.
struct Style {
    bool color{true};
};
}
