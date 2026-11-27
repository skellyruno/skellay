#include "EngineExchange.h"
#include "FruityDelayBank.h"
#include "ProjectState.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
using namespace skellay;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(false)
static void put(Configuration& s,int pos,int v){for(int i=0;i<4;++i)s.bytes[pos+i]=std::uint8_t(std::uint32_t(v)>>(i*8));}
int main(){
 // Compare actual allocated vector capacities at high rate; one engine at a time.
 auto st=skelly::fldbank::defaultState();st.p[132]=2;for(int i=0;i<8;++i){st.bank[i].enabled=true;st.bank[i].sync=true;st.bank[i].mode=1;}
 Configuration b{Model::bank,skelly::fldbank::serialise(st)};
 std::size_t full=0,small=0;{auto e=prepare(b,192000,120,4096);CHECK(e);full=e->delayStorageBytes();}{auto e=prepare(b,192000,120,4096,60);CHECK(e);small=e->delayStorageBytes();}
 CHECK(small<full/2);std::printf("Bank delay storage 192k/4x, 8 banks: full=%zu compact=%zu reduction=%.2f%%\n",full,small,100.*(1.-double(small)/full));
 CHECK(bankReserveFloor(120)==60&&bankReserveFloor(50)==40&&bankReserveFloor(30)==20);
 // Compact and full reserve match through valid tempo and timing changes, including max time.
 auto x=prepare(b,48000,120,128,60),y=prepare(b,48000,120,128);CHECK(x&&y);auto c=liveControls(b);float l[128],r[128],a[128],d[128];
 for(int at=0;at<48000*5;at+=128){double bpm=at<24000?120:60;x->updateTempo(bpm);y->updateTempo(bpm);if(at>=24000)c.values[12]=7680;CHECK(x->updateControls(c));CHECK(y->updateControls(c));for(int i=0;i<128;++i){l[i]=a[i]=.01f*std::sin(.013f*(at+i));r[i]=d[i]=.01f*std::cos(.03f*(at+i));}CHECK(x->process(l,r,128));CHECK(y->process(a,d,128));for(int i=0;i<128;++i){CHECK(l[i]==a[i]);CHECK(r[i]==d[i]);}}
 // Old scheduled repeats survive a structural swap even after the initial 20 ms fade.
 for(auto m:{Model::classic,Model::modern,Model::bank}){
  auto s=defaults(m);if(m==Model::classic)put(s,8,0);else if(m==Model::modern){put(s,104,0);put(s,20,192);}else {auto bs=skelly::fldbank::defaultState();bs.p[0]=0;bs.bank[0].tapBeforeFeedback=true;s.bytes=skelly::fldbank::serialise(bs);}
  auto old=prepare(s,48000,120,128);auto ref=prepare(s,48000,120,128);CHECK(old&&ref);EngineExchange ex;ex.initialise(std::move(old),128,960,48000*2);
  auto other=defaults(m==Model::modern?Model::modern:Model::classic);if(m==Model::modern)put(other,8,2); // model switch
  double energy=0;for(int at=0;at<48000;at+=128){std::fill_n(l,128,0);std::fill_n(r,128,0);if(!at)l[0]=.25f;std::copy_n(l,128,a);std::copy_n(r,128,d);
   if(at==4096){ex.publish(prepare(other,48000,120,128));}auto changed=liveControls(other);CHECK(ex.process(l,r,128,120,at>=4096?&changed:nullptr));CHECK(ref->process(a,d,128));if(at>=8192)for(int i=0;i<128;++i){CHECK(l[i]==a[i]);CHECK(r[i]==d[i]);energy+=std::abs(l[i])+std::abs(r[i]);}}
  CHECK(energy>.001);std::printf("Structural swap model=%d outgoing-tail abs sum=%.9g exact reference\n",int(m),energy);
 }
 // A raw Modern topology change must not clear active engine before swap.
 auto modern=defaults(Model::modern);auto e=prepare(modern,48000,120,128);CHECK(e);auto mc=liveControls(modern);mc.values[1]=(mc.values[1]+1)%4;CHECK(!e->updateRealtimeControls(mc));
 std::puts("Compact storage and structural tails passed");
}
