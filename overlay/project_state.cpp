#include "ProjectState.h"
#include <cstdio>
#include <cstdlib>
using namespace skellay;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(false)
static void put(std::vector<std::uint8_t>& b,std::uint32_t x){for(unsigned i=0;i<4;++i)b.push_back(static_cast<std::uint8_t>(x>>(8*i)));}
int main(){
 ProjectState s; s.engines[0].bytes[0]=1; s.engines[1].bytes[4]=1; s.engines[2].bytes[4]=1; CHECK(validProject(s));
 for(auto m:{Model::classic,Model::modern,Model::bank}){s.active=m;const auto bytes=encodeProject(s);CHECK(bytes.size()==956);auto a=decodeProject(bytes.data(),bytes.size(),ProjectState{});CHECK(a&&encodeProject(*a)==bytes);for(std::size_t i=0;i<bytes.size();++i)CHECK(!decodeProject(bytes.data(),i,s));auto extra=bytes;extra.push_back(0);CHECK(!decodeProject(extra.data(),extra.size(),s));auto bad=bytes;bad[12]=255;CHECK(!decodeProject(bad.data(),bad.size(),s));
 std::vector<std::uint8_t> legacy;put(legacy,0x59414c53);put(legacy,1);put(legacy,static_cast<unsigned>(m));auto& c=s.engines[static_cast<unsigned>(m)-2];put(legacy,c.bytes.size());legacy.insert(legacy.end(),c.bytes.begin(),c.bytes.end());ProjectState prev;auto v1=decodeProject(legacy.data(),legacy.size(),prev);CHECK(v1&&v1->active==m);for(unsigned i=0;i<3;++i)CHECK(v1->engines[i].bytes==(i==static_cast<unsigned>(m)-2?c.bytes:prev.engines[i].bytes));}
 auto invalid=s;invalid.engines[0].model=Model::modern;CHECK(encodeProject(invalid).empty());invalid=s;invalid.active=static_cast<Model>(99);CHECK(encodeProject(invalid).empty());
 auto v15=s;auto& bank=v15.engines[2].bytes;bank[0]=15;for(unsigned i=0;i<8;++i){auto o=4+4*(12+16*i);bank[o]=144;bank[o+1]=bank[o+2]=bank[o+3]=0;}CHECK(validProject(v15));auto bytes=encodeProject(v15);auto restored=decodeProject(bytes.data(),bytes.size(),s);CHECK(restored&&restored->engines[2].bytes==bank);
 std::puts("Project state: three engine round trips, 2868 truncations, v1 migration, invalid lengths/models, v15 byte preservation passed");
}
