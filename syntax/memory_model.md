# Memory Model

Value semantics. Copy by default. Heap is explicit.

## Stack (default)

No allocator. No heap. Assignment copies the value.

```c
i32 x = 10;
i32 y = x;         // copy — x and y are independent
Point p = Point{x = 1; y = 2};
```

## T& — Safe References

Non-nullable alias. Must not outlive the referenced value. Zero cost.

```c
i32 mut val = 10;
i32& ref = val;
ref = 20;           // auto-deref
```

## T* — Raw Pointers

Nullable, unchecked. `null` is the zero value.

```c
i32* ptr = &val;
i32 data = ptr.*;
i32* empty = null;
```

## T? — Optional Types

Nullable but safe — must unwrap before use.

```c
i32? maybe = try get();
i32 v = maybe ?? default;
i32 v = maybe ?? -1;
```

## T[] — Slices

View into contiguous memory. Carries length. Bounds-checked at runtime.

```c
i32[4] arr = [1, 2, 3, 4];
i32[] slice = arr[0..2];
u64 n = slice.len;
```

## Fat Arrays

Every array carries its length at runtime.

```c
i32[4] arr = [1, 2, 3, 4];
i32 x = arr[10];  // runtime bounds check → error, not crash
```

## const — Compile-Time Constant

Evaluated at compile time, not a runtime variable.

```c
i32 const MAX_BUF = 4096;
string const NAME = "C^";
```

## Struct Layout

Natural alignment by default. `packed` suppresses padding.

```c
struct S { u8 a; i32 b; };         // 8 bytes (3 bytes padding)
packed struct P { u8 a; i32 b; };  // 5 bytes
```

## sizeof / alignof

Query type size and alignment at compile time.

```cpp
usize sz = @sizeof(i32);    // 4
usize al = @alignof(i32);   // 4
```

## volatile

Disables compiler optimisations. Required for memory-mapped IO.

```c
volatile u8* reg = (volatile u8*)0x4000;
reg.* = 0xFF;
```

## Defer

Cleanup runs when scope exits. Defers execute LIFO.

```c
void* data = alloc(u8, 256);
defer free(data);
```

## Allocators

`std.heap` provides 5 allocators:

| Allocator | Behaviour | Best for |
|---|---|---|
| `page` | OS pages (mmap) | large blocks, backing others |
| `arena` | bump + bulk free | temporary scratch work |
| `pool` | fixed-size blocks | many same-size objects |
| `stack` | stack-based (alloca) | fast temporary buffers |
| `c` | wraps malloc/free | C interop only |

```c
import std.heap;

heap.arena mut a = heap.arena.init(4096);
defer a.deinit();
i32* data = a.alloc(i32, 10);
```

## Errors over Crashes

Allocation failure is a returned error, not a segfault.

```c
i32! data = std.heap.page.alloc(i32, 1_000_000_000);
i32* ptr = try data;
```
