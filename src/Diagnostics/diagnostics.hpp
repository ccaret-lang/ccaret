// this is the caretc a compiler for the C^
#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace caret {
enum class Severity : std::uint8_t { Note, Warning, Error, Fatal };
struct Diagnostic {
    Severity severity{Severity::Error};
    std::string code;
    std::string message;
    std::uint32_t line{0};
    std::uint32_t column{0};
};
class DiagnosticsEngine {
public:
    void emit(Diagnostic d) { items_.push_back(std::move(d)); }
    const std::vector<Diagnostic>& items() const { return items_; }
    bool has_errors() const;

private:
    std::vector<Diagnostic> items_;
};
}
