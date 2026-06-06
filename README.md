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
fn main(args: str[]) -> i32 {
    print("{} arguments received from the command line", args.len);
    return 0;
}
```
