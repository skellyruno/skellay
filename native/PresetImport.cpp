#include "PresetImport.h"
#include <algorithm>
#include <array>
#include <limits>
namespace skellay {
namespace {
using Bytes=std::vector<std::uint8_t>;
constexpr std::size_t limit=64u*1024u*1024u;
std::uint32_t u32(const Bytes& d,std::size_t p){std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::uint32_t(d[p+i])<<(i*8);return v;}
int location(const Bytes& d,std::size_t p){auto v=u32(d,p);return v<=std::uint32_t(std::numeric_limits<int>::max())?int(v):-1;}
Bytes utf16(const char* s){Bytes b;for(;*s;++s){b.push_back(std::uint8_t(*s));b.push_back(0);}b.insert(b.end(),{0,0});return b;}
bool at(const Bytes& d,std::size_t p,const Bytes& s){return p<=d.size()&&s.size()<=d.size()-p&&std::equal(s.begin(),s.end(),d.begin()+p);}
bool readLength(const Bytes& d,std::size_t& p,std::uint32_t& n){n=0;for(unsigned i=0;i<5;++i){if(p>=d.size())return false;auto b=d[p++];if(i==4&&(b&0xf0))return false;n|=std::uint32_t(b&127)<<(7*i);if(!(b&128))return i==0||(b&127)!=0;}return false;}
}
std::optional<Configuration> importRaw(Model m,const Bytes& data){if(data.size()!=32&&data.size()!=108&&data.size()!=792)return {};Configuration c{m,data};if(!validate(c))return {};return c;}
std::optional<Configuration> importRaw(const Bytes& data){switch(data.size()){case 32:return importRaw(Model::classic,data);case 108:return importRaw(Model::modern,data);case 792:return importRaw(Model::bank,data);default:return {};}}
ImportResult scanFlPreset(const Bytes& d){
 ImportResult r;
 if(d.size()<22||d.size()>limit||!at(d,0,{'F','L','h','d'})||u32(d,4)!=6||!at(d,14,{'F','L','d','t'})||u32(d,18)!=d.size()-22){r.error="Unsupported or malformed FL container";return r;}
 const Bytes marker={0x9b,0,0,0,0,0x80};
 struct Spec {const char* label; Model model; unsigned size;};
 const std::array<Spec,3> specs={{{"Fruity Delay 2",Model::classic,32},{"Fruity Delay 3",Model::modern,108},{"Fruity Delay Bank",Model::bank,792}}};
 for(const auto& old:{utf16("Fruity Delay"),utf16("Fruity Delay 1")})for(std::size_t p=22;p<d.size();++p)if(at(d,p,old)&&at(d,p+old.size(),{0xd4,0x34}))r.legacyDelaySeen=true;
 for(const auto& spec:specs){const auto name=utf16(spec.label);for(std::size_t p=22;p<d.size();++p){if(!at(d,p,name))continue;auto h=p+name.size();if(!at(d,h,{0xd4,0x34})||d.size()-h<10)continue;
 // Strict initial support for the fixed wrapper offset observed in research.
 // Do not scan forward into another plugin if this wrapper differs.
 const auto k=h+58;
 if(!at(d,k,marker)||d.size()-k<14||d[k+10]!=0x29||d[k+11]!=0||d[k+12]!=0xd5){++r.rejected;continue;}
 auto offset=k+13;std::uint32_t n=0;
 if(!readLength(d,offset,n)||n!=spec.size||n>d.size()-offset){++r.rejected;continue;}
 Bytes raw(d.begin()+offset,d.begin()+offset+spec.size);auto c=importRaw(spec.model,raw);if(!c){++r.rejected;continue;}
 if(r.entries.size()>=1024){r.entries.clear();r.error="Too many delay instances";return r;}
 r.entries.push_back({std::move(*c),location(d,h+2),location(d,h+6),offset});p=offset+spec.size-1;
 }}
 std::sort(r.entries.begin(),r.entries.end(),[](const auto& a,const auto& b){return a.offset<b.offset;});
 for(std::size_t i=1;i<r.entries.size();++i)if(r.entries[i].offset<r.entries[i-1].offset+r.entries[i-1].state.bytes.size()){r.entries.clear();r.error="Overlapping delay records";return r;}
 if(r.entries.empty())r.error=r.legacyDelaySeen?"Original Fruity Delay is not supported":"No supported delay record found";
 return r;
}
}
