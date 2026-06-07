// Linux linker stub. Returns false so the calling driver (when
// wired in) sees "not implemented" and falls back to the system cc.
//
// Implementation plan: invoke `/usr/bin/ld` directly with a minimal
// flag set, or open-code a tiny ELF section stitcher for the common
// case of a single object + libc. The latter avoids the fork/exec
// cost on hot iteration loops.
#include "linux_linker.hpp"
namespace caret::backend::Linux {
bool link_elf(const std::string& object_path, const std::string& output_path) {
    (void)object_path; (void)output_path;
    return false;
}
}
