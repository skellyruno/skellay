#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "../ui/Rack.h"
#include "EngineExchange.h"
#include "TempoPolicy.h"
#include "PresetImport.h"
#include "ProjectState.h"
#include "Delay.h"
#include "FruityDelayBank.h"
#include "codec.h"
#include <array>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <cmath>
using namespace skellay;
namespace {
juce::String modelName(Model m){switch(m){case Model::classic:return "Classic";case Model::modern:return "Modern";case Model::bank:return "Bank";}return "Unknown";}
void put(std::vector<std::uint8_t>& b,size_t p,int v){auto u=static_cast<std::uint32_t>(v);for(unsigned i=0;i<4;++i)b[p+i]=std::uint8_t(u>>(8*i));}
int get(const Configuration& c,size_t p){return int(skelly::delay_research::read32(c.bytes.data()+p));}
class Processor final:public juce::AudioProcessor {
 mutable std::mutex lock; std::array<Configuration,3> configs{defaults(Model::classic),defaults(Model::modern),defaults(Model::bank)}; Model active=Model::classic;
 static size_t index(Model m){return size_t(int(m)-2);}
 std::atomic<unsigned> revision{1}, paramEpoch{0};
 std::atomic<int> audioModel{2};
 std::array<std::array<juce::AudioParameterInt*,133>,3> params{};
 LiveControls readControls(Model model) const noexcept {
  LiveControls c;c.model=model;for(int i=0;i<liveControlCount(model);++i)c.values[i]=params[index(model)][i]->get();return c;
 }
 Configuration merged(Model model) const {
  auto c=configs[index(model)];auto old=liveControls(c), now=readControls(model);
  if(old.values==now.values)return c;
  if(model==Model::bank&&get(c,0)==15){auto st=skelly::fldbank::parse(c.bytes);c.bytes=skelly::fldbank::serialise(*st);}
  for(int i=0;i<liveControlCount(model);++i)put(c.bytes,(model==Model::classic?0:4)+4*i,now.values[i]);return c;
 }
 void syncParams(const Configuration& c){auto raw=liveControls(c);for(int i=0;i<liveControlCount(c.model);++i){auto* p=params[index(c.model)][i];static_cast<juce::AudioProcessorParameter*>(p)->setValue(p->convertTo0to1(float(raw.values[i])));}}
 void registerParams(){
  for(auto model:{Model::classic,Model::modern,Model::bank}){auto raw=liveControls(configs[index(model)]);for(int i=0;i<liveControlCount(model);++i){
   auto name=juce::String(model==Model::classic?SKELLdelay::delay2ParamName(i):model==Model::modern?SKELLdelay::delay3ParamName(i):skelly::fldbank::controlName(i));
   if(model==Model::bank&&i>=4)name="Bank "+juce::String(1+(i-4)/16)+" "+name;else name=modelName(model)+" "+name;
   auto id=juce::String("skellay.")+juce::String(int(model))+".raw."+juce::String(i);
   auto* p=new juce::AudioParameterInt(juce::ParameterID{id,1},name,liveControlMin(model,i),liveControlMax(model,i),raw.values[i]);params[index(model)][i]=p;addParameter(p);
  }}
 }
 std::atomic<bool> running{false}, failed{false};
 std::atomic<double> tempo{120.0}; std::thread worker; EngineExchange exchange; int rate=48000,block=512;
 std::vector<float> mono; std::atomic<float> in{0},out{0};
 void stop(){running=false;if(worker.joinable())worker.join();exchange.reset();}
public:
 Processor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)){registerParams();}
 ~Processor()override{stop();}
 Configuration snapshot()const{std::lock_guard<std::mutex> g(lock);return merged(active);}
 unsigned version()const{return revision.load();}
 bool setConfig(Configuration c){if(!validate(c))return false;std::lock_guard<std::mutex> g(lock);paramEpoch.fetch_add(1);active=c.model;syncParams(c);configs[index(active)]=std::move(c);audioModel.store(int(active));paramEpoch.fetch_add(1);++revision;return true;}
 void select(Model m){std::lock_guard<std::mutex> g(lock);if(active!=m){active=m;audioModel.store(int(m));++revision;}}
 int rawIndex(Model m,size_t pos,bool byte)const noexcept{if(byte)return -1;size_t start=m==Model::classic?0:4;if(pos<start||(pos-start)%4)return -1;auto i=(pos-start)/4;return i<size_t(liveControlCount(m))?int(i):-1;}
 int rawValue(Model m,int i)const noexcept{return params[index(m)][i]->get();}
 void changeRaw(Model m,int i,int value){auto* q=params[index(m)][i];q->setValueNotifyingHost(q->convertTo0to1(float(value)));}
 void beginRaw(Model m,int i){params[index(m)][i]->beginChangeGesture();}
 void endRaw(Model m,int i){params[index(m)][i]->endChangeGesture();}
 bool error()const{return failed.load();}
 float inputPeak()const{return in.load();}float outputPeak()const{return out.load();}
 const juce::String getName()const override{return "Skellay";}
 bool acceptsMidi()const override{return false;}bool producesMidi()const override{return false;}bool isMidiEffect()const override{return false;}
 double getTailLengthSeconds()const override{return 120.0;}
 int getNumPrograms()override{return 1;}int getCurrentProgram()override{return 0;}void setCurrentProgram(int)override{}const juce::String getProgramName(int)override{return {}; }void changeProgramName(int,const juce::String&)override{}
 bool isBusesLayoutSupported(const BusesLayout& b)const override{auto ch=b.getMainOutputChannelSet();return (ch==juce::AudioChannelSet::mono()||ch==juce::AudioChannelSet::stereo())&&ch==b.getMainInputChannelSet();}
 void prepareToPlay(double sr,int maxBlock)override{
  stop();rate=int(std::lround(sr));block=juce::jlimit(32,4096,maxBlock);mono.resize(size_t(block));tempo=120.0;unsigned v;Configuration c;
  {std::lock_guard<std::mutex> g(lock);c=merged(active);v=revision.load();}
  auto first=prepare(c,rate,120,block,60);failed=!first;exchange.initialise(std::move(first),block,std::max(1,rate/50),rate*15);running=true;
  worker=std::thread([this,v,preparedModel=c.model,modernMode=c.model==Model::modern?liveControls(c).values[1]:-1,floor=60.]()mutable{while(running.load()){
   exchange.collect();auto current=revision.load();double bpm=tempo.load();const auto model=static_cast<Model>(audioModel.load());const bool modeChanged=model==Model::modern&&preparedModel==model&&rawValue(model,1)!=modernMode;
   if(current!=v||modeChanged||(preparedModel==Model::bank&&bpm<floor)){Configuration next;{std::lock_guard<std::mutex> g(lock);next=merged(active);current=revision.load();}
    const double nextFloor=bankReserveFloor(bpm);auto e=prepare(next,rate,bpm,block,nextFloor);failed=!e;if(e){exchange.publish(std::move(e));preparedModel=next.model;modernMode=next.model==Model::modern?liveControls(next).values[1]:-1;floor=nextFloor;v=current;}else std::this_thread::sleep_for(std::chrono::milliseconds(250));
   }std::this_thread::sleep_for(std::chrono::milliseconds(20));}
  });
 }
 void releaseResources()override{stop();}
 void processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&)override{
  juce::ScopedNoDenormals noDenormals; if(auto* ph=getPlayHead())if(auto pos=ph->getPosition())if(auto bpm=pos->getBpm())if(std::isfinite(*bpm)&&*bpm>=20&&*bpm<=400)tempo=*bpm;
  const int channels=b.getNumChannels(),n=b.getNumSamples();if(channels<1||n<=0)return;
  float input=0,output=0;for(int k=0;k<channels;++k)input=std::max(input,b.getMagnitude(k,0,n));in=input;
  const auto epoch=paramEpoch.load();const auto raw=readControls(static_cast<Model>(audioModel.load()));const auto* update=(epoch%2==0&&epoch==paramEpoch.load())?&raw:nullptr;
  for(int at=0;at<n;at+=block){int count=std::min(block,n-at);auto* l=b.getWritePointer(0,at);auto* r=channels>1?b.getWritePointer(1,at):mono.data();if(channels==1)std::copy_n(l,count,r);
   if(!exchange.process(l,r,count,tempo.load(),update)){std::fill_n(l,count,0);std::fill_n(r,count,0);}if(channels==1)for(int i=0;i<count;++i)l[i]=.5f*(l[i]+r[i]);}
  for(int k=0;k<channels;++k)output=std::max(output,b.getMagnitude(k,0,n));out=output;
 }
 bool hasEditor()const override{return true;}juce::AudioProcessorEditor* createEditor()override;
 void getStateInformation(juce::MemoryBlock& dest)override{
  std::lock_guard<std::mutex> g(lock);auto saved=configs;for(auto m:{Model::classic,Model::modern,Model::bank})saved[index(m)]=merged(m);auto bytes=encodeProject(ProjectState{active,std::move(saved)});dest.reset();if(!bytes.empty())dest.append(bytes.data(),bytes.size());
 }
 void setStateInformation(const void* data,int n)override{
  if(n<0)return;std::lock_guard<std::mutex> g(lock);auto saved=configs;for(auto m:{Model::classic,Model::modern,Model::bank})saved[index(m)]=merged(m);auto next=decodeProject(data,size_t(n),ProjectState{active,std::move(saved)});if(!next)return;paramEpoch.fetch_add(1);configs=std::move(next->engines);for(auto& c:configs)syncParams(c);active=next->active;audioModel.store(int(active));paramEpoch.fetch_add(1);++revision;
 }
};
#include "../overlay/EditorAdapter.inc"
juce::AudioProcessorEditor* Processor::createEditor(){return new Editor(*this);}
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Processor();}
