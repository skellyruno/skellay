#pragma once
#include "SkellayEngine.h"
#include <atomic>
#include <algorithm>
#include <vector>
namespace skellay {
// One producer (control thread), one consumer (audio thread). Destruction is
// never on audio thread. stop/reset only after both threads have stopped.
class EngineExchange {
 std::atomic<Engine*> pending{nullptr}, retired{nullptr};
 Engine* current=nullptr; Engine* fading=nullptr;
 int remaining=0, fade=1, capacity=0, tailLength=0, tailRemaining=0;
 std::vector<float> l,r;
public:
 EngineExchange()=default; EngineExchange(const EngineExchange&)=delete;
 ~EngineExchange(){reset();}
 void initialise(std::unique_ptr<Engine> e,int block,int fadeSamples,int tailSamples=0){reset();l.resize(block);r.resize(block);capacity=block;fade=std::max(1,fadeSamples);tailLength=std::max(0,tailSamples);current=e.release();}
 void publish(std::unique_ptr<Engine> e){collect();delete pending.exchange(e.release(),std::memory_order_acq_rel);}
 void collect(){delete retired.exchange(nullptr,std::memory_order_acq_rel);}
 void reset(){delete pending.exchange(nullptr);collect();delete current;delete fading;current=fading=nullptr;remaining=tailRemaining=0;}
 bool process(float* left,float* right,int n,double bpm=0,const LiveControls* controls=nullptr) noexcept {
  if(!left||!right||left==right||n<0||n>capacity)return false;
  if(fading&&remaining==0&&tailRemaining==0&&!retired.load(std::memory_order_acquire)){retired.store(fading,std::memory_order_release);fading=nullptr;}
  if(!fading){if(auto* next=pending.exchange(nullptr,std::memory_order_acq_rel)){fading=current;current=next;remaining=fading?fade:0;tailRemaining=fading?tailLength:0;}}
  if(!current)return true;
  if(controls)current->updateRealtimeControls(*controls);
  if(bpm>=20&&bpm<=400){current->updateTempo(bpm);if(fading&&remaining&&!tailLength)fading->updateTempo(bpm);}
  if(tailLength&&fading&&(remaining||tailRemaining)){
   // Feed the outgoing engine only a fading share of new input, then silence.
   // Preserve its scheduled repeats; bound memory to one outgoing engine.
   if(pending.load(std::memory_order_acquire)&&tailRemaining>fade)tailRemaining=fade;
   for(int i=0;i<n;++i){const float old=std::max(0,remaining-i)/float(fade);l[i]=left[i]*old;r[i]=right[i]*old;left[i]*=1-old;right[i]*=1-old;}
   const bool ok=current->process(left,right,n);const bool oldOk=fading->process(l.data(),r.data(),n);
   for(int i=0;i<n;++i){const float w=std::min(1.f,tailRemaining/float(fade));if(oldOk){left[i]+=l[i]*w;right[i]+=r[i]*w;}if(tailRemaining>0)--tailRemaining;if(remaining>0)--remaining;}
   return ok;
  }
  if(!fading||!remaining)return current->process(left,right,n);
  std::copy_n(left,n,l.data());std::copy_n(right,n,r.data());
  bool ok=current->process(left,right,n);bool oldOk=fading->process(l.data(),r.data(),n);
  for(int i=0;i<n;++i){float w=1.0f-float(remaining)/float(fade);if(remaining>0)--remaining;if(oldOk){left[i]=l[i]*(1-w)+left[i]*w;right[i]=r[i]*(1-w)+right[i]*w;}}
  return ok;
 }
};
}
