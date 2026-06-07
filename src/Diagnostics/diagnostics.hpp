// Diagnostics — the unified error / warning sink for the compiler.
//
// One engine instance per compile. Stages (lex, parse, semantic, …)
// push Diagnostics into it; the driver calls render_all() at the end
// of a failing run. Keeping this as a single sink means every error
// is rendered with the same format and we never double-print.
//
// Rendering rules live in syntax/errors_Design.md; the implementation
// here follows that spec verbatim. Any change to the visual layout
// should land in lock-step with the design doc.
#pragma once
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>
namespace caret {
// Severity buckets every diagnostic belongs to. The renderer picks a
// colour and a label from this enum; everything downstream (CLI exit
// code, JSON output) just counts Error + Fatal.
enum class Severity : std::uint8_t { Note, Warning, Error, Fatal };
// SuggestionKind is the discriminator between the two suggestion
// styles. `Try` is green and reads "try: do X"; `Tip` is blue and
// reads "tip: do X" — a softer best-practice nudge.
enum class SuggestionKind : std::uint8_t { Try, Tip };

// Diagnostic is the value type every front-end stage emits. It is
// flat-by-design: rendering doesn't need to walk sub-trees, only
// fields. The lexer / parser / semantic pass each build one of these
// per problem they find.
struct Diagnostic {
    Severity severity{Severity::Error};
    std::string code;       // optional, e.g. "E0001"; left empty today
    std::string message;    // one-line, human-readable
    std::string file;       // source path, "" for non-source diagnostics
    std::uint32_t line{0};  // 1-based; 0 means "no line info"
    std::uint32_t column{0};// 1-based; 0 means "no column info"
    std::string evidence;   // the offending token / lexeme (for the ^^^^^ underline)
    std::string hint;       // text shown after the carets, e.g. "try: change ..."
    SuggestionKind hint_kind{SuggestionKind::Try};
};

// DiagnosticsEngine is the single sink every pipeline stage pushes
// into. Render once at the end so errors stack in source order and
// share formatting. The engine is not thread-safe: caretc is a
// single-threaded driver for v0.1.0.
class DiagnosticsEngine {
public:
    // Add a diagnostic. Move-in keeps the call sites that already
    // build a Diagnostic locally zero-alloc on this hot path.
    void emit(Diagnostic d) { items_.push_back(std::move(d)); }

    // Convenience: build and emit an error with file/line/column from a
    // span-style token reference. Default hint_kind is Try because the
    // vast majority of in-parse diagnostics are actionable fixes.
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

    // Warning convenience, mirroring error() but defaulting the hint
    // kind to Tip — warnings tend to read better as soft suggestions.
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
    // Returns true the moment any Error or Fatal diagnostic has been
    // emitted. Warnings alone do not flip this. Drives the driver's
    // exit-code decision.
    bool has_errors() const;
    // Total Error + Fatal count. Used by tests to assert "exactly N
    // errors" without iterating the items vector.
    std::size_t error_count() const;

    // Render a single diagnostic in the C^ modal format. If
    // `source_text` is non-empty, the offending source line is printed
    // below the head line with a caret underline.
    //
    // Format spec: see syntax/errors_Design.md. Don't change the layout
    // here without updating that document and the snapshot tests.
    void render(std::ostream& os, const Diagnostic& d,
                const std::string& source_text) const;

    // Render every queued diagnostic, blank-line separated, and return
    // an exit code (0 if no errors, 1 otherwise). One call per compile
    // is the convention.
    int render_all(std::ostream& os, const std::string& source_text) const;

private:
    std::vector<Diagnostic> items_;
};

// Style is a future-proofing hook: once we add a flag to disable
// colour, this is the struct that carries the decision. Today it
// reads the TTY status directly inside render().
struct Style {
    bool color{true};
};
}
