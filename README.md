# C^ (CCaret): dev-v0.1.0

This is a official source code for the C^ Programming language.

## Philosophy

Meaningful. Accurate. Simple. Maxium Performace.

## Toolchain

`caret` is our toolchain which under the hood handles the following:
- `caretc`: this compiler
- `caretv`: version manager
- `caretp`: package manager

## Syntax

```c, cca
import std.io;

i32 main() {
  io.println("Hi, I'm C^"); // doesn't required explicity return 0; in main. still can do
}
```
