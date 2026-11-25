#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>
#include <memory>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

namespace skellay_ui {
inline const juce::Colour green{0xff9be578}, ink{0xff101311}, panel{0xff1b201d}, muted{0xff9ba79e}, pale{0xffe6ece5}, border{0xff353e37};
class HardwareLook final : public juce::LookAndFeel_V4 {
public:
 HardwareLook(){
  setColour(juce::TextButton::buttonColourId, panel);setColour(juce::TextButton::buttonOnColourId,green);
  setColour(juce::TextButton::textColourOffId,pale);setColour(juce::TextButton::textColourOnId,ink);
  setColour(juce::Slider::textBoxTextColourId,pale);setColour(juce::Slider::textBoxBackgroundColourId,ink);
  setColour(juce::Slider::textBoxOutlineColourId,border);setColour(juce::Slider::textBoxHighlightColourId,green.withAlpha(.3f));
  setColour(juce::ScrollBar::thumbColourId,border.brighter(.3f));
 }
 void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider&) override {
  const auto r=juce::Rectangle<float>(float(x),float(y),float(w),float(h)).reduced(5);
  const float radius=std::min(r.getWidth(),r.getHeight())*.5f;
  if(radius<12)return;
  const auto c=r.getCentre();const float a=start+juce::jlimit(0.f,1.f,pos)*(end-start);
  auto circle=[&](float rr){return juce::Rectangle<float>(c.x-rr,c.y-rr,rr*2,rr*2);};
  auto arc=[&](float from,float to,juce::Colour col,float thick){if(to<=from)return;juce::Path p;p.addCentredArc(c.x,c.y,radius,radius,0,from,to,true);g.setColour(col);g.strokePath(p,juce::PathStrokeType(thick,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));};
  arc(start,end,border,3.f);arc(start,a,green,3.f);
  const auto outer=circle(radius-5);g.setColour(juce::Colours::black.withAlpha(.65f));g.fillEllipse(outer.translated(0,2));
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff59635b),outer.getX(),outer.getY(),juce::Colour(0xff111512),outer.getRight(),outer.getBottom(),false));g.fillEllipse(outer);
  g.setColour(border);g.drawEllipse(outer,1.f);
  const auto face=outer.reduced(3);g.setGradientFill(juce::ColourGradient(juce::Colour(0xff343c36),face.getCentreX(),face.getY(),juce::Colour(0xff181c19),face.getCentreX(),face.getBottom(),false));g.fillEllipse(face);
  for(int i=0;i<32;++i){float t=float(i)*juce::MathConstants<float>::twoPi/32.f;float q=radius-7;g.setColour(juce::Colours::black.withAlpha(.22f));g.drawLine(c.x+std::sin(t)*q,c.y-std::cos(t)*q,c.x+std::sin(t)*(q-2),c.y-std::cos(t)*(q-2),1);}
  g.setColour(green);g.drawLine(c.x+std::sin(a)*(radius*.35f),c.y-std::cos(a)*(radius*.35f),c.x+std::sin(a)*(radius-9),c.y-std::cos(a)*(radius-14),2.5f);
 }
 void drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool over,bool down) override {
  auto r=b.getLocalBounds().toFloat().reduced(.5f);auto c=b.getToggleState()?green:panel;
  if(over)c=c.brighter(.12f);if(down)c=c.darker(.15f);
  g.setColour(c);g.fillRoundedRectangle(r,5);g.setColour(b.getToggleState()?green:border);g.drawRoundedRectangle(r,5,1);
 }
};
struct Control {juce::String label;int lo=0,hi=1,value=0;int key=0;bool discrete=false;};
struct Section {juce::String title;std::vector<Control> controls;};
class Cell final : public juce::Component {
public:
 Control spec;juce::Label label;juce::Slider slider;
 explicit Cell(Control c):spec(std::move(c)){
  label.setText(spec.label,juce::dontSendNotification);label.setJustificationType(juce::Justification::centred);
  label.setColour(juce::Label::textColourId,muted);label.setFont(juce::Font(juce::FontOptions(12.f)));
  addAndMakeVisible(label);addAndMakeVisible(slider);
  slider.setName(spec.label);slider.setTooltip(spec.label+" (original integer value)");
  slider.setSliderStyle(spec.discrete?juce::Slider::IncDecButtons:juce::Slider::RotaryHorizontalVerticalDrag);
  slider.setRotaryParameters(juce::MathConstants<float>::pi*1.25f,juce::MathConstants<float>::pi*2.75f,true);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,52,17);
  slider.setRange(double(spec.lo),double(spec.hi),1);slider.setValue(spec.value,juce::dontSendNotification);
  slider.setScrollWheelEnabled(false);
 }
 void resized() override {
  auto r=getLocalBounds().reduced(2);label.setBounds(r.removeFromTop(16));slider.setBounds(r);
 }
};
class Group final : public juce::Component {
public:
 juce::String name;std::vector<std::unique_ptr<Cell>> cells;
 explicit Group(juce::String n):name(std::move(n)){}
 int heightFor(int width)const {const int cols=std::max(1,width/66);return 29+((int(cells.size())+cols-1)/cols)*91;}
 void paint(juce::Graphics& g)override{
  auto r=getLocalBounds().toFloat().reduced(.5f);g.setColour(panel);g.fillRoundedRectangle(r,7);g.setColour(border);g.drawRoundedRectangle(r,7,1);
  g.setColour(green.withAlpha(.8f));g.fillRoundedRectangle(9,9,3,12,1);
  g.setColour(pale);g.setFont(juce::Font(juce::FontOptions(12.f).withStyle("Bold")));g.drawText(name,18,3,getWidth()-24,24,juce::Justification::centredLeft);
 }
 void resized()override{
  auto r=getLocalBounds().reduced(5);r.removeFromTop(24);
  int cols=std::max(1,getWidth()/66);cols=std::min(cols,std::max(1,int(cells.size())));
  int width=r.getWidth()/cols;
  for(size_t i=0;i<cells.size();++i)cells[i]->setBounds(r.getX()+int(i%size_t(cols))*width,r.getY()+int(i/size_t(cols))*91,width,91);
 }
};
class Rack final:public juce::Component {
 HardwareLook look;juce::TooltipWindow tooltip{this,500};
 std::array<juce::TextButton,3> modes;std::array<juce::TextButton,8> banks;
 juce::TextButton importButton{"IMPORT FL"},exportButton{"EXPORT RAW"};
 juce::Label status;juce::Viewport viewport;juce::Component content;
 std::vector<std::unique_ptr<Group>> groups;int mode=0,bank=0;float in=0,out=0;bool error=false;juce::String preset{"Factory / current state"};
 void layoutGroups(){
  int w=std::max(240,viewport.getWidth()-viewport.getScrollBarThickness()-4),y=0;
  // Pair compact sections on wider editors; large sections occupy a complete row.
  for(size_t i=0;i<groups.size();){
   auto& a=*groups[i];const bool pair=mode!=0&&i+1<groups.size()&&w>=520&&a.cells.size()<=4&&groups[i+1]->cells.size()<=4;
   if(pair){int half=(w-6)/2;int h=std::max(a.heightFor(half),groups[i+1]->heightFor(w-half-6));a.setBounds(0,y,half,h);groups[i+1]->setBounds(half+6,y,w-half-6,h);y+=h+6;i+=2;}
   else {int h=a.heightFor(w);a.setBounds(0,y,w,h);y+=h+6;++i;}
  }
  content.setSize(w,std::max(1,y));
 }
 void meter(juce::Graphics& g,juce::Rectangle<int> r,juce::String text,float v){
  g.setColour(muted);g.setFont(juce::Font(juce::FontOptions(11.f)));g.drawText(text,r.removeFromLeft(25),juce::Justification::centredLeft);
  auto db=(v>0.f&&std::isfinite(v)?std::max(-60.f,20.f*std::log10(v)):-60.f);auto n=juce::jlimit(0.f,1.f,(db+60.f)/60.f);
  auto value=r.removeFromRight(35);g.drawText(db<=-60?"-inf":juce::String(db,1),value,juce::Justification::centredRight);
  auto bar=r.reduced(2,4);g.setColour(ink);g.fillRoundedRectangle(bar.toFloat(),2);
  const int count=16,step=std::max(1,bar.getWidth()/count);for(int i=0;i<count;++i){g.setColour(float(i+1)/count<=n?(v>=1.f&&i>=14?juce::Colour(0xffe6a069):green):border);g.fillRect(bar.getX()+i*step,bar.getY(),std::max(1,step-2),bar.getHeight());}
 }
public:
 std::function<void(int)> onModel,onBank,onBegin,onEnd;
 std::function<void(int,int)> onValue;
 std::function<void()> onImport,onExport;
 Rack(){
  setLookAndFeel(&look);
  const char* names[]={"CLASSIC","MODERN","BANK"};
  for(int i=0;i<3;++i){modes[i].setButtonText(names[i]);addAndMakeVisible(modes[i]);modes[i].onClick=[this,i]{if(onModel)onModel(i);};}
  for(int i=0;i<8;++i){banks[i].setButtonText(juce::String(i+1).paddedLeft('0',2));addChildComponent(banks[i]);banks[i].onClick=[this,i]{if(onBank)onBank(i);};}
  for(auto* b:{&importButton,&exportButton})addAndMakeVisible(*b);
  importButton.onClick=[this]{if(onImport)onImport();};exportButton.onClick=[this]{if(onExport)onExport();};
  status.setColour(juce::Label::textColourId,muted);status.setFont(juce::Font(juce::FontOptions(12.f)));addAndMakeVisible(status);
  viewport.setViewedComponent(&content,false);viewport.setScrollBarsShown(true,false);addAndMakeVisible(viewport);
  setMode(0,0);
 }
 ~Rack()override{viewport.setViewedComponent(nullptr,false);groups.clear();setLookAndFeel(nullptr);}
 void setMode(int m,int b){mode=juce::jlimit(0,2,m);bank=juce::jlimit(0,7,b);for(int i=0;i<3;++i)modes[i].setToggleState(i==mode,juce::dontSendNotification);for(int i=0;i<8;++i){banks[i].setVisible(mode==2);banks[i].setToggleState(i==bank,juce::dontSendNotification);}resized();repaint();}
 void setSections(std::vector<Section> sections,bool resetScroll=true){
  groups.clear();for(auto& s:sections){auto group=std::make_unique<Group>(s.title);for(auto& spec:s.controls){auto cell=std::make_unique<Cell>(spec);auto* ptr=cell.get();const int key=spec.key;
   ptr->slider.onValueChange=[this,ptr,key]{if(onValue)onValue(key,int(ptr->slider.getValue()));};
   ptr->slider.onDragStart=[this,key]{if(onBegin)onBegin(key);};ptr->slider.onDragEnd=[this,key]{if(onEnd)onEnd(key);};
   group->addAndMakeVisible(*cell);group->cells.push_back(std::move(cell));}content.addAndMakeVisible(*group);groups.push_back(std::move(group));}
  layoutGroups();if(resetScroll)viewport.setViewPosition(0,0);repaint();
 }
 void updateValue(int key,int value){for(auto& g:groups)for(auto& c:g->cells)if(c->spec.key==key&&!c->slider.isMouseButtonDown()&&!c->slider.hasKeyboardFocus(true))c->slider.setValue(value,juce::dontSendNotification);}
 void setStatus(juce::String s){status.setText(s,juce::dontSendNotification);}
 void setPreset(juce::String s){preset=std::move(s);repaint(0,0,getWidth(),56);}
 void setMeter(float a,float b,bool e){in=a;out=b;error=e;repaint(0,0,getWidth(),74);}
 int controlCount()const{int n=0;for(auto& g:groups)n+=int(g->cells.size());return n;}
 bool geometryValid()const{for(auto& g:groups)for(auto& c:g->cells)if(c->getWidth()<56||c->getHeight()<85||!g->getLocalBounds().contains(c->getBounds()))return false;return true;}
 bool testChange(int key,int value){for(auto& g:groups)for(auto& c:g->cells)if(c->spec.key==key){c->slider.setValue(value,juce::sendNotificationSync);return true;}return false;}
 juce::Component* importTarget(){return &importButton;}
 void paint(juce::Graphics& g)override{
  g.fillAll(ink);g.setColour(juce::Colour(0xff171d19));g.fillRect(0,0,getWidth(),56);
  g.setColour(pale);g.setFont(juce::Font(juce::FontOptions(25.f).withStyle("Bold")));g.drawText("SKELLAY",12,3,130,31,juce::Justification::centredLeft);
  g.setColour(green);g.fillRoundedRectangle(13,47,28,2,1);
  g.setColour(muted);g.setFont(juce::Font(juce::FontOptions(10.f)));g.drawText("DELAY  /  0.9",13,32,130,13,juce::Justification::centredLeft);
  const int mx=getWidth()-174;meter(g,{mx,7,160,20},"IN",in);meter(g,{mx,29,160,20},"OUT",out);
  g.setColour(error?juce::Colour(0xffe6a069):muted);g.setFont(juce::Font(juce::FontOptions(12.f)));g.drawFittedText(error?"ENGINE ERROR":preset,148,8,std::max(0,mx-158),40,juce::Justification::centredLeft,2);
  g.setColour(border);g.drawHorizontalLine(55,0,float(getWidth()));
 }
 void resized()override{
  auto r=getLocalBounds().reduced(9);r.removeFromTop(51);
  auto tabs=r.removeFromTop(29);for(auto& b:modes)b.setBounds(tabs.removeFromLeft(76).reduced(2));
  exportButton.setBounds(tabs.removeFromRight(90).reduced(2));importButton.setBounds(tabs.removeFromRight(98).reduced(2));r.removeFromTop(7);
  if(mode==2){auto row=r.removeFromTop(29);int w=row.getWidth()/8;for(auto& b:banks)b.setBounds(row.removeFromLeft(w).reduced(2));r.removeFromTop(6);}
  status.setBounds(r.removeFromBottom(22));viewport.setBounds(r);layoutGroups();
 }
};
}
