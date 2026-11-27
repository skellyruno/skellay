#include "SkellayEngine.h"
#include "codec.h"
#include "FruityDelayBank.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace skellay {
namespace bank=skelly::fldbank;
using namespace SKELLdelay;
using namespace skelly::delay_research;
static bool bankValid(const std::vector<std::uint8_t>& b) {
 if(b.size()!=792) return false;
 auto get=[&](size_t p){return read32(b.data()+p);};
 auto v=get(0); if(v!=15&&v!=16)return false;
 for(int i=0;i<133;++i){auto x=get(4+4*i); if(i==131){if(x< -1||x>7)return false;continue;}if(i==132){if(x<0||x>2)return false;continue;}if(v==15&&i>=4&&(i-4)%16==8)x*=10; if(x<bank::controlMin(i)||x>bank::controlMax(i))return false;}
 for(int i=0;i<8;++i){size_t o=536+32*i;for(int f=0;f<2;++f){if(get(o+8*f)<0||get(o+8*f)>7||get(o+8*f+4)<0||get(o+8*f+4)>2)return false;}if(get(o+17)<0||get(o+17)>3||get(o+21)<0||get(o+25)< -128||get(o+25)>128)return false;for(int j:{16,29,30,31})if(b[o+j]>1)return false;}
 return true;
}
Configuration defaults(Model m){if(m==Model::bank)return {m,bank::serialise(bank::defaultState())};RawState s{static_cast<int>(m),{}};if(m==Model::classic)s.controls.assign(delay2Default.begin(),delay2Default.end());else if(m==Model::modern)s.controls.assign(delay3Default.begin(),delay3Default.end());else throw std::invalid_argument("model");return {m,*encode(s)};}
bool validate(const Configuration& c){return c.model==Model::bank?bankValid(c.bytes):decode(c.bytes,static_cast<int>(c.model)).has_value();}
int liveControlCount(Model m) noexcept {return m==Model::classic?8:m==Model::modern?26:m==Model::bank?bank::automatable:0;}
int liveControlMin(Model m,int i) noexcept {if(i<0||i>=liveControlCount(m))return 0;return m==Model::classic?delay2Min[i]:m==Model::modern?delay3Min[i]:bank::controlMin(i);}
int liveControlMax(Model m,int i) noexcept {if(i<0||i>=liveControlCount(m))return 0;return m==Model::classic?delay2Max[i]:m==Model::modern?delay3Max[i]:bank::controlMax(i);}
bool validLiveControls(const LiveControls& s) noexcept {const int n=liveControlCount(s.model);if(!n)return false;for(int i=0;i<n;++i)if(s.values[i]<liveControlMin(s.model,i)||s.values[i]>liveControlMax(s.model,i))return false;return true;}
LiveControls liveControls(const Configuration& s){LiveControls l;l.model=s.model;if(!validate(s))throw std::invalid_argument("state");if(s.model==Model::bank){auto p=bank::parse(s.bytes);std::copy_n(p->p.begin(),liveControlCount(s.model),l.values.begin());}else{auto p=decode(s.bytes,int(s.model));std::copy(p->controls.begin(),p->controls.end(),l.values.begin());}return l;}
struct Prepared final:Engine {
 LiveControls controls; double tempo=120., bankFloor=20.;
 std::size_t delayStorageBytes() const noexcept override {return b?b->delayStorageBytes():0;}
 bool updateRealtimeControls(const LiveControls& next) noexcept override {
  if(m && next.model==Model::modern && next.values[1]!=controls.values[1])return false;
  return updateControls(next);
 }
 void updateTempo(double bpm) noexcept override {
  if(!std::isfinite(bpm)||bpm<20||bpm>400||bpm==tempo||(b&&bpm<bankFloor))return;
  tempo=bpm;
  if(c){Delay2State s;std::copy_n(controls.values.begin(),8,s.v.begin());c->set(s,bpm);}
  else if(m){if(controls.values[2])m->setTempo(bpm);}else if(b)b->setTempo(bpm);
 }
 bool updateControls(const LiveControls& next) noexcept override {
  if(next.model!=controls.model||!validLiveControls(next))return false;
  if(next.values==controls.values)return true;
  const int n=liveControlCount(next.model);
  if(c){Delay2State s;std::copy_n(next.values.begin(),8,s.v.begin());c->set(s,tempo);}
  else if(m){if(next.values[2]&&!controls.values[2])m->setTempo(tempo);for(int i=0;i<n;++i)if(next.values[i]!=controls.values[i])m->setParameter(i,delay3EngineValue(i,next.values[i]));}
  else {for(int i=0;i<n;++i)if(next.values[i]!=controls.values[i])b->setControl(i,next.values[i]);}
  controls=next;return true;
 }
 int block; std::unique_ptr<Delay2Engine> c;std::unique_ptr<Delay3Engine> m;std::unique_ptr<bank::Engine> b;
 explicit Prepared(int n):block(n){}
 bool process(float* l,float* r,int n)noexcept override {
  if(!l||!r||l==r||n<0||n>block)return false;
  for(int i=0;i<n;++i)if(!std::isfinite(l[i])||!std::isfinite(r[i]))return false;
  if(c)c->process(l,r,n);else if(m)m->process(l,r,n);else b->process(l,r,n);
  for(int i=0;i<n;++i)if(!std::isfinite(l[i])||!std::isfinite(r[i]))return false;
  return true;
 }
};
std::unique_ptr<Engine> prepare(const Configuration& s,int rate,double bpm,int block,double bankMinBpm){
 if(rate<32000||rate>192000||!std::isfinite(bpm)||bpm<20||bpm>400||block<32||block>4096||!std::isfinite(bankMinBpm)||bankMinBpm<20||bankMinBpm>bpm||!validate(s))return {};
 try {auto e=std::make_unique<Prepared>(block);e->controls=liveControls(s);e->tempo=bpm;e->bankFloor=bankMinBpm; if(s.model==Model::classic){auto p=decode(s.bytes,2);Delay2State st;std::copy(p->controls.begin(),p->controls.end(),st.v.begin());e->c=std::make_unique<Delay2Engine>(st,rate,bpm,block,true);e->c->enableLiveTiming(20);}else if(s.model==Model::modern){auto p=decode(s.bytes,3);e->m=std::make_unique<Delay3Engine>(rate);e->m->setTempo(bpm);for(int i=0;i<26;++i)e->m->setParameter(i,delay3EngineValue(i,p->controls[i]));e->m->settle();}else {auto p=bank::parse(s.bytes);if(!p)return {};e->b=std::make_unique<bank::Engine>(*p,rate,bpm,block,true);e->b->enableLiveTiming(bankMinBpm);}return e;}catch(...){return {};}
}
}
