#include "../include/ResearchDelay.h"
using D=skellay::research::ResearchDelay;
extern "C" int render_delay(const float* x,float* y,unsigned n,unsigned tail,double fs,int mode){
 if(!x||!y||!n||mode<0||mode>2)return 0;
 D d;if(!d.prepare(fs))return 0;
 D::Controls p{120,.45f,mode==0?0.f:.4f,.6f};
 for(unsigned i=0;i<n+tail;++i){if(mode==2&&i>=n/2)p.milliseconds=220;auto v=d.process(i<n?D::Frame{x[2*i],x[2*i+1]}:D::Frame{},p);y[2*i]=v.l;y[2*i+1]=v.r;}
 return 1;
}
