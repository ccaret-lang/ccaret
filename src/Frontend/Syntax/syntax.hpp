// Language-level constants used in user-facing output: the compiler's
// name banner, the default file extension reported in --help / docs,
// and any other place we want a single source of truth for "what is
// this language called" without hard-coding the spelling at the call
// site.
#pragma once
namespace caret::frontend::syntax {
constexpr const char* kLanguageName = "C^";
// File extension for source files. .cca matches the sample programs
// under examples/ and the lexer default-accepts it from the driver.
constexpr const char* kFileExtension = ".cca";
}
