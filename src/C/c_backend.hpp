// this is the caretc a compiler for the C^
#pragma once
#include "AST/ast.hpp"
#include <string>
namespace caret::backend::c {
struct EmitResult {
    std::string code;
    bool ok{false};
};
EmitResult emit(const ast::TranslationUnit& unit);
}
