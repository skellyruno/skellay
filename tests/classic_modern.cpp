#include "codec.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace SKELLdelay;using namespace skelly::delay_research;
extern "C" int delay_render(const float*,float*,int,int,double,int,const int*,int);
#define CHECK(b) do{if(!(b)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#b);std::abort();}}while(false)
int main(){
 for(int model:{2,3}){
  RawState p{model,{}};if(model==2)p.controls.assign(delay2Default.begin(),delay2Default.end());else p.controls.assign(delay3Default.begin(),delay3Default.end());
  for(std::size_t i=0;i<p.controls.size();++i){const int lo=model==2?delay2Min[i]:delay3Min[i],hi=model==2?delay2Max[i]:delay3Max[i];auto q=p;for(int v:{lo,hi,0}){if(v<lo||v>hi)continue;q.controls[i]=v;auto a=encode(q);CHECK(a);auto b=decode(*a,model);CHECK(b&&b->controls==q.controls);}q.controls[i]=hi+1;CHECK(!encode(q));}
  auto bytes=encode(p);CHECK(bytes);auto broken=*bytes;broken.pop_back();CHECK(!decode(broken,model));if(model==3){broken=*bytes;broken[0]=2;CHECK(!decode(broken,model));}
  for(int rate:{32000,44100,48000,96000,192000}){const int n=rate;std::vector<float> x(2*n),y(2*n),z(2*n);for(int i=0;i<n;++i){x[2*i]=.1f*std::sin(i*.05f);x[2*i+1]=.12f*std::cos(i*.07f);}CHECK(delay_render(x.data(),y.data(),n,rate,130,model,p.controls.data(),512));CHECK(delay_render(x.data(),z.data(),n,rate,130,model,p.controls.data(),32));CHECK(y==z);CHECK(!delay_render(x.data(),z.data(),n,rate,0,model,p.controls.data(),512));x[0]=std::numeric_limits<float>::quiet_NaN();CHECK(!delay_render(x.data(),z.data(),n,rate,130,model,p.controls.data(),512));}
 }std::puts("Delay 2/3 PASS: codec endpoints, invalid records, five rates, fixed-block parity, nonfinite/tempo rejection");
}
