// Target description for the aarch64 (Apple Silicon, Linux arm64)
// backend. Same shape as the x86_64 header; the only thing exported
// today is the canonical triple. The implementation file is part of
// the build so the directory is exercised on every CI run, catching
// stale includes before they bite.
#pragma once
namespace caret::backend::aarch64 {
// Returns the canonical triple for the aarch64 target. The
// darwin-suffixed value is the safe default; a Linux-arm64 build
// will swap in the GNU suffix when the v0.2 native backend lands.
const char* triple();
}
