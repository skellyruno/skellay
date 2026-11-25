#include "EngineExchange.h"
#include <thread>
#include <cassert>
#include <cstdlib>
#include <cmath>
#include <cstdio>
using namespace skellay;
#define CHECK(x) do { if(!(x)) std::abort(); } while(false)
struct Gain:Engine {float g;std::atomic<int>& count;Gain(float v,std::atomic<int>& c):g(v),count(c){++count;}~Gain(){--count;}bool process(float*l,float*r,int n)noexcept override{for(int i=0;i<n;++i){l[i]*=g;r[i]*=g;}return true;}};
int main(){std::atomic<int> live{0};EngineExchange x; x.initialise(std::make_unique<Gain>(1,live),64,64);x.publish(std::make_unique<Gain>(0,live));float l[64],r[64];std::fill_n(l,64,1);std::fill_n(r,64,1);CHECK(x.process(l,r,64));CHECK(l[0]==1&&std::abs(l[63]-1.f/64)<1e-6);CHECK(x.process(l,r,64));x.collect();CHECK(live==1);std::thread producer([&]{for(int i=0;i<10000;++i)x.publish(std::make_unique<Gain>(float(i%2),live));});for(int k=0;k<10000;++k){std::fill_n(l,64,1);std::fill_n(r,64,1);CHECK(x.process(l,r,64));for(float v:l)CHECK(std::isfinite(v)&&v>=0&&v<=1);}producer.join();x.reset();CHECK(live==0);std::puts("Crossfade, concurrent publish/consume and reclamation passed");}
