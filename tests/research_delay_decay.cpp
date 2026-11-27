#include "../include/ResearchDelay.h"
#include <cassert>
#include <complex>
#include <cstdio>
using D=skellay::research::ResearchDelay;
int main(){
    constexpr double pi=3.14159265358979323846;
    constexpr int delay=512, echoes=8;
    unsigned checks=0;double maxError=0;
    for(double fs:{44100.,48000.,96000.})for(bool cubic:{false,true})for(float fraction:{0.f,.5f})for(float feedback:{.45f,.9f})for(float cross:{0.f,.35f,.5f,1.f}){
        D d;assert(d.prepare(fs,2,cubic));D::Controls p{1000*(delay+fraction)/fs,feedback,1,cross};
        std::vector<D::Frame> y;for(int i=0;i<=echoes*(delay+1)+4;++i)y.push_back(d.process({i==0?1.f:0.f,0},p));
        for(int e=1;e<=echoes;++e)for(double hz:{0.,1000.,8000.,12000.,16000.,20000.}){
            const double w=2*pi*hz/fs;const auto z=std::polar(1.,-w);std::complex<double> mid{},side{};
            for(int i=e*delay-2;i<=e*(delay+1)+2;++i){const auto ph=std::polar(1.,-w*(i-e*delay));mid+=double(y[i].l+y[i].r)*ph;side+=double(y[i].l-y[i].r)*ph;}
            const std::complex<double> linear=(1.-fraction)+double(fraction)*z;
            const auto out=cubic&&fraction==.5f?(-.0625/z+.5625+.5625*z-.0625*z*z):linear;
            const auto expectedMid=out*std::pow(double(feedback)*linear,e-1);
            const auto expectedSide=out*std::pow(double(feedback)*(1.-2.*cross)*linear,e-1);
            const double err=std::max(std::abs(mid-expectedMid),std::abs(side-expectedSide));maxError=std::max(maxError,err);assert(err<5e-6);checks+=2;
            if(fs==44100&&fraction==.5f&&feedback==.9f&&cross==.35f&&hz==12000&&(e==1||e==4||e==8))
                std::printf("cubic=%d echo=%d mid_db=%.9f side_db=%.9f\n",cubic,e,20*std::log10(std::abs(mid)),20*std::log10(std::abs(side)));
        }
    }
    std::printf("%u complex echo-spectrum checks, max absolute error %.12g\n",checks,maxError);
}
