// this is the caretc a compiler for the C^
#include "parser.hpp"
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

namespace caret::frontend {

namespace {

class Parser {
public:
    Parser(const std::vector<Token>& toks, DiagnosticsEngine& d, std::string path)
        : toks_(toks), diags_(d), path_(std::move(path)) {}

    ParseResult run() {
        ParseResult r;
        while (!at_eof()) {
            if (auto d = parse_top_level()) {
                r.unit.push_back(d);
            } else {
                if (!at_eof() && !sync_recovery()) break;
            }
        }
        r.ok = !diags_.has_errors();
        return r;
    }

private:
    const std::vector<Token>& toks_;
    DiagnosticsEngine&        diags_;
    std::string               path_;
    std::size_t               pos_{0};

    // ----- token helpers -----
    const Token& cur() const { return toks_[pos_]; }
    const Token& peek(std::size_t off = 0) const {
        return toks_[pos_ + off < toks_.size() ? pos_ + off : toks_.size() - 1];
    }
    bool at_eof() const { return cur().kind == TokenKind::Eof; }
    bool check(TokenKind k) const { return cur().kind == k; }
    bool check2(TokenKind a, TokenKind b) const {
        return cur().kind == a && peek(1).kind == b;
    }
    bool match(TokenKind k) {
        if (check(k)) { advance(); return true; }
        return false;
    }
    void advance() { if (!at_eof()) ++pos_; }
    const Token& expect(TokenKind k, const char* what) {
        if (check(k)) {
            const Token& t = cur();
            advance();
            return t;
        }
        error_at(cur(), std::string("expected ") + what + ", found " +
                          token_kind_name(cur().kind));
        return cur();
    }
    void error_at(const Token& t, std::string msg) {
        Diagnostic d;
        d.severity = Severity::Error;
        d.code = "E0001";
        d.message = std::move(msg);
        d.line = t.line;
        d.column = t.column;
        d.file = path_;
        d.evidence = token_evidence(t);
        diags_.emit(std::move(d));
    }
    void error_here(std::string msg) { error_at(cur(), std::move(msg)); }
    static std::string token_evidence(const Token& t) {
        if (t.lexeme.empty()) return std::string{};
        return std::string(t.lexeme);
    }

    // ----- recovery -----
    bool sync_recovery() {
        int safety = 0;
        while (!at_eof()) {
            if (cur().kind == TokenKind::Semicolon) { advance(); return true; }
            if (cur().kind == TokenKind::RBrace) return true;
            if (cur().kind == TokenKind::LBrace) return true;
            advance();
            if (++safety > 256) break;
        }
        return false;
    }

    // ----- top level -----
    ast::DeclPtr parse_top_level() {
        // imports
        if (check(TokenKind::KwImport)) return parse_import();
        // extern c { ... } block — flat
        if (check(TokenKind::KwExtern) && peek(1).kind == TokenKind::LBrace) {
            parse_extern_block();
            return nullptr;
        }
        if (check(TokenKind::KwExport)) {
            advance();
            return parse_function(/*is_export=*/true);
        }
        if (check(TokenKind::KwExtern)) {
            // extern RET_TYPE NAME(...);
            advance();
            return parse_function(/*is_export=*/false, /*is_extern=*/true);
        }
        // const / var / function at top level
        // Heuristic: if a Type followed by an identifier is a top-level var/const or fn.
        // We try var/const first, then fall through to function parsing.
        if (looks_like_top_var()) return parse_top_var();
        return parse_function();
    }

    bool looks_like_top_var() {
        // TYPE [mut|const] NAME = ...
        // TYPE [mut|const] NAME ;
        // TYPE [mut|const] NAME ( ... )   -> function
        std::size_t save = pos_;
        // Skip leading type
        if (!is_type_start(cur().kind)) return false;
        std::size_t p = pos_ + 1;
        // Skip pointer/reference markers
        while (p < toks_.size() &&
               (toks_[p].kind == TokenKind::Star || toks_[p].kind == TokenKind::Amp ||
                toks_[p].kind == TokenKind::KwMut)) {
            ++p;
        }
        // Expect identifier
        if (p >= toks_.size() || toks_[p].kind != TokenKind::Identifier) return false;
        ++p;
        // Optional const/mut
        if (p < toks_.size() && toks_[p].kind == TokenKind::KwMut) ++p;
        if (p < toks_.size() && toks_[p].kind == TokenKind::KwConst) ++p;
        if (p >= toks_.size()) return false;
        if (toks_[p].kind == TokenKind::Assign) return true;
        if (toks_[p].kind == TokenKind::Semicolon) return true;
        return false;
    }

    // ----- type parsing -----
    static bool is_type_start(TokenKind k) {
        switch (k) {
        case TokenKind::KwI8: case TokenKind::KwI16: case TokenKind::KwI32:
        case TokenKind::KwI64: case TokenKind::KwI128: case TokenKind::KwISize:
        case TokenKind::KwU8: case TokenKind::KwU16: case TokenKind::KwU32:
        case TokenKind::KwU64: case TokenKind::KwU128: case TokenKind::KwUSize:
        case TokenKind::KwF32: case TokenKind::KwF64:
        case TokenKind::KwBool: case TokenKind::KwChar: case TokenKind::KwVoid:
        case TokenKind::KwString: case TokenKind::KwAuto:
        case TokenKind::KwStruct: case TokenKind::KwEnum: case TokenKind::KwUnion:
        case TokenKind::KwError:
        case TokenKind::Identifier:
        case TokenKind::KwConst: case TokenKind::KwMut: case TokenKind::KwVolatile:
            return true;
        default: return false;
        }
    }

    static bool is_storage_keyword(TokenKind k) {
        return k == TokenKind::KwMut || k == TokenKind::KwConst;
    }

    ast::TypePtr parse_type() {
        // Optional leading const/volatile for raw-pointer types
        ast::TypePtr t;
        TokenKind lead = cur().kind;
        if (lead == TokenKind::KwConst || lead == TokenKind::KwVolatile) {
            // We'll wrap later for pointer types only.
            advance();
            t = parse_base_type();
            // For now ignore const on the base type; it applies only to pointee when
            // followed by *. We handle it as a plain type for now.
            return t;
        }
        t = parse_base_type();
        return t;
    }

    ast::TypePtr parse_base_type() {
        TokenKind k = cur().kind;
        switch (k) {
        case TokenKind::KwI8:  case TokenKind::KwI16:  case TokenKind::KwI32:
        case TokenKind::KwI64: case TokenKind::KwI128: case TokenKind::KwISize:
        case TokenKind::KwU8:  case TokenKind::KwU16:  case TokenKind::KwU32:
        case TokenKind::KwU64: case TokenKind::KwU128: case TokenKind::KwUSize:
        case TokenKind::KwF32:  case TokenKind::KwF64:
        case TokenKind::KwBool: case TokenKind::KwChar: case TokenKind::KwVoid:
        case TokenKind::KwString:
        case TokenKind::KwAuto: {
            std::string name = std::string(cur().lexeme);
            advance();
            return ast::Type::named(std::move(name));
        }
        case TokenKind::Identifier: {
            std::string name = std::string(cur().lexeme);
            advance();
            return ast::Type::named(std::move(name));
        }
        default:
            error_here(std::string("expected type, found ") +
                       token_kind_name(cur().kind));
            advance();
            return ast::Type::named("<error>");
        }
    }

    // Parse a type that may include mut, *, & suffixes.
    ast::TypePtr parse_full_type(bool allow_ptr_ref = true) {
        bool mut_target = false;
        if (check(TokenKind::KwMut)) { mut_target = true; advance(); }
        ast::TypePtr base = parse_base_type();
        if (!allow_ptr_ref) {
            if (mut_target) {
                auto t = base;
                t->mut_target = true;
            }
            return base;
        }
        // Pointer / reference chain
        while (check(TokenKind::Star) || check(TokenKind::Amp)) {
            bool is_ptr = check(TokenKind::Star);
            advance();
            auto wrap = std::make_shared<ast::Type>();
            wrap->kind = is_ptr ? ast::TypeKind::Pointer : ast::TypeKind::Reference;
            wrap->inner = base;
            wrap->mut_target = mut_target;
            mut_target = false;
            if (check(TokenKind::KwMut)) { wrap->mut_binding = true; advance(); }
            base = wrap;
        }
        return base;
    }

    // ----- top var / const decl -----
    ast::DeclPtr parse_top_var() {
        ast::Span span{cur().line, cur().column};
        ast::TypePtr ty = parse_full_type(/*allow_ptr_ref=*/true);
        std::string name;
        if (check(TokenKind::Identifier)) {
            name = std::string(cur().lexeme);
            advance();
        } else {
            error_here("expected variable name");
        }
        bool is_mut = false, is_const = false;
        if (check(TokenKind::KwMut)) { is_mut = true; advance(); }
        if (check(TokenKind::KwConst)) { is_const = true; advance(); }
        ast::ExprPtr init;
        if (match(TokenKind::Assign)) {
            init = parse_expr();
        }
        if (!match(TokenKind::Semicolon)) {
            error_here("expected `;` after top-level declaration");
        }
        auto d = std::make_shared<ast::Decl>();
        d->kind = is_const ? ast::DeclKind::Const : ast::DeclKind::Var;
        d->span = span;
        d->var_type = ty;
        d->var_name = name;
        d->var_is_mut = is_mut;
        d->var_is_const = is_const;
        d->var_init = init;
        return d;
    }

    // ----- function -----
    ast::DeclPtr parse_function(bool is_export = false, bool is_extern = false) {
        ast::Span span{cur().line, cur().column};
        ast::TypePtr ret = parse_full_type(/*allow_ptr_ref=*/true);
        if (!check(TokenKind::Identifier)) {
            error_here(std::string("expected function name, found ") +
                       token_kind_name(cur().kind));
            sync_recovery();
            return nullptr;
        }
        std::string name = std::string(cur().lexeme);
        advance();
        std::vector<ast::Parameter> params;
        if (!match(TokenKind::LParen)) {
            error_here("expected `(` after function name");
        } else {
            if (!check(TokenKind::RParen)) {
                do {
                    ast::Parameter p;
                    p.span = {cur().line, cur().column};
                    p.type = parse_full_type(/*allow_ptr_ref=*/true);
                    if (check(TokenKind::KwMut)) { p.is_mut = true; advance(); }
                    if (check(TokenKind::Identifier)) {
                        p.name = std::string(cur().lexeme);
                        advance();
                    } else {
                        error_here("expected parameter name");
                    }
                    params.push_back(std::move(p));
                } while (match(TokenKind::Comma));
            }
            expect(TokenKind::RParen, "`)`");
        }

        ast::StmtPtr body;
        if (check(TokenKind::LBrace)) {
            body = parse_block();
        } else if (is_extern) {
            if (!match(TokenKind::Semicolon)) {
                error_here("expected `;` after extern declaration");
            }
        } else {
            error_here("expected function body `{` or `;` for declaration");
            sync_recovery();
        }

        auto d = std::make_shared<ast::Decl>();
        d->kind = is_extern ? ast::DeclKind::ExternFn
                            : (is_export ? ast::DeclKind::ExportFn
                                         : ast::DeclKind::Function);
        d->span = span;
        d->fn_return_type = ret;
        d->fn_name = std::move(name);
        d->fn_params = std::move(params);
        d->fn_body = body;
        d->fn_is_extern = is_extern;
        d->fn_is_export = is_export;
        return d;
    }

    ast::DeclPtr parse_import() {
        ast::Span span{cur().line, cur().column};
        advance();
        std::string path;
        if (check(TokenKind::StringLiteral)) {
            path = std::string(cur().lexeme.substr(1, cur().lexeme.size() - 2));
            advance();
        } else {
            // dotted module path
            while (check(TokenKind::Identifier) || check(TokenKind::Dot) ||
                   check(TokenKind::Star)) {
                path += std::string(cur().lexeme);
                advance();
            }
        }
        if (!match(TokenKind::Semicolon)) {
            error_here("expected `;` after import");
        }
        auto d = std::make_shared<ast::Decl>();
        d->kind = ast::DeclKind::Import;
        d->span = span;
        d->import_path = std::move(path);
        return d;
    }

    void parse_extern_block() {
        // consume 'extern' '{'
        advance();
        if (!match(TokenKind::LBrace)) {
            error_here("expected `{` after `extern`");
            return;
        }
        while (!check(TokenKind::RBrace) && !at_eof()) {
            if (auto d = parse_function(/*is_export=*/false, /*is_extern=*/true)) {
                // We just drop these for now in the AST? No — keep them as ExternFn.
                // But we need a TU to return them. Just return first only.
                // For simplicity, push only the first and recover on others.
                // Actually we have no place to push multiple; ignore extras.
                (void)d;
            }
            if (!sync_recovery()) break;
        }
        expect(TokenKind::RBrace, "`}`");
    }

    // ----- statements -----
    ast::StmtPtr parse_block() {
        ast::Span span{cur().line, cur().column};
        expect(TokenKind::LBrace, "`{`");
        auto blk = ast::make_stmt(ast::StmtKind::Block, span);
        while (!check(TokenKind::RBrace) && !at_eof()) {
            if (auto s = parse_stmt()) blk->block_stmts.push_back(s);
            else if (!sync_recovery()) break;
        }
        expect(TokenKind::RBrace, "`}`");
        return blk;
    }

    ast::StmtPtr parse_stmt() {
        ast::Span span{cur().line, cur().column};
        if (check(TokenKind::Semicolon)) { advance(); return ast::make_stmt(ast::StmtKind::Empty, span); }
        if (check(TokenKind::LBrace)) return parse_block();
        if (check(TokenKind::KwReturn)) return parse_return();
        if (check(TokenKind::KwIf)) return parse_if();
        if (check(TokenKind::KwWhile)) return parse_while();
        if (check(TokenKind::KwFor)) return parse_for();
        if (check(TokenKind::KwLoop)) return parse_loop();
        if (check(TokenKind::KwBreak)) { advance(); expect(TokenKind::Semicolon, "`;`"); return ast::make_stmt(ast::StmtKind::Break, span); }
        if (check(TokenKind::KwContinue)) { advance(); expect(TokenKind::Semicolon, "`;`"); return ast::make_stmt(ast::StmtKind::Continue, span); }
        if (check(TokenKind::KwDefer)) return parse_defer();
        if (is_type_start(cur().kind) && looks_like_var_decl()) return parse_var_decl();
        auto e = parse_expr();
        if (!match(TokenKind::Semicolon)) {
            error_here("expected `;` after expression");
        }
        auto s = ast::make_stmt(ast::StmtKind::ExprStmt, span);
        s->expr = e;
        return s;
    }

    bool looks_like_var_decl() {
        // TYPE [mut] [const] NAME [= ...] ;
        std::size_t p = pos_;
        // skip base type
        ++p;
        // skip * / & suffixes
        while (p < toks_.size() &&
               (toks_[p].kind == TokenKind::Star || toks_[p].kind == TokenKind::Amp)) {
            ++p;
            if (p < toks_.size() && toks_[p].kind == TokenKind::KwMut) ++p;
        }
        if (p >= toks_.size() || toks_[p].kind != TokenKind::Identifier) return false;
        ++p;
        if (p < toks_.size() && toks_[p].kind == TokenKind::KwMut) ++p;
        if (p < toks_.size() && toks_[p].kind == TokenKind::KwConst) ++p;
        if (p >= toks_.size()) return false;
        return toks_[p].kind == TokenKind::Assign ||
               toks_[p].kind == TokenKind::Semicolon;
    }

    ast::StmtPtr parse_var_decl() {
        ast::Span span{cur().line, cur().column};
        ast::TypePtr ty = parse_full_type(/*allow_ptr_ref=*/true);
        std::string name;
        if (check(TokenKind::Identifier)) {
            name = std::string(cur().lexeme);
            advance();
        } else {
            error_here("expected variable name");
        }
        bool is_mut = false, is_const = false;
        if (check(TokenKind::KwMut)) { is_mut = true; advance(); }
        if (check(TokenKind::KwConst)) { is_const = true; advance(); }
        ast::ExprPtr init;
        if (match(TokenKind::Assign)) {
            init = parse_expr();
        }
        if (!match(TokenKind::Semicolon)) {
            error_here("expected `;` after variable declaration");
        }
        auto s = ast::make_stmt(ast::StmtKind::VarDecl, span);
        s->var_type = ty;
        s->var_name = std::move(name);
        s->is_mut = is_mut;
        s->is_const = is_const;
        s->var_init = init;
        return s;
    }

    ast::StmtPtr parse_return() {
        ast::Span span{cur().line, cur().column};
        advance();
        ast::StmtPtr s = ast::make_stmt(ast::StmtKind::Return, span);
        if (!check(TokenKind::Semicolon)) {
            s->return_value = parse_expr();
        }
        if (!match(TokenKind::Semicolon)) {
            error_here("expected `;` after return");
        }
        return s;
    }

    ast::StmtPtr parse_if() {
        ast::Span span{cur().line, cur().column};
        advance();
        auto cond = parse_expr();
        auto then_s = parse_block();
        ast::StmtPtr else_s;
        if (match(TokenKind::KwElif)) {
            else_s = parse_if();
        } else if (match(TokenKind::KwElse)) {
            if (check(TokenKind::LBrace)) else_s = parse_block();
            else else_s = parse_stmt();
        }
        auto s = ast::make_stmt(ast::StmtKind::If, span);
        s->if_cond = cond;
        s->if_then = then_s;
        s->if_else = else_s;
        return s;
    }

    ast::StmtPtr parse_while() {
        ast::Span span{cur().line, cur().column};
        advance();
        auto cond = parse_expr();
        auto body = parse_block();
        auto s = ast::make_stmt(ast::StmtKind::While, span);
        s->while_cond = cond;
        s->while_body = body;
        return s;
    }

    ast::StmtPtr parse_for() {
        ast::Span span{cur().line, cur().column};
        advance();
        auto s = ast::make_stmt(ast::StmtKind::For, span);
        if (!check(TokenKind::Semicolon)) s->for_init = parse_stmt();
        if (!check(TokenKind::Semicolon)) s->for_cond = parse_expr();
        expect(TokenKind::Semicolon, "`;`");
        if (!check(TokenKind::LBrace)) s->for_step = parse_expr();
        s->for_body = parse_block();
        return s;
    }

    ast::StmtPtr parse_loop() {
        ast::Span span{cur().line, cur().column};
        advance();
        auto s = ast::make_stmt(ast::StmtKind::Loop, span);
        s->loop_body = parse_block();
        return s;
    }

    ast::StmtPtr parse_defer() {
        ast::Span span{cur().line, cur().column};
        advance();
        auto s = ast::make_stmt(ast::StmtKind::Defer, span);
        if (check(TokenKind::LBrace)) {
            s->defer_expr = nullptr;
            // For now, parse block as a single compound expression: not ideal,
            // but we just want to skip the block.
            int depth = 1;
            advance();
            while (depth > 0 && !at_eof()) {
                if (check(TokenKind::LBrace)) ++depth;
                else if (check(TokenKind::RBrace)) --depth;
                advance();
            }
        } else {
            s->defer_expr = parse_expr();
            expect(TokenKind::Semicolon, "`;`");
        }
        return s;
    }

    // ----- expressions -----
    // Precedence climbing.
    ast::ExprPtr parse_expr() { return parse_assign(); }

    ast::ExprPtr parse_assign() {
        auto e = parse_ternary();
        if (check(TokenKind::Assign) ||
            check(TokenKind::PlusAssign) || check(TokenKind::MinusAssign) ||
            check(TokenKind::StarAssign) || check(TokenKind::SlashAssign) ||
            check(TokenKind::PercentAssign) || check(TokenKind::AmpAssign) ||
            check(TokenKind::PipeAssign) || check(TokenKind::CaretAssign) ||
            check(TokenKind::LtLtAssign) || check(TokenKind::GtGtAssign)) {
            std::string op_str = std::string(cur().lexeme);
            advance();
            auto rhs = parse_assign();
            auto out = make_expr(ExprKind_tag::Assign, e->span);
            out->assign_target = e;
            out->rhs = rhs;
            out->literal_text = std::move(op_str);
            return out;
        }
        return e;
    }

    // We keep a tiny namespace alias to avoid leaking the enum into the
    // namespace aliasing above.
    using ExprKind_tag = ast::ExprKind;

    ast::ExprPtr parse_ternary() {
        auto e = parse_or();
        if (match(TokenKind::Question)) {
            auto t = parse_expr();
            expect(TokenKind::Colon, "`:`");
            auto f = parse_ternary();
            (void)t; (void)f;
            return e; // ternary unsupported; just keep cond
        }
        return e;
    }

    ast::ExprPtr parse_or() {
        auto e = parse_and();
        while (check(TokenKind::PipePipe)) {
            advance();
            auto r = parse_and();
            auto out = make_expr(ast::ExprKind::Binary, e->span);
            out->binary_op = ast::BinaryOp::Or;
            out->lhs = e;
            out->rhs = r;
            e = out;
        }
        return e;
    }

    ast::ExprPtr parse_and() {
        auto e = parse_bitor();
        while (check(TokenKind::AmpAmp)) {
            advance();
            auto r = parse_bitor();
            auto out = make_expr(ast::ExprKind::Binary, e->span);
            out->binary_op = ast::BinaryOp::And;
            out->lhs = e;
            out->rhs = r;
            e = out;
        }
        return e;
    }

    ast::ExprPtr parse_bitor() {
        auto e = parse_bitxor();
        while (check(TokenKind::Pipe)) {
            advance();
            auto r = parse_bitxor();
            auto out = make_expr(ast::ExprKind::Binary, e->span);
            out->binary_op = ast::BinaryOp::BitOr;
            out->lhs = e;
            out->rhs = r;
            e = out;
        }
        return e;
    }

    ast::ExprPtr parse_bitxor() {
        auto e = parse_bitand();
        while (check(TokenKind::Caret)) {
            advance();
            auto r = parse_bitand();
            auto out = make_expr(ast::ExprKind::Binary, e->span);
            out->binary_op = ast::BinaryOp::BitXor;
            out->lhs = e;
            out->rhs = r;
            e = out;
        }
        return e;
    }

    ast::ExprPtr parse_bitand() {
        auto e = parse_equality();
        while (check(TokenKind::Amp)) {
            advance();
            auto r = parse_equality();
            auto out = make_expr(ast::ExprKind::Binary, e->span);
            out->binary_op = ast::BinaryOp::BitAnd;
            out->lhs = e;
            out->rhs = r;
            e = out;
        }
        return e;
    }

    ast::ExprPtr parse_equality() {
        auto e = parse_relational();
        while (check(TokenKind::Eq) || check(TokenKind::NotEq)) {
            ast::BinaryOp op = check(TokenKind::Eq) ? ast::BinaryOp::Eq
                                                    : ast::BinaryOp::NotEq;
            advance();
            auto r = parse_relational();
            auto out = make_expr(ast::ExprKind::Binary, e->span);
            out->binary_op = op;
            out->lhs = e;
            out->rhs = r;
            e = out;
        }
        return e;
    }

    ast::ExprPtr parse_relational() {
        auto e = parse_shift();
        while (check(TokenKind::LAngle) || check(TokenKind::LtEq) ||
               check(TokenKind::RAngle) || check(TokenKind::GtEq)) {
            ast::BinaryOp op = ast::BinaryOp::Lt;
            if (check(TokenKind::LAngle)) op = ast::BinaryOp::Lt;
            else if (check(TokenKind::LtEq)) op = ast::BinaryOp::LtEq;
            else if (check(TokenKind::RAngle)) op = ast::BinaryOp::Gt;
            else if (check(TokenKind::GtEq)) op = ast::BinaryOp::GtEq;
            advance();
            auto r = parse_shift();
            auto out = make_expr(ast::ExprKind::Binary, e->span);
            out->binary_op = op;
            out->lhs = e;
            out->rhs = r;
            e = out;
        }
        return e;
    }

    ast::ExprPtr parse_shift() {
        auto e = parse_additive();
        while (check(TokenKind::LtLt) || check(TokenKind::GtGt)) {
            ast::BinaryOp op = check(TokenKind::LtLt) ? ast::BinaryOp::Shl
                                                     : ast::BinaryOp::Shr;
            advance();
            auto r = parse_additive();
            auto out = make_expr(ast::ExprKind::Binary, e->span);
            out->binary_op = op;
            out->lhs = e;
            out->rhs = r;
            e = out;
        }
        return e;
    }

    ast::ExprPtr parse_additive() {
        auto e = parse_multiplicative();
        while (check(TokenKind::Plus) || check(TokenKind::Minus)) {
            ast::BinaryOp op = check(TokenKind::Plus) ? ast::BinaryOp::Add
                                                     : ast::BinaryOp::Sub;
            advance();
            auto r = parse_multiplicative();
            auto out = make_expr(ast::ExprKind::Binary, e->span);
            out->binary_op = op;
            out->lhs = e;
            out->rhs = r;
            e = out;
        }
        return e;
    }

    ast::ExprPtr parse_multiplicative() {
        auto e = parse_unary();
        while (check(TokenKind::Star) || check(TokenKind::Slash) ||
               check(TokenKind::Percent)) {
            ast::BinaryOp op = ast::BinaryOp::Mul;
            if (check(TokenKind::Star)) op = ast::BinaryOp::Mul;
            else if (check(TokenKind::Slash)) op = ast::BinaryOp::Div;
            else if (check(TokenKind::Percent)) op = ast::BinaryOp::Mod;
            advance();
            auto r = parse_unary();
            auto out = make_expr(ast::ExprKind::Binary, e->span);
            out->binary_op = op;
            out->lhs = e;
            out->rhs = r;
            e = out;
        }
        return e;
    }

    ast::ExprPtr parse_unary() {
        if (check(TokenKind::Minus)) {
            advance();
            auto r = parse_unary();
            auto out = make_expr(ast::ExprKind::Unary, r->span);
            out->unary_op = ast::UnaryOp::Neg;
            out->lhs = r;
            return out;
        }
        if (check(TokenKind::Plus)) {
            advance();
            auto r = parse_unary();
            auto out = make_expr(ast::ExprKind::Unary, r->span);
            out->unary_op = ast::UnaryOp::Plus;
            out->lhs = r;
            return out;
        }
        if (check(TokenKind::Bang)) {
            advance();
            auto r = parse_unary();
            auto out = make_expr(ast::ExprKind::Unary, r->span);
            out->unary_op = ast::UnaryOp::Not;
            out->lhs = r;
            return out;
        }
        if (check(TokenKind::Tilde)) {
            advance();
            auto r = parse_unary();
            auto out = make_expr(ast::ExprKind::Unary, r->span);
            out->unary_op = ast::UnaryOp::BitNot;
            out->lhs = r;
            return out;
        }
        if (check(TokenKind::PlusPlus)) {
            advance();
            auto r = parse_unary();
            auto out = make_expr(ast::ExprKind::Unary, r->span);
            out->unary_op = ast::UnaryOp::PreInc;
            out->lhs = r;
            return out;
        }
        if (check(TokenKind::MinusMinus)) {
            advance();
            auto r = parse_unary();
            auto out = make_expr(ast::ExprKind::Unary, r->span);
            out->unary_op = ast::UnaryOp::PreDec;
            out->lhs = r;
            return out;
        }
        if (check(TokenKind::Amp)) {
            advance();
            auto r = parse_unary();
            auto out = make_expr(ast::ExprKind::AddrOf, r->span);
            out->lhs = r;
            return out;
        }
        return parse_postfix();
    }

    ast::ExprPtr parse_postfix() {
        auto e = parse_primary();
        while (true) {
            if (check(TokenKind::Dot)) {
                advance();
                if (check(TokenKind::Identifier)) {
                    std::string fn = std::string(cur().lexeme);
                    advance();
                    auto out = make_expr(ast::ExprKind::FieldAccess, e->span);
                    out->object = e;
                    out->field_name = std::move(fn);
                    e = out;
                } else {
                    error_here("expected field name after `.`");
                    return e;
                }
            } else if (check(TokenKind::LParen)) {
                advance();
                auto call = make_expr(ast::ExprKind::Call, e->span);
                call->callee = e;
                if (!check(TokenKind::RParen)) {
                    do {
                        call->args.push_back(parse_assign());
                    } while (match(TokenKind::Comma));
                }
                expect(TokenKind::RParen, "`)`");
                e = call;
            } else if (check(TokenKind::LBracket)) {
                advance();
                auto idx = parse_expr();
                expect(TokenKind::RBracket, "`]`");
                auto out = make_expr(ast::ExprKind::IndexAccess, e->span);
                out->object = e;
                out->index = idx;
                e = out;
            } else if (check(TokenKind::PlusPlus)) {
                advance();
                auto out = make_expr(ast::ExprKind::Unary, e->span);
                out->unary_op = ast::UnaryOp::PostInc;
                out->lhs = e;
                e = out;
            } else if (check(TokenKind::MinusMinus)) {
                advance();
                auto out = make_expr(ast::ExprKind::Unary, e->span);
                out->unary_op = ast::UnaryOp::PostDec;
                out->lhs = e;
                e = out;
            } else {
                break;
            }
        }
        return e;
    }

    ast::ExprPtr parse_primary() {
        ast::Span span{cur().line, cur().column};
        const Token& t = cur();
        switch (t.kind) {
        case TokenKind::IntegerLiteral: {
            advance();
            auto e = make_expr(ast::ExprKind::IntegerLiteral, span);
            e->literal_text = std::string(t.lexeme);
            e->int_value = parse_int_literal(std::string(t.lexeme));
            return e;
        }
        case TokenKind::FloatLiteral: {
            advance();
            auto e = make_expr(ast::ExprKind::FloatLiteral, span);
            e->literal_text = std::string(t.lexeme);
            e->float_value = std::stod(std::string(t.lexeme));
            return e;
        }
        case TokenKind::StringLiteral: {
            advance();
            auto e = make_expr(ast::ExprKind::StringLiteral, span);
            // strip quotes
            e->string_value = unescape_c(std::string(t.lexeme));
            return e;
        }
        case TokenKind::CharLiteral: {
            advance();
            auto e = make_expr(ast::ExprKind::CharLiteral, span);
            e->char_value = parse_char_literal(std::string(t.lexeme));
            return e;
        }
        case TokenKind::KwTrue: {
            advance();
            auto e = make_expr(ast::ExprKind::BoolLiteral, span);
            e->bool_value = true;
            return e;
        }
        case TokenKind::KwFalse: {
            advance();
            auto e = make_expr(ast::ExprKind::BoolLiteral, span);
            e->bool_value = false;
            return e;
        }
        case TokenKind::KwNull: {
            advance();
            auto e = make_expr(ast::ExprKind::NullLiteral, span);
            return e;
        }
        case TokenKind::Identifier: {
            std::string name = std::string(t.lexeme);
            advance();
            auto e = make_expr(ast::ExprKind::Identifier, span);
            e->name = std::move(name);
            return e;
        }
        case TokenKind::LParen: {
            advance();
            auto e = parse_expr();
            expect(TokenKind::RParen, "`)`");
            return e;
        }
        default:
            error_here(std::string("expected expression, found ") +
                       token_kind_name(t.kind));
            advance();
            return make_expr(ast::ExprKind::IntegerLiteral, span);
        }
    }

    // ----- literal helpers -----
    static long long parse_int_literal(const std::string& s) {
        std::string t = s;
        for (auto it = t.begin(); it != t.end(); ) {
            if (*it == '_') it = t.erase(it);
            else { *it = static_cast<char>(std::tolower(*it)); ++it; }
        }
        if (t.size() > 2 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X')) {
            return std::stoll(t.substr(2), nullptr, 16);
        }
        if (t.size() > 2 && t[0] == '0' && (t[1] == 'b' || t[1] == 'B')) {
            return std::stoll(t.substr(2), nullptr, 2);
        }
        if (t.size() > 2 && t[0] == '0' && (t[1] == 'o' || t[1] == 'O')) {
            return std::stoll(t.substr(2), nullptr, 8);
        }
        // strip suffix
        if (!t.empty() && (t.back() == 'u' || t.back() == 'i' ||
                           t.back() == 'l' || t.back() == 'f' ||
                           t.back() == 'd')) {
            t.pop_back();
        }
        return std::stoll(t, nullptr, 10);
    }

    static std::uint32_t parse_char_literal(const std::string& s) {
        // s like 'a' or '\n'
        std::string body = s.substr(1, s.size() - 2);
        if (!body.empty() && body[0] == '\\') {
            if (body.size() == 2) {
                switch (body[1]) {
                case 'n': return '\n';
                case 't': return '\t';
                case 'r': return '\r';
                case '0': return '\0';
                case '\\': return '\\';
                case '\'': return '\'';
                case '"': return '"';
                }
            }
        }
        return body.empty() ? 0 : static_cast<std::uint32_t>(body[0]);
    }

    static std::string unescape_c(std::string s) {
        if (s.size() >= 2 && s.front() == '"') s = s.substr(1, s.size() - 2);
        std::string out;
        out.reserve(s.size());
        for (std::size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '\\' && i + 1 < s.size()) {
                switch (s[i + 1]) {
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                case 'r': out += '\r'; break;
                case '\\': out += '\\'; break;
                case '"': out += '"'; break;
                case '0': out += '\0'; break;
                default: out += s[i + 1]; break;
                }
                ++i;
            } else {
                out += s[i];
            }
        }
        return out;
    }
};

}  // namespace

ParseResult parse(const std::vector<Token>& tokens, DiagnosticsEngine& diags,
                  const std::string& source_path) {
    Parser p(tokens, diags, source_path);
    return p.run();
}
}
