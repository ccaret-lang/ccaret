// this is the caretc a compiler for the C^
#pragma once
#include <string>
namespace caret::backend::Windows {
bool link_pe(const std::string& object_path, const std::string& output_path);
}
