#include "../include/ResearchDelay.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <random>
using D=skellay::research::ResearchDelay;
int main(){
    std::mt19937 rng(41);std::uniform_real_distribution<float> u(-1,1);
    for(double fs:{8000.,44100.,48000.,96000.,192000.,384000.}){
        D a,b;assert(a.prepare(fs));assert(b.prepare(fs,2,true));
        D::Controls p{1000.*32/fs,.7f,1,.3f};
        for(int i=0;i<20000;++i){D::Frame x{u(rng),u(rng)};auto y=a.process(x,p),z=b.process(x,p);assert(y.l==z.l&&y.r==z.r);}
        a.reset();b.reset();p={1000.*(32.5/fs),.9f,0,.7f};
        for(int i=0;i<20000;++i){D::Frame x{u(rng),u(rng)};auto y=a.process(x,p),z=b.process(x,p);assert(y.l==x.l&&y.r==x.r&&z.l==x.l&&z.r==x.r);}
        // After an integer target settles, both histories must be identical.
        p={1000.*64/fs,.9f,1,.7f};int fade=int(std::round(fs*.025));
        for(int i=0;i<fade+20000;++i){D::Frame x{u(rng),u(rng)};auto y=a.process(x,p),z=b.process(x,p);assert(std::isfinite(z.l)&&std::isfinite(z.r));if(i>=fade)assert(y.l==z.l&&y.r==z.r);}
        a.reset();b.reset();p={1000.*(32.5/fs),0,1,0};
        for(int i=0;i<100;++i){auto y=a.process({i==0?1.f:0.f,0},p),z=b.process({i==0?1.f:0.f,0},p);float want=i==31||i==34?-.0625f:i==32||i==33?.5625f:0;assert(std::abs(z.l-want)<1e-6);assert(y.r==0&&z.r==0);}
        b.reset();p={1000.*(1.5/fs),0,1,0};for(int i=0;i<10;++i){auto z=b.process({i==0?1.f:0.f,0},p);assert(std::abs(z.l-(i==1||i==2?.5f:0.f))<1e-6);}
        std::printf("%.0f Hz: integer parity, fractional impulse, dry and shared feedback state passed\n",fs);
    }
}
