// Windows linker stub. Implementation plan: shell out to `link.exe`
// (MSVC) or `lld-link` (LLVM drop-in) with a minimal flag set. The
// stub returns false so the driver (when wired in) sees "not
// implemented" and falls back to the system C compiler.
#include "windows_linker.hpp"
namespace caret::backend::Windows {
bool link_pe(const std::string& object_path, const std::string& output_path) {
    (void)object_path; (void)output_path;
    return false;
}
}
