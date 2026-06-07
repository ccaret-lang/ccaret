// x86_64 target metadata.
//
// Today this is just the canonical triple. Once the native backend
// lands, this translation unit grows: register definitions, calling
// convention, ELF section handling, the lot.
#include "target.hpp"
namespace caret::backend::x86_64 {
// x86_64-pc-linux-gnu is the canonical triple for the Linux x86_64
// build we ship by default. Other triples (darwin, mingw) will be
// picked by the build system on their respective platforms.
const char* triple() { return "x86_64-pc-linux-gnu"; }
}
