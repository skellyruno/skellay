#include "../include/ResearchDelay.h"
using D=skellay::research::ResearchDelay;
extern "C" int cubic_delay(int cubic,const float* x,float* y,unsigned n,unsigned tail,double fs,double samples,float feedback,int jump){
    if(!x||!y||!n||cubic<0||cubic>1||!std::isfinite(samples)||!std::isfinite(feedback)||samples<1||samples>fs)return 0;
    D d;if(!d.prepare(fs,2,cubic!=0))return 0;
    D::Controls p{1000*samples/fs,feedback,1,.35f};
    for(unsigned i=0;i<n+tail;++i){if(jump&&i>=n/2)p.milliseconds=1000*(samples+fs*.1)/fs;auto v=d.process(i<n?D::Frame{x[2*i],x[2*i+1]}:D::Frame{},p);y[2*i]=v.l;y[2*i+1]=v.r;}
    return 1;
}
