// this is the caretc a compiler for the C^
#include "windows_linker.hpp"
namespace caret::backend::Windows {
bool link_pe(const std::string& object_path, const std::string& output_path) {
    (void)object_path; (void)output_path;
    return false;
}
}
