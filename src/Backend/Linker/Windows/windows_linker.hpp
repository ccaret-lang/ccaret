// Windows PE linker stub. Return value is false (unsupported) until
// the v0.2 native backend lands; the real implementation will call
// `link.exe` (or the LLVM `lld-link` drop-in) with the right flag
// set for the C^ calling convention.
#pragma once
#include <string>
namespace caret::backend::Windows {
bool link_pe(const std::string& object_path, const std::string& output_path);
}
