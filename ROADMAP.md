# C^: v0.1.0 Roadmap

> **Status:** working
> **Target tag:** `v0.1.0`
> **Codename:** *Blueberry*
> **Audience:** C and C++ developers who want a "C with better control"

This document is the contract for what ships in `v0.1.0` and what is explicitly deferred. It is intentionally narrow. We are not trying to ship a complete language in this release; we are shipping a **usable subset** with working tooling, a real test suite, and a efficient standard library that proves the design.

---

## Scope: In vs Out

### In Scope (must ship)

#### Language

- Primitives: `i8`..`i128`, `u8`..`u128`, `f32`, `f64`, `bool`, `char`, `void`, `string`
- `auto` type inference
- Variables: `mut`, `const`, immutable by default
- Functions with parameters, return type, `export`
- `extern c` (single declaration and block)
- `import` (namespace, wildcard, `std.io`, `lib.test_math` style)
- `link "lib"`
- Structs (with default field values, `packed`)
- Enums (with explicit values)
- Unions: plain and tagged
- Pointers: `T*`, `T mut*`, `T* mut`, with postfix `.*` deref
- References: `T&`, `T mut&`, with auto-deref
- Arrays: `T[N]`, `T[_]`, with `ArrayLiteral`
- Slices: `T[]`, with range slicing `arr[a..b]`
- Optionals: `T?`, with `??` coalesce
- Control flow: `if`/`elif`/`else`, `while`, `for` (C-style + range), `loop`, `match`
- `defer` (LIFO, both statement and block form)
- `try` / `catch` / `error` sets / `!` error-union return type
- `volatile` qualifier (full end-to-end)
- Cast `(T)x`, ternary `c ? a : b`, `++`/`--`, compound assignment

#### Builtins
- All 13 type-introspection builtins: `@sizeof`, `@alignof`, `@typeof`, `@type_name`, `@offsetof`, `@has_field`, `@implements`, `@is_same`, `@is_signed`, `@is_unsigned`, `@is_integral`, `@is_floating`, `@is_pointer`
- `@panic(msg)` (runtime abort with message)

#### Standard library (`std/`)
- `std.io` — `print`, `println` (variadic, format-string `{}` placeholders)
- `std.string` — `string.from_cstr(cstr)`, `string.eq(a, b)`, `len(s)`

#### CI
- All 8 OS workflows green: Linux, macOS, Windows, FreeBSD, OpenBSD, NetBSD, Dragonfly, illumos
- Each workflow builds, runs the test runner, and uploads a binary artifact

### Out of scope (deferred to v0.2.0+)

| Feature | Target |
|---|---|
| Generics (`<T>`) | v0.2.0 |
| Traits + `impl` blocks | v0.2.0 |
| Tuples | v0.2.0 |
| `import c "header.h"` | v0.2.0 |
| Concurrency builtins (`@atomic_*`, `@fence`, `@cpu_relax`) | v0.3.0 |
| Volatile/atomic memory ordering | v0.3.0 |
| Static asserts, `@compile_error`, `@unreachable`, `@bit_cast` | v0.3.0 |
| Self-hosted compiler | v1.0.0 (or never — to be decided) |
| Package manager | v0.4.0+ |
| LSP | v0.4.0+ |

---

## Detailed Task List (per area, for assignability)

### Language / compiler

- [ ] Fix `import` resolution so `examples/hello.cca` works
- [ ] Implement `volatile` end-to-end
- [ ] Implement `error` sets
- [ ] Implement `!` error-union return type
- [ ] Implement `try` / `catch`
- [ ] Implement `import c "header.h"`
- [x] Embed version string in binary, expose `--version` and `--help`
- [ ] Audit and complete all 13 `@builtins`
- [ ] Add `@panic(msg)` builtin
- [ ] Add `@addr_of`, `@volatile_load`, `@volatile_store` builtins (per `syntax/builtins.md` §Memory & Pointer and §Volatile & Atomic)
- [x] Lexer: support hex (`0xFF`), octal (`0o755`), binary (`0b1010`), and underscore separators (`0xDEAD_BEEF`)

### Standard library


### Tooling

- [ ] Test runner C++ binary
- [ ] Convert all `tests/test_*.cca` to `// EXPECT:` format
- [ ] `make install` with `GNUInstallDirs`
- [ ] CI smoke test in every workflow
- [ ] `RELEASING.md`

### Release machinery

- [x] `LICENSE` (Apache-2.0) — already shipped
- [ ] `CHANGELOG.md` with v0.1.0 entry
- [ ] GitHub release with binaries for Linux/macOS/Windows
- [ ] `README.md` "Getting Started" section

### Hygiene

- [ ] Add `.editorconfig`
- [ ] Add `SECURITY.md` (how to report a vulnerability)

---

## After v0.1.0

What comes next is intentionally not detailed here, but the rough order is:

1. **v0.1.1** — bug fixes reported in the first 2 weeks
2. **v0.2.0** — generics + traits + impl (the big one)
3. **v0.3.0** — concurrency builtins, static asserts, full volatile story
4. **v0.4.0** — LSP, package manager, C header import
5. **v1.0.0** — promise of C ABI stability; possibly self-hosted compiler

---

## How to use this document

- If you are picking up a task, find it in §5 and link your PR to its checkbox.
- If you want to add scope, write a new milestone in §4 and get it merged *before* you start coding.
- If you want to remove scope, edit §3.1 in the same PR that removes the code.
- This document is the source of truth for what v0.1.0 means. If `checklist.txt` and `ROADMAP.md` disagree, `ROADMAP.md` wins.
