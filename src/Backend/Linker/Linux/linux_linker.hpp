// this is the caretc a compiler for the C^
#pragma once
#include <string>
namespace caret::backend::Linux {
bool link_elf(const std::string& object_path, const std::string& output_path);
}
