// Public compile-time identifiers for the caretc toolchain. These
// macros are what embedders and downstream tools include to detect
// the compiler / language. Source the version triple from a single
// place so the binary, docs, and --version output never drift.
#pragma once
#define CARETC_VERSION_MAJOR 0
#define CARETC_VERSION_MINOR 1
#define CARETC_VERSION_PATCH 0
#define CARETC_VERSION "0.1.0"
#define CARETC_LANGUAGE_NAME "C^"
// Default file extension recognised by caretc and produced by the
// formatter. Kept here as well as in Frontend/Syntax/syntax.hpp so
// external embedders that don't pull in the frontend headers can
// still discover it.
#define CARETC_LANGUAGE_FILE_EXT ".cca"
