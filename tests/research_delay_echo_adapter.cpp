#include "../include/ResearchDelay.h"
using D=skellay::research::ResearchDelay;
extern "C" int echo_render(const float* x,float* y,unsigned n,unsigned total,double fs,double samples,float fb,float wet,float cross,int cubic,float trim,int jump){
    if(!x||!y||!n||total<n||!std::isfinite(samples)||samples<1||samples>fs||cubic<0||cubic>1)return 0;
    D d;if(!d.prepare(fs,2,cubic!=0,trim))return 0;
    D::Controls p{1000*samples/fs,fb,wet,cross};
    for(unsigned i=0;i<total;++i){if(jump&&i>=n/2)p.milliseconds=1000*(samples+fs*.1)/fs;auto v=d.process(i<n?D::Frame{x[2*i],x[2*i+1]}:D::Frame{},p);y[2*i]=v.l;y[2*i+1]=v.r;}
    return 1;
}
