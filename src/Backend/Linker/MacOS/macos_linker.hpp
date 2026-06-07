// macOS Mach-O linker stub.
//
// Same shape as the Linux ELF stub: the implementation lands in
// v0.2 alongside the native code generator. The return value is
// false (unsupported) until then.
#pragma once
#include <string>
namespace caret::backend::MacOS {
bool link_macho(const std::string& object_path, const std::string& output_path);
}
