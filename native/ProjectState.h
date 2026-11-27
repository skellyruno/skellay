#pragma once
#include "SkellayEngine.h"
#include <array>
#include <optional>
#include <stdexcept>
namespace skellay {
struct ProjectState {
 Model active = Model::classic;
 std::array<Configuration,3> engines{defaults(Model::classic), defaults(Model::modern), defaults(Model::bank)};
};
inline bool validModel(Model m) noexcept {return m==Model::classic||m==Model::modern||m==Model::bank;}
inline bool validProject(const ProjectState& s){
 if(!validModel(s.active))return false;
 for(unsigned i=0;i<3;++i)if(s.engines[i].model!=static_cast<Model>(i+2)||!validate(s.engines[i]))return false;
 return true;
}
inline std::vector<std::uint8_t> encodeProject(const ProjectState& s){
 if(!validProject(s))return {};
 std::vector<std::uint8_t> b; b.reserve(956);
 auto put=[&](std::uint32_t x){for(unsigned i=0;i<4;++i)b.push_back(static_cast<std::uint8_t>(x>>(8*i)));};
 put(0x59414c53);put(2);put(static_cast<unsigned>(s.active));
 for(const auto& c:s.engines){put(static_cast<unsigned>(c.bytes.size()));b.insert(b.end(),c.bytes.begin(),c.bytes.end());}
 return b;
}
// Decode entirely before changing active plugin state. A v1 preset replaces only its own engine.
inline std::optional<ProjectState> decodeProject(const void* data,std::size_t n,const ProjectState& previous){
 if(!data||n<16||n>2048)return {};
 const auto* b=static_cast<const std::uint8_t*>(data);std::size_t pos=0;
 auto get=[&]()->std::optional<std::uint32_t>{if(n-pos<4)return {};std::uint32_t x=0;for(unsigned i=0;i<4;++i)x|=std::uint32_t(b[pos++])<<(8*i);return x;};
 const auto magic=get(),version=get(),m=get();if(magic!=0x59414c53u||!version||(*version!=1&&*version!=2)||!m||*m<2||*m>4)return {};
 auto next=previous;next.active=static_cast<Model>(*m);
 auto read=[&](Model which){const auto len=get();const unsigned expected=which==Model::classic?32:which==Model::modern?108:792;if(!len||*len!=expected||*len>n-pos)return false;Configuration c{which,{b+pos,b+pos+*len}};pos+=*len;if(!validate(c))return false;next.engines[static_cast<unsigned>(which)-2]=std::move(c);return true;};
 if(*version==1){if(!read(next.active))return {};}else for(unsigned i=2;i<=4;++i)if(!read(static_cast<Model>(i)))return {};
 if(pos!=n||!validProject(next))return {};
 return next;
}
}
