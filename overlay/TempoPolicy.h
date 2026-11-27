#pragma once
#include "SkellayEngine.h"
namespace skellay {
// Call on the control thread with a validated configuration. Modern uses its
// native in-place tempo update; free-running banks need no tempo rebuild.
inline bool requiresTempoRebuild(const Configuration& c) noexcept {
 if(c.model==Model::classic)return true;
 if(c.model!=Model::bank || c.bytes.size()!=792)return false;
 for(unsigned i=0;i<8;++i){const auto o=536+32*i;if(c.bytes[o+29] && c.bytes[o+31])return true;}
 return false;
}
}
