// this is the caretc a compiler for the C^
#include "macos_linker.hpp"
namespace caret::backend::MacOS {
bool link_macho(const std::string& object_path, const std::string& output_path) {
    (void)object_path; (void)output_path;
    return false;
}
}
