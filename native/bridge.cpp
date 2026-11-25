#include "Delay.h"
#include <algorithm>
#include <cmath>
#include <vector>
using namespace SKELLdelay;
extern "C" int delay_render(const float* in,float* out,int n,int sr,double bpm,int model,const int* raw,int block){
 if(!in||!out||!raw||n<1||sr<32000||sr>192000||!std::isfinite(bpm)||bpm<20||bpm>400||block<1||block>4096||(model!=2&&model!=3))return 0;
 for(int i=0;i<n*2;++i)if(!std::isfinite(in[i])||std::abs(in[i])>4.f)return 0;
 try{
 std::vector<float> l(block),r(block);std::unique_ptr<Delay2Engine> d2;std::unique_ptr<Delay3Engine> d3;
 if(model==2){Delay2State s;for(int i=0;i<8;++i){if(raw[i]<delay2Min[i]||raw[i]>delay2Max[i])return 0;s.v[i]=raw[i];}d2=std::make_unique<Delay2Engine>(s,sr,bpm,block,true);}
 else{for(int i=0;i<26;++i)if(raw[i]<delay3Min[i]||raw[i]>delay3Max[i])return 0;d3=std::make_unique<Delay3Engine>(sr);d3->setTempo(bpm);for(int i=0;i<26;++i)d3->setParameter(i,delay3EngineValue(i,raw[i]));d3->settle();}
 for(int k=0;k<n;k+=block){int m=std::min(block,n-k);for(int i=0;i<m;++i){l[i]=in[2*(k+i)];r[i]=in[2*(k+i)+1];}if(d2)d2->process(l.data(),r.data(),m);else d3->process(l.data(),r.data(),m);for(int i=0;i<m;++i){if(!std::isfinite(l[i])||!std::isfinite(r[i]))return 0;out[2*(k+i)]=l[i];out[2*(k+i)+1]=r[i];}}
 return 1;}catch(...){return 0;}
}
