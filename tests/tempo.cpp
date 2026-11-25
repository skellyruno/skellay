#include "TempoPolicy.h"
#include "EngineExchange.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <atomic>
using namespace skellay;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(false)
static std::atomic<unsigned> allocations{0};
void* operator new(std::size_t n){++allocations;if(auto p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
static void put(Configuration& c,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)c.bytes[o+i]=static_cast<std::uint8_t>(v>>(8*i));}
int main(){
 CHECK(!requiresTempoRebuild(defaults(Model::classic))); CHECK(!requiresTempoRebuild(defaults(Model::modern)));
 auto bank=defaults(Model::bank);for(unsigned i=0;i<8;++i)bank.bytes[536+32*i+29]=0;CHECK(!requiresTempoRebuild(bank));bank.bytes[536+29]=1;bank.bytes[536+31]=0;CHECK(!requiresTempoRebuild(bank));bank.bytes[536+31]=1;CHECK(!requiresTempoRebuild(bank));
 for(int rate:{32000,44100,48000,96000,192000})for(int keep:{0,1})for(int sync:{0,1}){
  auto c=defaults(Model::modern);put(c,12,sync);put(c,16,keep);put(c,20,192);put(c,60,4500);put(c,104,0);CHECK(validate(c));
  EngineExchange x,y;x.initialise(prepare(c,rate,120,128),128,64);y.initialise(prepare(c,rate,120,128),128,64);
  float l[128],r[128],a[128],b[128];double energy=0,difference=0;auto count=allocations.load();
  for(int at=0;at<rate*2;at+=128){std::fill_n(l,128,0);std::fill_n(r,128,0);if(!at){l[0]=.25f;r[0]=-.125f;}std::copy_n(l,128,a);std::copy_n(r,128,b);
   double bpm=at>=rate/4?90.:120.;CHECK(x.process(l,r,128,bpm));CHECK(y.process(a,b,128,120));for(int i=0;i<128;++i){CHECK(std::isfinite(l[i])&&std::isfinite(r[i]));if(at>=rate/4){energy+=std::abs(l[i])+std::abs(r[i]);difference+=std::abs(l[i]-a[i])+std::abs(r[i]-b[i]);}}}
  CHECK(allocations.load()==count);if(sync){CHECK(energy>1e-5);CHECK(difference>1e-5);}else CHECK(difference==0);
  std::printf("rate=%d keep=%d sync=%d post-change_abs_sum=%.8g difference=%.8g allocations=0\n",rate,keep,sync,energy,difference);
 }
}
