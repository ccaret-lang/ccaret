// C backend — lowers a parsed AST to a C17 source file.
//
// This is the production backend for v0.1.0. We translate the
// C^ AST directly to C and hand the result off to the system cc
// (see main.cpp). The output is plain C17: no GCC extensions, no
// inline assembly, so it builds with any conformant compiler.
#pragma once
#include "AST/ast.hpp"
#include <string>
namespace caret::backend::c {
// EmitResult: holds the generated source plus a status flag. `ok`
// becomes false on any error surfaced during emission; today the
// backend does not emit diagnostics of its own but the plumbing is
// in place for the upcoming type-aware lowerings.
struct EmitResult {
    std::string code;
    bool ok{false};
};

// Drive the lowering. The unit is consumed in source order: forward
// decls, top-level var/const decls, function bodies, then imports
// (silently dropped — module resolution is post-v0.1.0).
EmitResult emit(const ast::TranslationUnit& unit);
}
