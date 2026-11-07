#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr int baseW=960,baseH=600;
const juce::Colour dark=juce::Colour::fromRGB(6,9,10);
const juce::Colour panel=juce::Colour::fromRGB(20,26,29);
const juce::Colour bone=juce::Colour::fromRGB(232,230,218);
const juce::Colour muted=juce::Colour::fromRGB(140,158,147);
const char* modes[]={"CHORUS","FLANGER","VOWEL","REVERSE","ORBIT","TUNED"};
const char* lanes[]={"DEPTH","SPEED","DELAY","SPREAD","CROSS","DRY","WET"};
const char* knobNames[][4]{
    {"DETUNE","WIDTH","WARMTH","ENSEMBLE"},
    {"THRU ZERO*","FB COLOR","MANUAL","ENV FOLLOW"},
    {"VOWEL","GENDER","SHARPNESS","COMB"},
    {"GRAIN","OVERLAP","SMEAR","SHIMMER*"},
    {"RADIUS","TILT","AIR","SPIN"},
    {"TRACK","GLIDE*","HARMONIC","RESONANCE*"}
};
const juce::Colour tints[]{
    juce::Colour::fromRGB(47,217,255),juce::Colour::fromRGB(255,176,46),
    juce::Colour::fromRGB(166,107,255),juce::Colour::fromRGB(255,47,179),
    juce::Colour::fromRGB(77,139,255),juce::Colour::fromRGB(214,255,58)
};
const char* presets[]{
    "INIT","Lush Choir Bus","Vocal Double Wash","Crypt Ensemble",
    "Jet Skeleton","Through-Zero Whoosh","Grave Ring",
    "Talking Bones","Ghost Choir Sweep","Wah Reaper",
    "Bloom Backwards","Spectral Tide","Rage Swell",
    "Skull Carousel","Haunted Merry-Go-Round","Slow Moon",
    "Key-Locked Jet","Pitch Ghost","Harmonic Chain"
};
void styleSlider(juce::Slider& slider,bool vertical){
    slider.setSliderStyle(vertical?juce::Slider::LinearVertical:
                             juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
    slider.setMouseDragSensitivity(vertical?180:145);
    slider.setScrollWheelEnabled(true);
    slider.setDoubleClickReturnValue(true,.5);
}
void label(juce::Label& text,const juce::String& content){
    text.setText(content,juce::dontSendNotification);
    text.setJustificationType(juce::Justification::centred);
    text.setFont(juce::Font(11.5f));
    text.setColour(juce::Label::textColourId,muted);
}
void panelRect(juce::Graphics&g,float x,float y,float w,float h){
    g.setColour(panel);g.fillRoundedRectangle(x,y,w,h,10.f);
    g.setColour(juce::Colour::fromRGB(45,54,57));g.drawRoundedRectangle(x,y,w,h,10.f,1.f);
}
}
void FlangLookAndFeel::drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,
                                        float pos,float,float,juce::Slider::SliderStyle,juce::Slider&) {
    auto well=juce::Rectangle<float>(float(x+4),float(y+4),float(w-8),float(h-8));
    g.setColour(juce::Colour::fromRGB(11,17,20));g.fillRoundedRectangle(well,6.f);
    g.setColour(juce::Colour::fromRGB(38,48,50));g.drawRoundedRectangle(well,6.f,1.f);
    const float mid=well.getCentreX();
    for(int k=0;k<19;++k){
        const float yy=well.getY()+8.f+k*(well.getHeight()-16.f)/18.f;
        g.setColour(juce::Colour::fromRGB(71,83,78).withAlpha(.5f));
        g.drawLine(mid-18,yy,mid+18,yy,.6f);
    }
    const float capY=std::clamp(pos,well.getY()+14.f,well.getBottom()-14.f);
    g.setColour(accent.withAlpha(.6f));g.drawLine(mid,capY,mid,well.getBottom()-4.f,2.f);
    juce::Path cap;
    cap.startNewSubPath(mid-22,capY-13);
    cap.lineTo(mid+22,capY-13);
    cap.lineTo(mid+19,capY+13);
    cap.lineTo(mid-19,capY+13);
    cap.closeSubPath();
    g.setColour(juce::Colour::fromRGB(53,65,65));g.fillPath(cap);
    g.setColour(juce::Colour::fromRGB(131,154,145));g.strokePath(cap,juce::PathStrokeType(1.f));
    g.setColour(juce::Colour::fromRGB(9,13,15));g.fillRect(mid-13.f,capY-1.5f,26.f,3.f);
    g.setColour(accent);g.fillRect(mid-10.f,capY-.7f,20.f,1.4f);
    g.setColour(juce::Colour::fromRGB(173,183,165));g.fillEllipse(mid-3.f,capY-9.f,6.f,6.f);
}
void FlangLookAndFeel::drawRotarySlider(juce::Graphics&g,int x,int y,int w,int h,
                                        float position,float start,float end,juce::Slider&) {
    const auto bounds=juce::Rectangle<float>(float(x),float(y),float(w),float(h));
    const float cx=bounds.getCentreX(),cy=bounds.getCentreY();
    const float radius=std::min(w,h)*.37f;
    g.setColour(juce::Colour::fromRGB(9,14,16));g.fillEllipse(cx-radius-4,cy-radius-4,2*radius+8,2*radius+8);
    g.setColour(juce::Colour::fromRGB(56,72,70));g.drawEllipse(cx-radius-4,cy-radius-4,2*radius+8,2*radius+8,2.f);
    juce::Path arc;
    arc.addCentredArc(cx,cy,radius+4,radius+4,0,start,start+(end-start)*position,true);
    g.setColour(accent);g.strokePath(arc,juce::PathStrokeType(2.5f));
    juce::ColourGradient grad(juce::Colour::fromRGB(79,94,93),cx-radius,cy-radius,
                              juce::Colour::fromRGB(25,33,37),cx+radius,cy+radius,false);
    g.setGradientFill(grad);g.fillEllipse(cx-radius,cy-radius,2*radius,2*radius);
    const float angle=start+(end-start)*position;
    g.setColour(bone);
    g.drawLine(cx,cy,cx+std::sin(angle)*radius*.74f,cy-std::cos(angle)*radius*.74f,3.f);
    g.setColour(juce::Colour::fromRGB(8,12,12));g.fillEllipse(cx-3,cy-3,6,6);
}
void FlangLookAndFeel::drawButtonBackground(juce::Graphics&g,juce::Button&button,
                                             const juce::Colour&,bool hover,bool down) {
    const auto b=button.getLocalBounds().toFloat().reduced(1.f);
    g.setColour(down||button.getToggleState()?accent.withAlpha(.23f):
                hover?panel.brighter(.16f):panel);
    g.fillRoundedRectangle(b,5.f);
    g.setColour((down||button.getToggleState())?accent:juce::Colour::fromRGB(61,76,69));
    g.drawRoundedRectangle(b,5.f,1.f);
}
SkellFlangAudioProcessorEditor::SkellFlangAudioProcessorEditor(SkellFlangAudioProcessor&p)
: AudioProcessorEditor(&p),processor(p) {
    setLookAndFeel(&skin);
    for(int i=0;i<7;++i){
        styleSlider(faders[i],true);addAndMakeVisible(faders[i]);
        label(faderLabels[i],lanes[i]);addAndMakeVisible(faderLabels[i]);
        faderLinks[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            p.parameters,juce::StringArray{"depth","speed","delay","spread","cross","dry","wet"}[i],faders[i]);
    }
    styleSlider(order,true);order.setRange(1,8,1);addAndMakeVisible(order);
    orderLink=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.parameters,"order",order);
    for(int i=0;i<7;++i) {
        styleSlider(knobs[i],false);addAndMakeVisible(knobs[i]);
        label(knobLabels[i],i==0?"FEEDBACK":i==1?"TONE":i==2?"OUTPUT":"MODE");
        addAndMakeVisible(knobLabels[i]);
        const auto id=i==0?"feedback":i==1?"tone":i==2?"output":
                      i==3?"extra0":i==4?"extra1":i==5?"extra2":"extra3";
        knobLinks[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            p.parameters,id,knobs[i]);
    }
    for(int i=0;i<6;++i){
        tabs[i].setButtonText(modes[i]);addAndMakeVisible(tabs[i]);
        tabs[i].onClick=[this,i]{
            if(auto*param=processor.parameters.getParameter("mode")){
                param->beginChangeGesture();
                param->setValueNotifyingHost(param->convertTo0to1(float(i)));
                param->endChangeGesture();
            }
        };
    }
    for(int i=0;i<6;++i)target.addItem(modes[i],i+1);
    for(int i=0;i<5;++i)shape.addItem(juce::StringArray{"Sine","Triangle","Saw+","Saw-","Square"}[i],i+1);
    for(int i=0;i<12;++i)root.addItem(juce::StringArray{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}[i],i+1);
    for(int i=0;i<4;++i)scale.addItem(juce::StringArray{"Major","Minor","Pentatonic","Chromatic"}[i],i+1);
    tuneSource.addItem("FOLLOW",1);tuneSource.addItem("KEY",2);
    juce::ComboBox* boxes[]{&target,&shape,&root,&scale,&tuneSource};
    const char* names[]{"morphTarget","lfoShape","tnRoot","tnScale","tnSource"};
    for(int i=0;i<5;++i) {
        addAndMakeVisible(*boxes[i]);
        comboLinks[i]=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            p.parameters,names[i],*boxes[i]);
    }
    morph.setSliderStyle(juce::Slider::LinearHorizontal);
    morph.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
    addAndMakeVisible(morph);
    morphLink=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.parameters,"morph",morph);
    addAndMakeVisible(power);addAndMakeVisible(sync);
    powerLink=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.parameters,"power",power);
    syncLink=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.parameters,"lfoSync",sync);
    addAndMakeVisible(preset);addAndMakeVisible(settings);
    preset.onClick=[this]{
        juce::PopupMenu menu;
        for(int i=0;i<19;++i)menu.addItem(i+1,presets[i]);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&preset),
            [safe=juce::Component::SafePointer<SkellFlangAudioProcessorEditor>(this)](int selected){
                if(safe&&selected>0)safe->processor.applyFactoryPreset(selected-1);
            });
    };
    settings.onClick=[this]{
        juce::PopupMenu menu;
        menu.addItem(1,"UI Scale 75%");menu.addItem(2,"UI Scale 100%");
        menu.addItem(3,"UI Scale 125%");menu.addItem(4,"UI Scale 150%");
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&settings),
            [safe=juce::Component::SafePointer<SkellFlangAudioProcessorEditor>(this)](int selection){
                if(safe&&selection>0) {
                    const float factor=.5f+.25f*selection;
                    safe->setSize(int(baseW*factor),int(baseH*factor));
                }
            });
    };
    setResizable(true,true);
    if(auto* c=getConstrainer())c->setFixedAspectRatio(double(baseW)/baseH);
    setResizeLimits(720,450,1440,900);
    setSize(baseW,baseH);
    updateMode();
    startTimerHz(30);
}
SkellFlangAudioProcessorEditor::~SkellFlangAudioProcessorEditor(){
    stopTimer();setLookAndFeel(nullptr);
}
void SkellFlangAudioProcessorEditor::resized() {
    const float s=getWidth()/float(baseW);
    auto place=[s](juce::Component&c,int x,int y,int w,int h){
        c.setBounds(int(x*s),int(y*s),int(w*s),int(h*s));
    };
    place(settings,12,9,60,31);place(preset,312,8,390,33);place(power,851,9,94,32);
    for(int i=0;i<6;++i)place(tabs[i],104+i*100,59,94,39);
    place(target,163,102,128,26);place(morph,313,105,370,24);
    place(order,23,147,60,258);
    for(int i=0;i<7;++i){
        place(faders[i],101+i*84,143,73,264);
        place(faderLabels[i],98+i*84,412,80,26);
    }
    place(shape,722,324,122,30);place(sync,851,324,91,30);
    place(root,722,324,105,30);place(scale,831,324,110,30);
    place(tuneSource,722,363,124,30);
    for(int i=0;i<7;++i){
        const int xx=i<2?24+i*100:i==2?740:242+(i-3)*119;
        place(knobs[i],xx,492,70,66);
        place(knobLabels[i],xx-15,553,101,21);
    }
}
void SkellFlangAudioProcessorEditor::updateMode() {
    const auto* raw=processor.parameters.getRawParameterValue("mode");
    const int mode=std::clamp(raw?int(raw->load()):0,0,5);
    if(mode==shownMode)return;
    shownMode=mode;
    skin.accent=tints[mode];
    for(int i=0;i<6;++i)tabs[i].setToggleState(i==mode,juce::dontSendNotification);
    for(int i=0;i<4;++i)label(knobLabels[3+i],knobNames[mode][i]);
    shape.setVisible(mode!=5);sync.setVisible(mode!=5);
    root.setVisible(mode==5);scale.setVisible(mode==5);tuneSource.setVisible(mode==5);
    repaint();
}
void SkellFlangAudioProcessorEditor::timerCallback(){
    updateMode();
    const juce::String name=processor.parameters.state.getProperty("presetName","INIT").toString();
    if(preset.getButtonText()!=name)preset.setButtonText(name);
    repaint();
}
void SkellFlangAudioProcessorEditor::paint(juce::Graphics& g) {
    const float s=getWidth()/float(baseW);
    g.fillAll(dark);
    g.saveState();g.addTransform(juce::AffineTransform::scale(s));
    juce::ColourGradient bg(juce::Colour::fromRGB(16,22,24),0,50,
                            juce::Colour::fromRGB(4,7,8),960,600,true);
    g.setGradientFill(bg);g.fillRect(0,0,baseW,baseH);
    g.setColour(juce::Colour::fromRGB(7,10,11));g.fillRect(0,0,960,48);
    panelRect(g,13,129,78,316);panelRect(g,95,129,604,316);
    panelRect(g,710,128,239,315);panelRect(g,14,463,820,117);
    g.setColour(skin.accent);g.fillRoundedRectangle(18.f,10.f,32.f,30.f,6.f);
    g.setColour(dark);
    g.fillEllipse(26.f,17.f,16.f,13.f);
    g.fillRect(29,28,10,5);
    g.setFont(juce::Font(25.f,juce::Font::bold));g.setColour(bone);
    g.drawText("SKELL-FLANG",58,5,244,29,juce::Justification::centredLeft);
    g.setColour(skin.accent);g.setFont(juce::Font(10.f));
    g.drawText("BEND THE BONES",59,32,220,13,juce::Justification::left);
    g.setColour(muted);g.setFont(juce::Font(11.5f));
    g.drawText("ORD",19,420,67,19,juce::Justification::centred);
    g.drawText("MORPH",99,106,60,20,juce::Justification::centred);
    g.drawText("LIVE SIGNAL",727,143,140,18,juce::Justification::left);
    g.drawText(shownMode==5?"KEY / TUNING":"LFO / TEMPO",723,302,181,18,juce::Justification::left);
    g.drawText("SIGNAL",853,473,82,19,juce::Justification::centred);
    const auto px=processor.meters.peaks[0].load(),py=processor.meters.peaks[1].load();
    g.setColour(juce::Colour::fromRGB(9,14,17));g.fillRoundedRectangle(722.f,163.f,215.f,128.f,5.f);
    g.setColour(juce::Colour::fromRGB(47,65,59));g.drawLine(724.f,226.f,935.f,226.f,.8f);
    juce::Path scope;
    for(int i=0;i<64;++i) {
        const float v=processor.meters.scope[size_t(i)].load(std::memory_order_relaxed);
        const float x=726.f+float(i)*3.25f,y=226.f-std::clamp(v,-1.f,1.f)*51.f;
        if(i==0)scope.startNewSubPath(x,y);else scope.lineTo(x,y);
    }
    g.setColour(skin.accent.withAlpha(.88f));g.strokePath(scope,juce::PathStrokeType(1.4f));
    for(int i=0;i<2;++i) {
        g.setColour(juce::Colour::fromRGB(13,22,19));g.fillRoundedRectangle(float(875+i*28),495.f,17.f,69.f,2.f);
        const float peak=std::clamp(i==0?px:py,0.f,1.f);
        g.setColour(peak>.94f?juce::Colours::orange:skin.accent);
        g.fillRoundedRectangle(float(875+i*28),564.f-63.f*peak,17.f,63.f*peak,2.f);
    }
    g.setColour(muted);g.setFont(juce::Font(11.f));
    g.drawText("HOST "+juce::String(processor.meters.bpm.load(),1)+" BPM   •   PDC 0   •   CPU "+
               juce::String(processor.meters.cpu.load(),1)+"%   •   "+modes[std::max(0,shownMode)],
               22,577,855,22,juce::Justification::left);
    g.setColour(skin.accent.withAlpha(.65f));g.fillRect(0,599,960,1);
    g.restoreState();
}
