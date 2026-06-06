// AST translation unit.
//
// Intentionally empty: every AST node is either a plain aggregate or an
// inline helper defined in ast.hpp. We keep the .cpp around so the build
// system can list a translation unit per directory and so we have a
// landing pad for future non-trivial helpers (deep clone, visitor
// dispatch, debug-dump, etc.) without forcing a header-only rebuild.
#include "ast.hpp"
