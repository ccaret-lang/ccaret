// Target description for the x86_64 (System V / Windows) backend.
//
// The native backend isn't wired in yet; today the C backend
// generates portable C and we hand it to the system cc. Once the LIR
// lands, this header will be the entry point for the target's
// machine description: calling convention, register set, ELF/PE
// emission. For now we only export the canonical target triple so
// `--target=` on the command line has something real to report.
#pragma once
namespace caret::backend::x86_64 {
// Returns the canonical LLVM-style triple for the x86_64 target we
// generate for by default. The value is fixed for v0.1.0; a future
// per-platform switch will pick the right one at compile time.
const char* triple();
}
