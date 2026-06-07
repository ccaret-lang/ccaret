// aarch64 target metadata. See x86_64.cpp for the parallel file.
#include "aarch64.hpp"
namespace caret::backend::aarch64 {
// aarch64-apple-darwin is the default for v0.1.0 because that's the
// most common dev machine in our contributor base. The Linux-arm64
// triple will replace this on non-Darwin builds once the native
// backend exists; until then the value is informational.
const char* triple() { return "aarch64-apple-darwin"; }
}
