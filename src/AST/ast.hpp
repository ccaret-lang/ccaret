// AST — Abstract Syntax Tree for C^ source.
//
// One file deliberately holds every node kind: keeping Type, Expr, Stmt,
// Decl in lock-step makes parser and backend changes a single grep away.
// Nodes are allocated through std::shared_ptr — not because we share
// ownership (we don't, the parser is the sole producer) but because it
// gives us cheap recursive types and a deterministic free order at
// translation-unit teardown. If allocator pressure ever shows up in a
// profile, this is the first thing to replace with an arena.
//
// Spans store line + column only; the original source buffer lives in
// main.cpp and the diagnostics renderer slices it on demand.
#pragma once
#include <cstdint>
#include <memory>
// #include <optional>
#include <string>
// #include <variant>
#include <vector>

namespace caret::ast {

// === Type system ===
//
// TypeKind discriminates the small closed set of type shapes the v0.1.0
// front-end understands. Anything user-defined (struct, enum, error)
// resolves to TypeKind::Named with the identifier verbatim — name
// resolution happens later in the (still-stub) semantic pass.
enum class TypeKind : std::uint8_t {
    Named,     // built-in (i32, f64, bool, void, string, ...) or a user type
    Pointer,   // T*  / T mut*   — raw, possibly null, postfix-deref via `.*`
    Reference, // T&  / T mut&   — non-null alias, auto-deref on every use
    Array,     // T[N]           — fixed length, length stored as text for now
    Slice,     // T[]            — fat pointer (ptr + length)
    Optional,  // T?             — nullable but must be unwrapped before use
    Auto,      // deduced from initializer (var decls only)
    ErrorUnion,// T!Err          — value-or-error union (see syntax/errors.cca)
};

struct Type;
using TypePtr = std::shared_ptr<Type>;

// Type is a uniform record for every TypeKind. Fields not relevant to a
// given kind are left at their default — using std::variant here would
// save a few bytes per node but cost us the pleasantly flat ergonomics
// the rest of the compiler relies on.
struct Type {
    TypeKind kind{TypeKind::Named};
    std::string name;              // Named: the spelling as it appeared in source
    TypePtr    inner;              // Pointer / Reference / Array / Slice / Optional / ErrorUnion
    bool       mut_target{false};  // `mut` BEFORE * / &  — pointee/referent is mutable
    bool       mut_binding{false}; // `mut` AFTER  * / &  — the pointer/ref itself is reassignable
    std::string array_size;        // Array: kept as source text; const-eval happens later
    std::string error_set;         // ErrorUnion: empty means "inferred error set"

    // Convenience factory for the common case: a plain named type.
    // Most call sites in the parser want this shape; keeping the helper
    // here avoids leaking std::make_shared boilerplate everywhere.
    static TypePtr named(std::string n) {
        auto t = std::make_shared<Type>();
        t->kind = TypeKind::Named;
        t->name = std::move(n);
        return t;
    }
    // Sentinel for `auto` — the parser emits this for var decls whose
    // type was elided. Resolution happens in the semantic pass.
    static TypePtr auto_() {
        auto t = std::make_shared<Type>();
        t->kind = TypeKind::Auto;
        return t;
    }
};

// Source span. Both fields are 1-based to match what users see in
// diagnostics. Column 0 / line 0 are treated as "unset" downstream.
struct Span {
    std::uint32_t line{1};
    std::uint32_t column{1};
};

// Forward declarations so the Expr / Stmt / Decl records can refer to
// each other through their shared_ptr aliases below.
struct Expr;
struct Stmt;
struct Decl;

using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;
using DeclPtr = std::shared_ptr<Decl>;

// === Expressions ===
//
// ExprKind covers every leaf and inner node a parser can produce. We
// keep the enum dense and small (uint8_t) so a vector<ExprPtr> stays
// cache-friendly when the backend sweeps it.
enum class ExprKind : std::uint8_t {
    IntegerLiteral, FloatLiteral, StringLiteral, CharLiteral, BoolLiteral, NullLiteral,
    Identifier,
    Unary, Binary, Assign,
    Call,
    FieldAccess,   // a.b
    IndexAccess,   // a[i]
    Cast,          // (T) x
    Grouped,       // ( x )
    AddrOf,        // &var
    PostfixDeref,  // ptr.*  — C^'s postfix raw-pointer dereference
};

// UnaryOp covers both prefix and postfix forms. Postfix variants are
// disambiguated by ExprKind::Unary nodes with PostInc / PostDec rather
// than by a separate kind, keeping the AST shape uniform.
enum class UnaryOp : std::uint8_t {
    Neg, Not, BitNot, Plus, PreInc, PreDec, PostInc, PostDec, Deref,
};

// BinaryOp lists every infix operator we recognise today. Precedence
// lives in the parser (parse_*); this enum is only the label.
enum class BinaryOp : std::uint8_t {
    Add, Sub, Mul, Div, Mod,
    Eq, NotEq, Lt, LtEq, Gt, GtEq,
    BitAnd, BitOr, BitXor, Shl, Shr,
    And, Or,
};

// Expr is the union-by-fields record for every expression node. Only
// fields meaningful to `kind` are populated; the rest stay default.
// Treat it as a tagged record, not a variant — code reading any node
// must switch on `kind` first.
struct Expr {
    ExprKind kind{ExprKind::Identifier};
    Span span{};

    // Literal payloads. literal_text always holds the raw source spelling
    // for round-trippability; the parsed value goes in the typed field.
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

    // Assignment target. For compound assign (`+=` etc.) the operator
    // text is stashed in `literal_text` so the backend can re-derive it
    // without keeping a second enum in sync.
    ExprPtr  assign_target;

    // Call
    ExprPtr                callee;
    std::vector<ExprPtr>   args;

    // Field access (a.b) and index access (a[i]) share an `object` slot.
    ExprPtr  object;
    std::string field_name;
    ExprPtr  index;

    // Cast: `(T) x` — cast_type holds T, lhs holds x.
    TypePtr  cast_type;
};

// Allocate a fresh Expr with kind/span pre-filled. Use this in the
// parser instead of std::make_shared so we have one place to add
// instrumentation (e.g. id-numbering) later.
inline ExprPtr make_expr(ExprKind k, Span s = {}) {
    auto e = std::make_shared<Expr>();
    e->kind = k;
    e->span = s;
    return e;
}

// === Statements ===
//
// StmtKind covers everything a function body can contain. Top-level
// declarations live on Decl — keeping the two trees separate stops the
// parser from accidentally accepting a `fn` inside an `if`.
enum class StmtKind : std::uint8_t {
    VarDecl, ExprStmt, Return, Block, If, While, For, Loop, Break, Continue,
    Defer, Empty,
};

// Stmt mirrors Expr: a tagged record where only the fields relevant to
// `kind` are populated. Adding a new statement kind almost always means
// adding fields here and a `case` in the parser + backend.
struct Stmt {
    StmtKind kind{StmtKind::Empty};
    Span span{};

    // VarDecl (`TYPE [mut|const] NAME [= INIT];`)
    TypePtr  var_type;
    std::string var_name;
    bool     is_mut{false};
    bool     is_const{false};
    ExprPtr  var_init;

    // ExprStmt (`<expr>;`)
    ExprPtr  expr;

    // Return — return_value is null for `return;` in void-returning fns.
    ExprPtr  return_value;

    // Block — children statements in source order.
    std::vector<StmtPtr> block_stmts;

    // If — `else if` chains are represented by nesting another If into
    // `if_else`; the parser builds the chain right-recursively.
    ExprPtr  if_cond;
    StmtPtr  if_then;
    StmtPtr  if_else;

    // While
    ExprPtr  while_cond;
    StmtPtr  while_body;

    // For (`for INIT; COND; STEP { ... }`). All three header slots are
    // optional; missing pieces stay null.
    StmtPtr  for_init;
    ExprPtr  for_cond;
    ExprPtr  for_step;
    StmtPtr  for_body;

    // Loop — unconditional infinite loop; broken by `break`.
    StmtPtr  loop_body;

    // Defer — runs `defer_expr` at scope exit (LIFO across multiple defers
    // in the same scope). The C backend currently lowers this as a comment;
    // proper lowering is tracked in the IR roadmap.
    ExprPtr  defer_expr;
};

// Allocate a fresh Stmt with kind/span pre-filled. Mirror of make_expr.
inline StmtPtr make_stmt(StmtKind k, Span s = {}) {
    auto st = std::make_shared<Stmt>();
    st->kind = k;
    st->span = s;
    return st;
}

// === Declarations (top-level) ===
//
// DeclKind enumerates everything that can appear at file scope.
// ExternFn and ExportFn carry the same shape as Function but flag the
// linkage so the backend can emit `extern` / `__attribute__((visibility))`
// or equivalent.
enum class DeclKind : std::uint8_t {
    Function, Var, Const, Struct, Enum, Union, ErrorSet, Import, ExternFn, ExportFn,
};

// One formal parameter of a function. Mutability flags follow the same
// rules as VarDecl: `mut` makes the binding writable inside the body;
// type-level `mut` on `*` / `&` lives in the Type record instead.
struct Parameter {
    TypePtr       type;
    std::string   name;
    bool          is_mut{false};
    bool          is_const{false};
    Span          span{};
};

// Decl is the top-level tagged record. As with Expr/Stmt only the
// fields relevant to `kind` are populated. The backend switches on kind
// once per declaration; readers should do the same.
struct Decl {
    DeclKind kind{DeclKind::Function};
    Span span{};

    // Function / ExternFn / ExportFn. fn_body is null for extern decls
    // and for forward declarations.
    TypePtr  fn_return_type;
    std::string fn_name;
    std::vector<Parameter> fn_params;
    StmtPtr  fn_body;
    bool     fn_is_extern{false};
    bool     fn_is_export{false};

    // Var / Const at translation-unit scope.
    TypePtr  var_type;
    std::string var_name;
    bool     var_is_mut{false};
    bool     var_is_const{false};
    ExprPtr  var_init;

    // Import — the dotted module path as written (e.g. "std.io"). The
    // module resolver hasn't landed yet, so this is informational only.
    std::string import_path;
};

// A whole compilation unit is just the ordered list of its top-level
// declarations. Source order is preserved so the backend can emit
// forward decls and bodies in two passes without reshuffling.
using TranslationUnit = std::vector<DeclPtr>;

}
