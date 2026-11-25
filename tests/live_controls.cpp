#include "SkellayEngine.h"
#include "FruityDelayBank.h"
#include "Delay.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <chrono>
using namespace skellay;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(false)
static std::atomic<unsigned> allocs{0};
void* operator new(std::size_t n){++allocs;if(auto p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept{std::free(p);}void operator delete[](void* p) noexcept{std::free(p);}void operator delete(void* p,std::size_t) noexcept{std::free(p);}void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
void put(Configuration& s,int pos,int v){for(int i=0;i<4;++i)s.bytes[pos+i]=std::uint8_t(std::uint32_t(v)>>(i*8));}
Configuration make(Model m,int os=0,bool sync=true,bool pattern=false){auto s=defaults(m);if(m==Model::classic){put(s,8,0);}else if(m==Model::bank){auto st=skelly::fldbank::defaultState();st.p[0]=0;st.p[132]=os;st.bank[0].sync=sync;st.bank[0].tapBeforeFeedback=true;st.bank[0].steps=pattern?4:0;st.bank[0].amount=64;s.bytes=skelly::fldbank::serialise(st);}return s;}
int main(){
 for(auto m:{Model::classic,Model::bank})for(int sr:{32000,44100,48000,96000,192000})for(int block:{32,257,4096}){
  auto s=make(m);auto e=prepare(s,sr,120,block);CHECK(e);std::vector<float> l(sr*2),r(sr*2);l[0]=.25f;r[0]=-.125f;auto n=allocs.load();
  for(int at=0;at<2*sr;at+=block){if(at>=sr/8)e->updateTempo(60);CHECK(e->process(l.data()+at,r.data()+at,std::min(block,2*sr-at)));}
  CHECK(allocs==n);int first=int(std::nearbyint(sr*.375)),second=first+int(std::nearbyint(sr*.75));CHECK(std::abs(l[first])>.05);CHECK(std::abs(l[second])>.001);CHECK(std::abs(l[first+first])<1e-6);std::printf("tail model=%d sr=%d block=%d first=%d %.7g second=%d %.7g allocations=0\n",int(m),sr,block,first,l[first],second,l[second]);
 }
 for(auto m:{Model::classic,Model::modern,Model::bank}){
  auto s=make(m);auto x=prepare(s,48000,120,128),y=prepare(s,48000,120,128);CHECK(x&&y);auto c=liveControls(s);std::vector<float> a(96000),b(96000),l(96000),r(96000);a[0]=l[0]=.2f;b[0]=r[0]=.1f;auto n=allocs.load();
  int control=m==Model::classic?3:m==Model::modern?14:10;
  for(int at=0;at<96000;at+=128){if(at>=6000){c.values[control]=liveControlMin(m,control);CHECK(x->updateControls(c));}CHECK(x->process(l.data()+at,r.data()+at,128));CHECK(y->process(a.data()+at,b.data()+at,128));}CHECK(allocs==n);
  double diff=0,tail=0;for(int i=0;i<96000;++i){diff+=std::abs(l[i]-a[i]);if(i>6000)tail+=std::abs(l[i]);}CHECK(diff>1e-5);CHECK(tail>1e-5);c.values[control]=liveControlMax(m,control)+1;CHECK(!x->updateControls(c));std::printf("automation model=%d tail=%.9g difference=%.9g allocations=0\n",int(m),tail,diff);
 }
 for(int os:{0,1,2})for(bool sync:{false,true}){
  auto s=make(Model::bank,os,sync,true);auto st=skelly::fldbank::parse(s.bytes);for(int i=0;i<8;++i){st->bank[i].enabled=true;st->bank[i].sync=sync;st->bank[i].steps=4;st->bank[i].amount=32;st->bank[i].tapBeforeFeedback=true;st->p[16*i+12]=1440+120*i;st->p[16*i+10]=32;st->p[16*i+18]=32;}s.bytes=skelly::fldbank::serialise(*st);auto x=prepare(s,48000,120,128);CHECK(x);auto c=liveControls(s);float l[128],r[128];double energy=0;auto n=allocs.load();auto start=std::chrono::steady_clock::now();
  for(int at=0;at<96000;at+=128){for(int i=0;i<128;++i){l[i]=at==0&&i==0?.2f:0;r[i]=0;}if(at>=6000){x->updateTempo(70.+(at/128)%100);c.values[12]=1000+(at/128)%1000;c.values[13]=((at/128)%200)-100;c.values[10]=32+(at/128)%16;CHECK(x->updateControls(c));}CHECK(x->process(l,r,128));for(int i=0;i<128;++i)if(at>6000)energy+=std::abs(l[i])+std::abs(r[i]);}CHECK(allocs==n);CHECK(energy>1e-6);double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();std::printf("bank stress os=%d sync=%d 8 banks energy=%.8g cpu_pct=%.3f allocations=0\n",1<<os,sync,energy,ms/20);
 }
 // Exercise every exposed raw integer with a changed, valid value and a render.
 for(auto m:{Model::classic,Model::modern,Model::bank}){
  auto s=make(m);if(m==Model::bank){auto st=skelly::fldbank::parse(s.bytes);for(int i=0;i<8;++i){st->bank[i].enabled=true;st->bank[i].filterType[0]=1;st->bank[i].filterType[1]=1;st->p[16*i+18]=16;st->p[16*i+10]=16;}s.bytes=skelly::fldbank::serialise(*st);}
  auto e=prepare(s,48000,120,128);CHECK(e);auto base=liveControls(s);auto count=allocs.load();float l[128],r[128];
  for(int k=0;k<liveControlCount(m);++k){auto c=base;const auto lo=liveControlMin(m,k),hi=liveControlMax(m,k);auto v=c.values[k];c.values[k]=v==hi?std::max(lo,v-std::max(1,(hi-lo)/16)):std::min(hi,v+std::max(1,(hi-lo)/16));CHECK(c.values[k]!=v);CHECK(e->updateControls(c));for(int j=0;j<128;++j){l[j]=.005f*std::sin(.03f*j);r[j]=.003f*std::cos(.07f*j);}CHECK(e->process(l,r,128));CHECK(e->updateControls(base));CHECK(e->process(l,r,128));}
  CHECK(allocs==count);std::printf("all raw control smoke model=%d changed=%d allocations=0\n",int(m),liveControlCount(m));
 }

}
