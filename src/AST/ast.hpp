// this is the caretc a compiler for the C^
#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace caret::ast {

// Type system
enum class TypeKind : std::uint8_t {
    Named,    // i32, void, string, user types
    Pointer,  // T* / T mut*
    Reference,// T& / T mut&
    Array,    // T[N]
    Slice,    // T[]
    Optional, // T?
    Auto,     // deduced
    ErrorUnion,// T!Err
};

struct Type;
using TypePtr = std::shared_ptr<Type>;

struct Type {
    TypeKind kind{TypeKind::Named};
    std::string name;             // for Named
    TypePtr    inner;              // for Pointer/Reference/Array/Slice/Optional/ErrorUnion
    bool       mut_target{false};  // T mut*
    bool       mut_binding{false}; // T* mut
    std::string array_size;        // for Array (kept as text)
    std::string error_set;         // for ErrorUnion (empty = inferred)

    static TypePtr named(std::string n) {
        auto t = std::make_shared<Type>();
        t->kind = TypeKind::Named;
        t->name = std::move(n);
        return t;
    }
    static TypePtr auto_() {
        auto t = std::make_shared<Type>();
        t->kind = TypeKind::Auto;
        return t;
    }
};

// Spans
struct Span {
    std::uint32_t line{1};
    std::uint32_t column{1};
};

// Forward declarations
struct Expr;
struct Stmt;
struct Decl;

using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;
using DeclPtr = std::shared_ptr<Decl>;

// === Expressions ===
enum class ExprKind : std::uint8_t {
    IntegerLiteral, FloatLiteral, StringLiteral, CharLiteral, BoolLiteral, NullLiteral,
    Identifier,
    Unary, Binary, Assign,
    Call,
    FieldAccess,    // a.b
    IndexAccess,    // a[i]
    Cast,           // (T) x
    Grouped,
    AddrOf,         // &var  (the @sizeof / @alignof builtins are skipped)
};

enum class UnaryOp : std::uint8_t {
    Neg, Not, BitNot, Plus, PreInc, PreDec, PostInc, PostDec, Deref,
};

enum class BinaryOp : std::uint8_t {
    Add, Sub, Mul, Div, Mod,
    Eq, NotEq, Lt, LtEq, Gt, GtEq,
    BitAnd, BitOr, BitXor, Shl, Shr,
    And, Or,
};

struct Expr {
    ExprKind kind{ExprKind::Identifier};
    Span span{};

    // Literal
    std::string literal_text;
    long long   int_value{0};
    double      float_value{0.0};
    std::string string_value;
    std::uint32_t char_value{0};
    bool        bool_value{false};

    // Identifier
    std::string name;

    // Unary / Binary
    UnaryOp  unary_op{UnaryOp::Neg};
    BinaryOp binary_op{BinaryOp::Add};
    ExprPtr  lhs;
    ExprPtr  rhs;

    // Assign target
    ExprPtr  assign_target;

    // Call
    ExprPtr                callee;
    std::vector<ExprPtr>   args;

    // Field / Index
    ExprPtr  object;
    std::string field_name;
    ExprPtr  index;

    // Cast
    TypePtr  cast_type;
};

inline ExprPtr make_expr(ExprKind k, Span s = {}) {
    auto e = std::make_shared<Expr>();
    e->kind = k;
    e->span = s;
    return e;
}

// === Statements ===
enum class StmtKind : std::uint8_t {
    VarDecl, ExprStmt, Return, Block, If, While, For, Loop, Break, Continue,
    Defer, Empty,
};

struct Stmt {
    StmtKind kind{StmtKind::Empty};
    Span span{};

    // VarDecl
    TypePtr  var_type;
    std::string var_name;
    bool     is_mut{false};
    bool     is_const{false};
    ExprPtr  var_init;

    // ExprStmt
    ExprPtr  expr;

    // Return
    ExprPtr  return_value;  // may be null for void

    // Block
    std::vector<StmtPtr> block_stmts;

    // If
    ExprPtr  if_cond;
    StmtPtr  if_then;
    StmtPtr  if_else;

    // While
    ExprPtr  while_cond;
    StmtPtr  while_body;

    // For
    StmtPtr  for_init;
    ExprPtr  for_cond;
    ExprPtr  for_step;
    StmtPtr  for_body;

    // Loop
    StmtPtr  loop_body;

    // Defer
    ExprPtr  defer_expr;
};

inline StmtPtr make_stmt(StmtKind k, Span s = {}) {
    auto st = std::make_shared<Stmt>();
    st->kind = k;
    st->span = s;
    return st;
}

// === Declarations (top-level) ===
enum class DeclKind : std::uint8_t {
    Function, Var, Const, Struct, Enum, Union, ErrorSet, Import, ExternFn, ExportFn,
};

struct Parameter {
    TypePtr       type;
    std::string   name;
    bool          is_mut{false};
    bool          is_const{false};
    Span          span{};
};

struct Decl {
    DeclKind kind{DeclKind::Function};
    Span span{};

    // Function
    TypePtr  fn_return_type;
    std::string fn_name;
    std::vector<Parameter> fn_params;
    StmtPtr  fn_body;             // may be null for declarations
    bool     fn_is_extern{false};
    bool     fn_is_export{false};

    // Var / Const (top-level)
    TypePtr  var_type;
    std::string var_name;
    bool     var_is_mut{false};
    bool     var_is_const{false};
    ExprPtr  var_init;

    // Import
    std::string import_path;
};

using TranslationUnit = std::vector<DeclPtr>;

}
