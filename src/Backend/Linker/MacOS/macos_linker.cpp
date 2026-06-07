// macOS linker stub. Implementation plan: invoke `ld` via the
// Xcode toolchain wrapper, or call the Mach-O linker APIs directly
// for a tighter build cycle. Return false until the v0.2 native
// backend lands.
#include "macos_linker.hpp"
namespace caret::backend::MacOS {
bool link_macho(const std::string& object_path, const std::string& output_path) {
    (void)object_path; (void)output_path;
    return false;
}
}
