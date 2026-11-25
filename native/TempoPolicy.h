#pragma once
#include "SkellayEngine.h"
namespace skellay {
// Call on the control thread with a validated configuration. Modern uses its
// native in-place tempo update; free-running banks need no tempo rebuild.
inline bool requiresTempoRebuild(const Configuration& c) noexcept {
 (void)c;return false;
}
}
