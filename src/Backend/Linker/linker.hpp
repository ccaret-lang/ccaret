// Linker facade.
//
// The native linker is not yet built; today main.cpp shells out to
// the system C compiler and lets `cc` perform the link. Once the
// v0.2 native code generator lands, this header will own the
// platform-specific link entry points (see linux/macos/windows
// subdirectories) and the driver will call into them.
#pragma once
#include <string>
namespace caret::backend {
// link_object: turn the object file at `object_path` into a runnable
// executable at `output_path`. Returns true on success. Reserved for
// the v0.2 native pipeline; today the entry point is unwired.
bool link_object(const std::string& object_path, const std::string& output_path);
}
