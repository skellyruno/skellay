#include "PresetImport.h"
#include "ProjectState.h"
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <cmath>
#define CHECK(b) do{if(!(b)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#b);std::abort();}}while(false)
using namespace skellay;
using B=std::vector<std::uint8_t>;
static void append(B& d,const B& x){d.insert(d.end(),x.begin(),x.end());}
static void put(B& d,unsigned n){for(int i=0;i<4;++i)d.push_back((n>>(8*i))&255);}
static B wrapper(const Configuration& c,int slot){B d;const char* name=c.model==Model::classic?"Fruity Delay 2":c.model==Model::modern?"Fruity Delay 3":"Fruity Delay Bank";for(;*name;++name)append(d,{std::uint8_t(*name),0});append(d,{0,0,0xd4,0x34});put(d,5);put(d,slot);d.resize(d.size()+48,0);append(d,{0x9b,0,0,0,0,0x80,0,0,0,0,0x29,0,0xd5});unsigned n=c.bytes.size();while(n>=128){d.push_back((n&127)|128);n>>=7;}d.push_back(n);append(d,c.bytes);return d;}
static B file(const B& payload){B d={'F','L','h','d',6,0,0,0,0,0,1,0,96,0,'F','L','d','t'};put(d,payload.size());append(d,payload);return d;}
static void render(const Configuration& c){auto e=prepare(c,48000,130,256);CHECK(e);float l[256]={.1f},r[256]={-.05f};for(int i=0;i<128;++i)CHECK(e->process(l,r,256));}
int main(int argc,char** argv){B payload;int i=0;for(auto m:{Model::classic,Model::modern,Model::bank}){auto c=defaults(m);CHECK(importRaw(m,c.bytes)->bytes==c.bytes);CHECK(importRaw(c.bytes)->model==m);append(payload,wrapper(c,++i));}auto d=file(payload);auto a=scanFlPreset(d);CHECK(a.entries.size()==3);for(unsigned j=0;j<3;++j){auto& e=a.entries[j];CHECK(e.slot==int(j+1));CHECK(e.state.bytes==defaults(e.state.model).bytes);render(e.state);}for(std::size_t n=0;n<d.size();++n){B shortd(d.begin(),d.begin()+n);CHECK(scanFlPreset(shortd).entries.empty());}auto wrong=d;wrong[0]='X';CHECK(scanFlPreset(wrong).entries.empty());CHECK(!importRaw(Model::modern,defaults(Model::classic).bytes));auto invalid=defaults(Model::modern);invalid.bytes[0]=2;CHECK(!importRaw(invalid.model,invalid.bytes));CHECK(scanFlPreset(file(wrapper(invalid,1))).entries.empty());
 auto wrongLength=wrapper(defaults(Model::classic),1);wrongLength[wrongLength.size()-33]=31;append(wrongLength,wrapper(defaults(Model::classic),2));auto second=scanFlPreset(file(wrongLength));CHECK(second.entries.size()==1&&second.entries[0].slot==2&&second.rejected==1);
 B old;for(char ch:std::string("Fruity Delay"))append(old,{std::uint8_t(ch),0});append(old,{0,0,0xd4,0x34});CHECK(scanFlPreset(file(old)).legacyDelaySeen);
 unsigned rng=1234567;for(int k=0;k<2000;++k){auto fuzz=d;rng=rng*1664525u+1013904223u;auto pos=22+rng%(fuzz.size()-22);rng=rng*1664525u+1013904223u;fuzz[pos]^=std::uint8_t(rng>>24);auto result=scanFlPreset(fuzz);for(const auto& e:result.entries){CHECK(validate(e.state));CHECK(e.offset+e.state.bytes.size()<=fuzz.size());}}
 int recovered=0;for(int k=1;k+1<argc;k+=2){auto model=static_cast<Model>(std::atoi(argv[k]));std::ifstream f(argv[k+1],std::ios::binary);CHECK(f);B raw((std::istreambuf_iterator<char>(f)),{});auto c=importRaw(model,raw);CHECK(c);CHECK(importRaw(raw)->model==model);auto result=scanFlPreset(file(wrapper(*c,2)));CHECK(result.entries.size()==1);CHECK(result.entries[0].state.bytes==raw);ProjectState project;project.active=model;project.engines[static_cast<unsigned>(model)-2]=*c;auto saved=encodeProject(project);auto restored=decodeProject(saved.data(),saved.size(),ProjectState{});CHECK(restored&&restored->active==model&&restored->engines[static_cast<unsigned>(model)-2].bytes==raw);render(restored->engines[static_cast<unsigned>(model)-2]);++recovered;}
 std::printf("Preset import: 3 synthetic wrappers, %zu truncated inputs, wrong model/version, legacy detection; %d recovered raw records imported and rendered.\n",d.size(),recovered);}
