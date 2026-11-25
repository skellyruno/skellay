// Adapter for the verified native v0.7 Processor. Raw parameter IDs remain unchanged.
class Editor final:public juce::AudioProcessorEditor,private juce::Timer {
 Processor& p; skellay_ui::Rack rack; Model displayedModel=Model::classic; int selectedBank=0;
 struct Binding{size_t pos;bool byte;int raw;};
 std::vector<Binding> bindings;
 std::unique_ptr<juce::FileChooser> chooser;unsigned seen=0;
 void setRaw(int key,int v){
  if(key<0||size_t(key)>=bindings.size())return;auto b=bindings[size_t(key)];auto c=p.snapshot();if(c.model!=displayedModel)return;
  if(b.raw>=0){p.changeRaw(c.model,b.raw,v);return;}
  if(c.model==Model::bank&&get(c,0)==15){auto state=skelly::fldbank::parse(c.bytes);if(!state)return;c.bytes=skelly::fldbank::serialise(*state);}
  if(b.pos>=c.bytes.size())return;if(b.byte)c.bytes[b.pos]=std::uint8_t(v);else put(c.bytes,b.pos,v);
  if(p.setConfig(std::move(c)))seen=p.version();
 }
 void rebuild(bool reset=true){
  auto c=p.snapshot();seen=p.version();displayedModel=c.model;bindings.clear();
  rack.setMode(int(c.model)-2,selectedBank);std::vector<skellay_ui::Section> sections;
  auto section=[&](const char* title){sections.push_back({title,{}});};
  auto add=[&](juce::String name,int lo,int hi,size_t pos,bool byte=false){
   int v=byte?c.bytes[pos]:get(c,pos);int key=int(bindings.size());bindings.push_back({pos,byte,p.rawIndex(c.model,pos,byte)});
   sections.back().controls.push_back({name,lo,hi,v,key,hi-lo<=8});
  };
  if(c.model==Model::classic){
   auto raw=[&](int i){add(SKELLdelay::delay2ParamName(i),SKELLdelay::delay2Min[i],SKELLdelay::delay2Max[i],4*i);};
   section("01 / TIMING & FEEDBACK");for(int i:{4,5,3,7})raw(i);
   section("02 / INPUT & ROUTING");for(int i:{0,1,2,6})raw(i);
  }else if(c.model==Model::modern){
   auto raw=[&](int i){add(SKELLdelay::delay3ParamName(i),SKELLdelay::delay3Min[i],SKELLdelay::delay3Max[i],4+4*i);};
   section("01 / TIMING");for(int i:{4,2,3,6})raw(i);
   section("02 / STEREO");for(int i:{1,7,5})raw(i);
   section("03 / MODULATION");for(int i:{8,9,10})raw(i);
   section("04 / DIFFUSION");for(int i:{11,12})raw(i);
   section("05 / FEEDBACK & FILTER");for(int i:{14,15,16,17})raw(i);
   section("06 / DISTORTION");for(int i:{13,18,19,20})raw(i);
   section("07 / DEGRADATION");for(int i:{21,22})raw(i);
   section("08 / OUTPUT");for(int i:{0,23,25,24})raw(i);
  }else if(auto st=skelly::fldbank::parse(c.bytes)){
   c.bytes=skelly::fldbank::serialise(*st);auto raw=[&](int i){add(i==131?"Solo (-1 = off)":i==132?"Oversampling":skelly::fldbank::controlName(i),i==131?-1:i==132?0:skelly::fldbank::controlMin(i),i==131?7:i==132?2:skelly::fldbank::controlMax(i),4+4*i);};
   section("GLOBAL / MIX & ENGINE");for(int i=0;i<4;++i)raw(i);raw(131);raw(132);
   int k=4+16*selectedBank;size_t o=536+32*selectedBank;
   auto option=[&](const char* name,int lo,int hi,size_t off,bool byte=false){add(name,lo,hi,o+off,byte);};
   auto local=[&](int i){if(k+i<131)raw(k+i);};
   section("BANK / TIMING & ROUTE");local(8);local(9);option("Sync",0,1,31,true);option("Enabled",0,1,29,true);option("Routing mode",0,3,17);option("Tap pre-feedback",0,1,30,true);
   section("INPUT & FEEDBACK");for(int i:{0,1,5,6,7})local(i);
   section("FILTER 1");for(int i:{2,3,4})local(i);option("Type",0,7,0);option("Order",0,2,4);option("Filter after",0,1,16,true);
   section("FILTER 2");for(int i:{10,11,12})local(i);option("Type",0,7,8);option("Order",0,2,12);
   section("PATTERN & OUTPUT");for(int i:{13,14,15})local(i);option("Pattern steps",0,std::max(128,get(c,o+21)),21);option("Pattern amount",-128,128,25);
  }
  rack.setSections(std::move(sections),reset);
 }
 void apply(Configuration c,juce::String name){if(p.setConfig(std::move(c))){rack.setPreset(name);rack.setStatus("Preset loaded");rebuild();}else rack.setStatus("Invalid delay configuration");}
 void openFile(){
  chooser=std::make_unique<juce::FileChooser>("Import FL delay settings",juce::File{},"*.fst;*.flp;*.bin");
  juce::Component::SafePointer<Editor> self(this);
  chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[self](const juce::FileChooser& fc){
   if(!self)return;auto f=fc.getResult();if(!f.existsAsFile())return;juce::MemoryBlock bytes;
   if(f.getSize()<=0||f.getSize()>64*1024*1024||!f.loadFileAsData(bytes)||bytes.getSize()==0){self->rack.setStatus("Empty, too large or unreadable");return;}
   auto* begin=static_cast<const std::uint8_t*>(bytes.getData());std::vector<std::uint8_t> data(begin,begin+bytes.getSize());auto name=f.getFileNameWithoutExtension();
   if(f.hasFileExtension("bin")){auto c=importRaw(data);if(c)self->apply(std::move(*c),name);else self->rack.setStatus("Invalid raw delay settings");return;}
   auto result=scanFlPreset(data);if(result.entries.empty()){self->rack.setStatus(juce::String(result.error));return;}
   if(result.entries.size()==1){self->apply(std::move(result.entries.front().state),name);return;}
   auto entries=std::make_shared<std::vector<PresetEntry>>(std::move(result.entries));juce::PopupMenu menu;
   for(size_t i=0;i<entries->size();++i){auto& e=(*entries)[i];menu.addItem(int(i+1),modelName(e.state.model)+" | track "+juce::String(e.track)+" / slot "+juce::String(e.slot));}
   menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(self->rack.importTarget()),[self,entries,name](int choice){if(self&&choice>0&&size_t(choice)<=entries->size())self->apply(std::move((*entries)[size_t(choice-1)].state),name);});
  });
 }
 void exportFile(){
  auto c=p.snapshot();chooser=std::make_unique<juce::FileChooser>("Export raw delay settings",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Skellay.bin"),"*.bin");
  juce::Component::SafePointer<Editor> self(this);chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[self,c=std::move(c)](const juce::FileChooser& fc){if(!self)return;auto f=fc.getResult();if(f==juce::File{})return;self->rack.setStatus(f.replaceWithData(c.bytes.data(),c.bytes.size())?"Raw settings exported":"Export failed");});
 }
 void timerCallback()override{
  if(seen!=p.version())rebuild(false);else for(size_t i=0;i<bindings.size();++i)if(bindings[i].raw>=0)rack.updateValue(int(i),p.rawValue(displayedModel,bindings[i].raw));
  rack.setMeter(p.inputPeak(),p.outputPeak(),p.error());
 }
public:
 explicit Editor(Processor& proc):AudioProcessorEditor(proc),p(proc){
  addAndMakeVisible(rack);rack.onModel=[this](int i){p.select(static_cast<Model>(i+2));rack.setPreset("Current engine state");rebuild();};
  rack.onBank=[this](int i){selectedBank=i;rebuild();};rack.onImport=[this]{openFile();};rack.onExport=[this]{exportFile();};
  rack.onValue=[this](int key,int value){setRaw(key,value);};
  rack.onBegin=[this](int key){if(key>=0&&size_t(key)<bindings.size()&&bindings[size_t(key)].raw>=0)p.beginRaw(displayedModel,bindings[size_t(key)].raw);};
  rack.onEnd=[this](int key){if(key>=0&&size_t(key)<bindings.size()&&bindings[size_t(key)].raw>=0)p.endRaw(displayedModel,bindings[size_t(key)].raw);};
  setResizable(true,true);setResizeLimits(760,460,1440,1000);setSize(980,690);rebuild();startTimerHz(20);
 }
 ~Editor()override{stopTimer();chooser.reset();}
 void resized()override{rack.setBounds(getLocalBounds());}
};
