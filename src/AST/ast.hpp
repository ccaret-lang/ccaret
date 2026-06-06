// this is the caretc a compiler for the C^
#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace caret::ast {
enum class NodeKind : std::uint16_t {
    TranslationUnit,
    FunctionDecl,
    ParamDecl,
    BlockStmt,
    ReturnStmt,
    ExprStmt,
    LiteralExpr,
    IdentifierExpr,
    BinaryExpr,
    CallExpr,
};
struct AstNode {
    NodeKind kind{NodeKind::TranslationUnit};
    std::string name;
    std::vector<AstNode> children;
};
}
