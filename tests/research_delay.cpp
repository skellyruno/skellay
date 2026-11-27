#include "../include/ResearchDelay.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <random>
using D=skellay::research::ResearchDelay;
int main(){
 for(double rate:{8000.,44100.,48000.,96000.,192000.,384000.}){
  D d;assert(d.prepare(rate));assert(d.storageBytes()==(std::size_t(2*rate)+2)*sizeof(D::Frame));
  D::Controls p{100,.5f,1,1};const int n=int(rate*.1);
  for(int i=0;i<=3*n;i++){const auto y=d.process({i==0?1.f:0,0},p);assert(std::abs(y.l-(i==n?1.f:i==3*n?.25f:0))<1e-6);assert(std::abs(y.r-(i==2*n?.5f:0))<1e-6);}
  d.reset();p={1000.*(10.5/rate),0,1,0};for(int i=0;i<25;++i){auto y=d.process({i==0?1.f:0.f,0},p);assert(std::abs(y.l-((i==10||i==11)?.5f:0.f))<1e-6&&y.r==0);}
  d.reset();p={100,0,0,0};for(int i=0;i<1000;++i){auto y=d.process({.25f,-.5f},p);assert(y.l==.25f&&y.r==-.5f);}
  // Compare both fixed heads against an independent linear-ramp oracle.
  d.reset();p={100,0,1,0};const int switchAt=int(rate*.3), fade=std::max(2,int(std::round(rate*.025)));
  for(int i=0;i<switchAt+fade+4;++i){if(i==switchAt)p.milliseconds=200;const auto y=d.process({float(i)/float(rate),0},p);const float old=float(std::max(0,i-n))/float(rate),next=float(std::max(0,i-2*n))/float(rate);float t=std::clamp(float(i-switchAt)/float(fade-1),0.f,1.f);float a=t*t*(3-2*t);assert(std::abs(y.l-(old+a*(next-old)))<1e-6);}
  d.reset();p={100,.8f,1,.2f};for(int i=0;i<int(rate*.5);++i){auto y=d.process({.2f,-.1f},p);assert(std::isfinite(y.l)&&std::isfinite(y.r));}
  p.milliseconds=200;float peak=0;for(int i=0;i<int(rate*.1);++i){auto y=d.process({},p);peak=std::max(peak,std::abs(y.l));}assert(peak>.05f);
  std::mt19937 rng(7);std::uniform_real_distribution<float> u(-1,1);for(int i=0;i<100000;++i){p.milliseconds=i%137==0?u(rng)*4000:p.milliseconds;p.feedback=1;p.wet=1;p.cross=1;auto y=d.process({u(rng),u(rng)},p);assert(std::isfinite(y.l)&&std::isfinite(y.r)&&std::abs(y.l)<=32&&std::abs(y.r)<=32);}
  auto y=d.process({std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}, {std::numeric_limits<double>::quiet_NaN(),0,1,0});assert(std::isfinite(y.l)&&std::isfinite(y.r));
  std::printf("%.0f Hz: impulse, feedback, dry identity, tail, hostile controls, storage %zu bytes passed\n",rate,d.storageBytes());
 }
 D d;assert(!d.prepare(0));assert(!d.prepare(48000,9));auto y=d.process({.5f,.4f},{});assert(y.l==.5f&&y.r==.4f);
}
