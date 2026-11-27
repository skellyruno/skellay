#pragma once
#include "Delay.h"
#include <cstdint>
#include <vector>
#include <optional>
namespace skelly::delay_research {
struct RawState { int model=0;std::vector<int> controls; };
inline bool valid(const RawState& s) noexcept {
 if(s.model==2){if(s.controls.size()!=8)return false;for(unsigned i=0;i<8;++i)if(s.controls[i]<SKELLdelay::delay2Min[i]||s.controls[i]>SKELLdelay::delay2Max[i])return false;return true;}
 if(s.model==3){if(s.controls.size()!=26)return false;for(unsigned i=0;i<26;++i)if(s.controls[i]<SKELLdelay::delay3Min[i]||s.controls[i]>SKELLdelay::delay3Max[i])return false;return true;}return false;
}
inline std::int64_t read32(const unsigned char* p) noexcept {std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::uint32_t(p[i])<<(8*i);return v<=0x7fffffffu?std::int64_t(v):std::int64_t(v)-0x100000000ll;}
inline std::optional<RawState> decode(const std::vector<unsigned char>& b,int model){
 if((model==2&&b.size()!=32)||(model==3&&(b.size()!=108||read32(b.data())!=1))||(model!=2&&model!=3))return std::nullopt;
 RawState s{model,{}};for(std::size_t i=model==3?4:0;i<b.size();i+=4)s.controls.push_back(static_cast<int>(read32(b.data()+i)));if(!valid(s))return std::nullopt;return s;
}
inline std::optional<std::vector<unsigned char>> encode(const RawState& s){
 if(!valid(s))return std::nullopt;
 std::vector<unsigned char> out;auto add=[&](int x){auto v=static_cast<std::uint32_t>(x);for(unsigned i=0;i<4;++i)out.push_back(static_cast<unsigned char>(v>>(8*i)));};if(s.model==3)add(1);for(int x:s.controls)add(x);return out;
}
}
