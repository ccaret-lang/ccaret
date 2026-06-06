# C^ (CCaret) Core Language Specification

## 1. Source Layout & Modules

* **File Extension:** `.cca` (No header files like `.h` or `.hp` exist in the ecosystem).
* **Module Import:** `import std.io;` (Compiler resolves file dependencies directly in a single pass).
* **Statement Termination:** explicit semicolons `;`.

---

## 2. Type System & Variable Mutation

* **Immutable Default:** Variables are read-only unless flagged with `mut`.
* **Compile-Time Constant:** `const` defines values computed entirely during compilation.
* **Variable Syntax Layout:** `Type [Modifier] Identifier = Value;`
* **Syntax Examples:**
* Immutable Stack Scalar: `i32 velocity = 250;`
* Mutable Stack Scalar: `i32 mut score = 0;`
* Global Compile-Time Constant: `u64 const MAX_LIMIT = 0xFFFFFFFF;`



### Primitive Bit-Width Matrix

* **Signed Integers:** `i8`, `i16`, `i32`, `i64`, `i128`
* **Unsigned Integers:** `u8`, `u16`, `u32`, `u64`, `u128`
* **Floating Point:** `f32`, `f64`
* **Logical & Text Primitives:** `bool` (`true`/`false`), `char` (32-bit UTF-8), `void` (empty return signature).

---

## 3. Memory Model Mechanics

* **References (`T&`):** Safe, non-nullable aliases that automatically dereference on use.
* Syntax: `i32 mut health = 100; i32& ref = health; ref = 90;`


* **Raw Pointers (`T*`):** Explicit hardware addresses that can be `null`.
* **Postfix Dereference (`ptr.*`):** Banish prefix asterisk visual bugs.
* Syntax: `i32 mut val = 5; i32* ptr = &val; ptr.* = 20;`


* **Mutability Placement Rule:**
  * `mut` **before** `*`/`&` = mutable **target** data
  * `mut` **after** `*`/`&` = mutable **pointer/reference binding**
  * Examples: `T mut*` = mutable target, immutable binding; `T* mut` = immutable target, mutable binding; `T mut* mut` = both mutable


* **Memory Arenas:** Dynamic allocations must utilize an explicit local allocator memory pool instance.
* **Deterministic Guard (`defer`):** Delays statement execution until parent scope block closes.
* Syntax: `Arena mut pool = Arena.init(sys.allocator); defer pool.deinit();`



---

## 4. Compound Types & Data Layouts

* **Struct Blocks:** Contiguous fixed memory properties separated by newlines or semicolons.
* Syntax: `struct Coordinate { f64 x; f64 y; }`


* **Union Blocks:** Absolute bitwise overlap segments.
* Syntax: `union NumericData { raw_bits(u32); bytes[u8[4]]; }`


* **Enum Units:** Uniformly scoped lists using commas or newlines.
* Syntax: `enum Color { Red, Green, Blue }`


* **Tuples:** Anonymous mismatched collections tracked via native inline index positions.
* Syntax: `(i32, bool) packet = (200, true); i32 code = packet.0;`



---

## 5. Control Flow & Patterns

* **If Conditionals:** No parenthesis surrounding boolean algebra equations.
* Syntax: `if count == 0 { ... } elif count < 10 { ... } else { ... }`


* **Range-Based For Loops:** Uses exclusive double-dot assignment step layout blocks.
* Syntax: `for i32 i = 0..10 { ... }`


* **While Loops:** Conditional execution blocks.
* Syntax: `while working == true { ... }`


* **Loop Statements:** Condition-free high-performance infinite iteration loops.
* Syntax: `loop { if done { break; } }`


* **Exhaustive Match Expressions:** Safe alternative to legacy switch blocks with zero fall-through.
* Syntax:
```c
match status {
    Color.Red => { handle_red(); }
    Color.Blue => { handle_blue(); }
    else => { handle_fallback(); }
}

```





---

## 6. Functional Architectures & Traits (`trait`)

* **Data and Logic Split:** Data is contained in pure Struct models; methods live in isolated `impl` execution blocks.
* **Method Binding Self Tokens:** Methods label variable capture context using explicit state tags: `this` (read-only copy), `this&` (immutable reference link), or `this& mut` (mutable reference link).
* Syntax:
```c
struct Drone { i32 alt; }
impl Drone {
    void ascend(this& mut, i32 amount) { this.alt += amount; }
}

```




* **Trait Contracts (`trait`):** Pure compile-time interface contracts with zero runtime overhead cost.
* Syntax:
```c
trait Agent { void task(this); }
impl Drone to Agent { void task(this) { ... } }

```





---

## 7. Error Handling Architecture

* **Dedicated Error Sets:** `error` defines isolated compile-time tokens completely separated from standard enums.
* Syntax: `error DeviceFault { Timeout, Disconnected }`


* **Error Union Type (`!`):** Return signatures bind failure codes directly to standard value slots.
* Syntax: `i32!DeviceFault read_port() { return DeviceFault.Timeout; }`


* **Error Bubbling Operator (`try`):** Automatically bubbles failures up to caller execution frames.
* Syntax: `i32 data = try read_port();`


* **Fallback Interceptor (`catch`):** Halts failure tracks natively with direct default values or inline catch execution blocks.
* Syntax: `i32 status = read_port() catch 0;`
* Syntax Block: `i32 code = read_port() catch { log_fault(err); return; };`



---

## 8. Strings & C Interoperability

* **String Allocation Strategy:** Fat pointer slices containing a raw data reference pointer (`u8* data`) alongside an explicit 64-bit size counter (`u64 len`).
* **Balanced String Formatting:** Uses compile-time verified inferred brackets `{}` for content streaming.
* Syntax: `println("User {} checked in.", name);`
* Configured Modifier Syntax: `println("Hex code: { :x }", memory_address);`


* **Native C ABI Interoperability:** Uses the `extern` identifier layout context to link with legacy `libc` functions with zero overhead.
* Syntax: `extern i32 printf(u8* format, ...);`
