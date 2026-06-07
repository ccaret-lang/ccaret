// HIR lowering — stub.
//
// The HIR will own desugaring of `defer`, range-for, `?`-unwrap, and
// the other language-level sugar into explicit HIR primitives. We
// keep the .cpp around so the build system can target the directory
// today and so the implementation has a landing pad for the v0.2
// pipeline work.
#include "hir.hpp"
