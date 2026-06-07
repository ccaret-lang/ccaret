// Linux ELF linker stub.
//
// The v0.2 native pipeline will replace the system-cc link step
// with a direct `ld`-equivalent invocation; the implementation will
// live here. The current stub returns false so a calling driver
// (if ever wired in) sees a clear "not implemented" signal.
#pragma once
#include <string>
namespace caret::backend::Linux {
bool link_elf(const std::string& object_path, const std::string& output_path);
}
