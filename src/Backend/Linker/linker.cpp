// Linker facade stub.
//
// The driver currently invokes the system C compiler for the link
// step, so this translation unit is empty. The platform-specific
// helpers (Linux/MacOS/Windows) below already exist and will move
// into the call path once the native code generator lands.
#include "linker.hpp"
