#include "../include/ResearchAllpassDelay.h"
#include <cassert>
#include <complex>
#include <cstdio>
#include <random>
using D=skellay::research::ResearchAllpassDelay;
int main(){
    constexpr int N=512;constexpr double pi=3.14159265358979323846;unsigned checks=0;double maxError=0;
    for(double fs:{44100.,48000.,96000.})for(double f:{0.,.25,.5,.75})for(double feedback:{.45,.9})for(double cross:{0.,.35,1.})for(double damping:{0.,6000.})for(double width:{0.,1.,2.}){
        D d;assert(d.prepare(fs,N+f,feedback,cross,damping,width));std::vector<D::Frame> y;
        for(int i=0;i<5*N;++i)y.push_back(d.process({i==0?1.:0.,0}));
        for(int e=1;e<=4;++e)for(double hz:{0.,1000.,8000.,12000.,20000.}){
            const auto z=std::polar(1.,-2*pi*hz/fs);const double a=(1-f)/(1+f);const auto A=f==0?std::complex<double>(1,0):(a+z)/(1.+a*z);const double p=damping==0?0:std::exp(-2*pi*damping/fs);const auto C=(1-p)/(1.-p*z);std::complex<double> mid{},side{};
            for(int j=0;j<N;++j){const auto ph=std::polar(1.,-2*pi*hz*j/fs);mid+=(y[e*N+j].l+y[e*N+j].r)*ph;side+=(y[e*N+j].l-y[e*N+j].r)*ph;}
            const auto m=A*std::pow(feedback*A*C,e-1),s=width*A*std::pow(feedback*(1-2*cross)*A*C,e-1);maxError=std::max({maxError,std::abs(mid-m),std::abs(side-s)});assert(maxError<1e-8);checks+=2;
        }
    }
    std::mt19937 rng(44);std::uniform_real_distribution<double> u(-1,1);
    for(double fs:{8000.,44100.,48000.,96000.,192000.,384000.}){
        D a,b;assert(a.prepare(fs,64.5,.95,1,0,1));assert(b.prepare(fs,64.5,.95,1,0,0));
        for(int i=0;i<50000;++i){if(i%8191==0){a.reset();b.reset();}D::Frame x{u(rng),u(rng)};auto y=a.process(x),z=b.process(x);assert(std::isfinite(y.l)&&std::isfinite(y.r));assert(std::abs((y.l+y.r)-(z.l+z.r))<1e-12);}
        D dry;assert(dry.prepare(fs,64.5,.95,.35,6000,2,0));for(int i=0;i<3000;++i){D::Frame x{u(rng),u(rng)};auto y=dry.process(x);assert(y.l==x.l&&y.r==x.r);}
        assert(!dry.prepare(fs,64.001,.5,0,0,1));
    }
    std::printf("%u complex allpass echo checks, max error %.12g; six-rate width independence and dry stress passed\n",checks,maxError);
}
