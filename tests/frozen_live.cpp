#include "SkellayEngine.h"
#include "FruityDelayBank.h"
#include "Delay.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
using namespace skellay;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(false)
int main(){
 for(int sr:{32000,48000,96000})for(int os:{0,1,2})for(int block:{32,257,512}){
  auto st=skelly::fldbank::defaultState();st.p[132]=os;for(int i=0;i<8;++i){st.bank[i].enabled=i<3;st.bank[i].mode=1+i%3;st.bank[i].steps=3;st.bank[i].amount=i%2?-64:64;st.bank[i].tapBeforeFeedback=true;st.p[16*i+12]=400+173*i;st.p[16*i+13]=i%2?40:-50;st.p[16*i+10]=32;st.p[16*i+18]=48;}
  skelly::fldbank::Engine frozen(st,sr,130,block,true);auto live=prepare({Model::bank,skelly::fldbank::serialise(st)},sr,130,block);CHECK(live);
  float maxDiff=0;std::vector<float> l(block),r(block),a(block),b(block);for(int at=0;at<sr;at+=block){int n=std::min(block,sr-at);for(int i=0;i<n;++i){l[i]=a[i]=.02f*std::sin(.03f*(at+i));r[i]=b[i]=.01f*std::sin(.07f*(at+i));}frozen.process(a.data(),b.data(),n);CHECK(live->process(l.data(),r.data(),n));for(int i=0;i<n;++i)maxDiff=std::max({maxDiff,std::abs(l[i]-a[i]),std::abs(r[i]-b[i])});}CHECK(maxDiff<1e-6);std::printf("bank frozen/live sr=%d os=%d block=%d max_diff=%.9g\n",sr,1<<os,block,maxDiff);
 }
}
