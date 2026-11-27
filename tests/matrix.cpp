#include "SkellayEngine.h"
#include "FruityDelayBank.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#define CHECK(b) do{if(!(b)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#b);std::abort();}}while(false)
using namespace skellay;
int main(){
 for(auto m:{Model::classic,Model::modern,Model::bank}){
 auto s=defaults(m);CHECK(validate(s));auto bad=s;bad.bytes.pop_back();CHECK(!validate(bad));
 if(m==Model::bank){auto v=s;v.bytes[0]=15;for(int b=0;b<8;++b){auto p=4+4*(12+16*b);v.bytes[p]=144;v.bytes[p+1]=0;v.bytes[p+2]=0;v.bytes[p+3]=0;}CHECK(validate(v));auto a=skelly::fldbank::parse(v.bytes);CHECK(a&&skelly::fldbank::serialise(*a)==s.bytes);bad=s;bad.bytes[536+16]=2;CHECK(!validate(bad));bad=s;bad.bytes[4]=255;CHECK(!validate(bad));}
 for(int rate:{32000,44100,48000,96000,192000}){
 std::vector<float> refL,refR;double maxLoad=0,maxErr=0;
 for(int block:{32,64,256,511,512,1024,4096}){
 auto engine=prepare(s,rate,130,block);CHECK(engine);const int n=rate;std::vector<float> l(n),r(n);l[0]=.5f;r[0]=-.25f;auto start=std::chrono::steady_clock::now();for(int k=0;k<n;k+=block)CHECK(engine->process(l.data()+k,r.data()+k,std::min(block,n-k)));double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();maxLoad=std::max(maxLoad,ms/10);if(refL.empty()){refL=l;refR=r;}else for(int i=0;i<n;++i)maxErr=std::max({maxErr,double(std::abs(l[i]-refL[i])),double(std::abs(r[i]-refR[i]))});
 l[0]=std::numeric_limits<float>::quiet_NaN();CHECK(!engine->process(l.data(),r.data(),1));CHECK(!engine->process(l.data(),r.data(),block+1));
 }
 std::printf("model=%d rate=%d seven_blocks max_abs_diff=%.9g max_offline_cpu_pct=%.3f\n",int(m),rate,maxErr,maxLoad);
 }
 }
 for(int os:{0,1,2}) {
  auto st=skelly::fldbank::defaultState();st.p[132]=os;st.p[0]=0;
  for(int i=0;i<8;++i){st.bank[i].enabled=true;st.bank[i].mode=1+i%3;st.p[12+16*i]=192*(i+1);st.p[18+16*i]=32;st.bank[i].steps=4;st.bank[i].amount=64;}
  Configuration s{Model::bank,skelly::fldbank::serialise(st)};CHECK(validate(s));auto e=prepare(s,48000,130,256);CHECK(e);double sum=0;std::vector<float> l(48000),r(48000);l[0]=.1f;r[0]=.05f;
  for(int k=0;k<48000;k+=256)CHECK(e->process(l.data()+k,r.data()+k,std::min(256,48000-k)));
  for(int i=0;i<48000;++i)sum+=std::abs(l[i])+std::abs(r[i]);CHECK(sum>0);std::printf("eight active banks oversampling level=%d abs_sum=%.9g\n",os,sum);
 }
}
