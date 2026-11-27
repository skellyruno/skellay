#include "../include/ResearchDelay.h"
#include <cassert>
#include <cstdio>
#include <random>
using D=skellay::research::ResearchDelay;
int main(){
    std::mt19937 rng(61);std::uniform_real_distribution<float> u(-1,1);
    for(double fs:{8000.,44100.,48000.,96000.,192000.,384000.})for(bool cubic:{false,true}){
        D a,b,dry;assert(a.prepare(fs,2,cubic));assert(b.prepare(fs,2,cubic,-3));assert(dry.prepare(fs,2,cubic,-3));
        const float g=std::pow(10.f,-3.f/20.f);D::Controls p{120,.8f,1,.3f};
        for(int i=0;i<100000;++i){if(i%8191==0){a.reset();b.reset();dry.reset();}if(i%127==0){p.milliseconds=1+400*std::abs(u(rng));p.feedback=.95f*std::abs(u(rng));p.cross=std::abs(u(rng));}D::Frame x{u(rng),u(rng)};auto y=a.process(x,p),z=b.process(x,p);assert(std::isfinite(z.l)&&std::isfinite(z.r));assert(z.l==g*y.l&&z.r==g*y.r);auto pd=p;pd.wet=0;auto w=dry.process(x,pd);assert(w.l==x.l&&w.r==x.r);}
    }
    std::puts("Wet trim: exact full-wet scaling and dry identity under changing feedback/time/cross controls passed");
}
