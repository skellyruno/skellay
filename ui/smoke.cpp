#include "Rack.h"
#include <iostream>
using namespace skellay_ui;
int main(int argc,char** argv){
 juce::ScopedJuceInitialiser_GUI init;
 if(argc!=2)return 2;
 juce::File output(juce::String::fromUTF8(argv[1]));if(output.createDirectory().failed())return 3;
 Rack rack; rack.setVisible(true);
 int key=0,got=-1,value=-1; rack.onValue=[&](int k,int v){got=k;value=v;};
 auto group=[&](const char* title,std::initializer_list<const char*> names){Section s{title,{}};for(auto n:names)s.controls.push_back({n,0,128,64,key++,false});return s;};
 for(int mode=0;mode<3;++mode){
  key=0;std::vector<Section> sections;
  if(mode==0)sections={group("01 / TIMING & FEEDBACK",{"Time","Stereo offset","Feedback","Cutoff"}),group("02 / INPUT & ROUTING",{"Input pan","Input volume","Dry","Mode"})};
  else if(mode==1)sections={group("01 / TIMING",{"Time","Tempo sync","Keep pitch","Time smoothing"}),group("02 / STEREO",{"Delay type","Stereo spread","Time R / pan"}),group("03 / MODULATION",{"Mod rate","Mod amount","Cutoff mod"}),group("04 / DIFFUSION",{"Diffusion","Diffusion spread"}),group("05 / FEEDBACK & FILTER",{"Feedback","Filter mode","Cutoff","Resonance"}),group("06 / DISTORTION",{"Saturation type","Distortion","Hardness","Symmetry"}),group("07 / DEGRADATION",{"Rate reduce","Bit reduce"}),group("08 / OUTPUT",{"Send","Wet","Dry","Colour"})};
  else sections={group("GLOBAL / MIX & ENGINE",{"Dry","Output","Input","Feedback","Solo","Oversampling"}),group("BANK / TIMING & ROUTE",{"Time","Stereo offset","Sync","Enabled","Routing mode","Tap pre-feedback"}),group("INPUT & FEEDBACK",{"Input pan","Input volume","Feedback pan","Feedback level","Feedback cross"}),group("FILTER 1",{"Cutoff","Resonance","Gain","Type","Order","Filter after"}),group("FILTER 2",{"Cutoff","Resonance","Gain","Type","Order"}),group("PATTERN & OUTPUT",{"Output pan","Output volume","Send to next","Pattern steps","Pattern amount"})};
  // Synthetic visual fixture only: no DSP, actual presets or fake audio metrics.
  for(auto& s:sections)for(auto& c:s.controls)if(c.label=="Mode"||c.label=="Sync"||c.label=="Enabled"||c.label=="Tempo sync"||c.label=="Keep pitch"||c.label=="Type"||c.label=="Order"||c.label=="Oversampling"||c.label=="Delay type"||c.label=="Filter mode"||c.label=="Saturation type"||c.label=="Routing mode"||c.label=="Tap pre-feedback"||c.label=="Filter after"){c.hi=3;c.value=1;c.discrete=true;}
  rack.setSize(588,414);rack.setMode(mode,2);rack.setSections(std::move(sections));rack.setPreset("UI fixture / no audio engine");rack.setStatus("Original integer controls  |  UI component test");
  if(rack.controlCount()!=key||!rack.geometryValid()||!rack.testChange(0,27)||got!=0||value!=27)return 4;
  for(auto dims:{std::pair<int,int>{560,370},{588,414},{1100,800}}){rack.setSize(dims.first,dims.second);if(!rack.geometryValid())return 5;}
  rack.setSize(588,414);
  auto image=rack.createComponentSnapshot(rack.getLocalBounds(),true,1.f);
  auto file=output.getChildFile(juce::String(mode==0?"classic":mode==1?"modern":"bank")+".png");auto stream=file.createOutputStream();juce::PNGImageFormat png;
  if(!stream||!png.writeImageToStream(image,*stream))return 6;
  std::cout<<"Mode "<<mode<<": "<<key<<" controls; layout and value event passed\n";
 }
 return 0;
}
