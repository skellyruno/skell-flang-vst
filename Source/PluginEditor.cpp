#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr int baseW=720,baseH=450;
const juce::Colour dark=juce::Colour::fromRGB(7,12,14);
const juce::Colour panel=juce::Colour::fromRGB(17,26,29);
const juce::Colour bone=juce::Colour::fromRGB(226,239,230);
const juce::Colour muted=juce::Colour::fromRGB(143,169,164);
const char* modes[]={"CHORUS","FLANGER","VOWEL","REVERSE","ORBIT","TUNED"};
const char* lanes[]={"DEPTH","SPEED","DELAY","SPREAD","CROSS","DRY","WET"};
const char* knobNames[][4]{
    {"DETUNE","WIDTH","WARMTH","ENSEMBLE"},
    {"SWEEP RANGE","FB COLOR","MANUAL","ENV FOLLOW"},
    {"VOWEL","GENDER","SHARPNESS","COMB"},
    {"GRAIN","OVERLAP","SMEAR","SHIMMER"},
    {"RADIUS","TILT","AIR","SPIN"},
    {"TRACK","GLIDE","HARMONIC","RESONANCE"}
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
    // Each control restores its own parameter default below, never a hardcoded 0.5.
}
void label(juce::Label& text,const juce::String& content){
    text.setText(content,juce::dontSendNotification);
    text.setJustificationType(juce::Justification::centred);
    text.setFont(juce::Font(12.5f));
    text.setColour(juce::Label::textColourId,muted);
}
void panelRect(juce::Graphics&g,float x,float y,float w,float h){
    g.setColour(panel);g.fillRoundedRectangle(x,y,w,h,9.f);
    g.setColour(juce::Colour::fromRGB(45,54,57));g.drawRoundedRectangle(x,y,w,h,9.f,.9f);
}
}

void FlangLookAndFeel::drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,
                                       float pos,float,float,juce::Slider::SliderStyle,juce::Slider&) {
    const auto well=juce::Rectangle<float>(float(x+4),float(y+2),float(w-8),float(h-4));
    const float mid=well.getCentreX();
    g.setColour(juce::Colour::fromRGB(8,17,20));g.fillRoundedRectangle(well,5.f);
    g.setColour(juce::Colour::fromRGB(35,55,55));g.drawRoundedRectangle(well,5.f,.8f);
    for(int k=0;k<13;++k) {
        const float yy=well.getY()+12.f+k*(well.getHeight()-24.f)/12.f;
        g.setColour(juce::Colour::fromRGB(59,86,83).withAlpha(.45f));
        g.drawLine(mid-10.f,yy,mid+10.f,yy,.7f);
    }
    const float capY=std::clamp(pos,well.getY()+11.f,well.getBottom()-11.f);
    g.setColour(accent.withAlpha(.48f));g.drawLine(mid,well.getBottom()-10.f,mid,capY,2.f);
    const float half=std::min(16.f,well.getWidth()*.45f);
    const juce::Rectangle<float> thumb(mid-half,capY-9.f,half*2.f,18.f);
    g.setColour(juce::Colour::fromRGB(30,49,52));g.fillRoundedRectangle(thumb,4.f);
    g.setColour(juce::Colour::fromRGB(117,160,151));g.drawRoundedRectangle(thumb,4.f,1.f);
    // An original split-bone indicator instead of a large metallic slider cap.
    g.setColour(juce::Colour::fromRGB(197,217,207));g.fillEllipse(mid-2.f,capY-4.5f,4.f,4.f);
    g.setColour(accent);g.fillRoundedRectangle(mid-9.f,capY+2.f,18.f,1.8f,.9f);
}

void FlangLookAndFeel::drawRotarySlider(juce::Graphics&g,int x,int y,int w,int h,
                                        float position,float start,float end,juce::Slider&) {
    const float cx=float(x)+w*.5f,cy=float(y)+h*.5f;
    const float radius=std::min(w,h)*.32f;
    g.setColour(juce::Colour::fromRGB(6,14,16));
    g.fillEllipse(cx-radius-3,cy-radius-3,2*radius+6,2*radius+6);
    g.setColour(juce::Colour::fromRGB(60,89,85));
    g.drawEllipse(cx-radius-3,cy-radius-3,2*radius+6,2*radius+6,1.4f);
    juce::Path arc;
    arc.addCentredArc(cx,cy,radius+3,radius+3,0,start,start+(end-start)*position,true);
    g.setColour(accent);g.strokePath(arc,juce::PathStrokeType(2.3f));
    g.setColour(juce::Colour::fromRGB(37,55,57));
    g.fillEllipse(cx-radius,cy-radius,2*radius,2*radius);
    g.setColour(juce::Colour::fromRGB(92,125,119));
    g.drawEllipse(cx-radius,cy-radius,2*radius,2*radius,.8f);
    const float angle=start+(end-start)*position;
    g.setColour(bone);
    g.drawLine(cx,cy,cx+std::sin(angle)*radius*.72f,cy-std::cos(angle)*radius*.72f,2.8f);
    g.setColour(juce::Colour::fromRGB(8,14,15));
    g.fillEllipse(cx-2.f,cy-2.f,4.f,4.f);
}

void FlangLookAndFeel::drawButtonBackground(juce::Graphics&g,juce::Button&button,
                                             const juce::Colour&,bool hover,bool down) {
    const auto b=button.getLocalBounds().toFloat().reduced(1.f);
    const bool selected=button.getToggleState();
    g.setColour(selected?accent.withAlpha(.15f):
                hover?juce::Colour::fromRGB(25,43,44):juce::Colour::fromRGB(14,24,27));
    g.fillRoundedRectangle(b,5.f);
    g.setColour(selected?accent.withAlpha(.86f):juce::Colour::fromRGB(40,67,64));
    g.drawRoundedRectangle(b,5.f,selected?1.2f:.8f);
    if(down){g.setColour(accent.withAlpha(.20f));g.fillRoundedRectangle(b.reduced(2.f),4.f);}
}
SkellFlangAudioProcessorEditor::SkellFlangAudioProcessorEditor(SkellFlangAudioProcessor&p)
: AudioProcessorEditor(&p),processor(p) {
    setLookAndFeel(&skin);
    // Accessible text contrast and consistent dark dropdowns, even in FL Studio.
    for(auto* dropdown : {&target,&shape,&root,&scale,&tuneSource}) {
        dropdown->setColour(juce::ComboBox::backgroundColourId,juce::Colour::fromRGB(13,29,31));
        dropdown->setColour(juce::ComboBox::textColourId,bone);
        dropdown->setColour(juce::ComboBox::outlineColourId,juce::Colour::fromRGB(52,80,74));
        dropdown->setColour(juce::ComboBox::arrowColourId,juce::Colour::fromRGB(128,208,182));
    }
    setColour(juce::PopupMenu::backgroundColourId,juce::Colour::fromRGB(15,27,29));
    setColour(juce::PopupMenu::textColourId,bone);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour::fromRGB(45,81,76));
    setColour(juce::PopupMenu::highlightedTextColourId,bone);
    page=juce::jlimit(0,2,int(p.parameters.state.getProperty("uiPage",0)));
    for(int i=0;i<7;++i){
        styleSlider(faders[i],true);addAndMakeVisible(faders[i]);
        const auto id=juce::StringArray{"depth","speed","delay","spread","cross","dry","wet"}[i];
        if(auto* param=p.parameters.getParameter(id))
            faders[i].setDoubleClickReturnValue(true,param->convertFrom0to1(param->getDefaultValue()));
        label(faderLabels[i],lanes[i]);addAndMakeVisible(faderLabels[i]);
        faderLinks[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            p.parameters,juce::StringArray{"depth","speed","delay","spread","cross","dry","wet"}[i],faders[i]);
    }
    styleSlider(order,true);order.setRange(1,8,1);order.setDoubleClickReturnValue(true,3);addAndMakeVisible(order);
    orderLink=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.parameters,"order",order);
    for(int i=0;i<7;++i) {
        styleSlider(knobs[i],false);addAndMakeVisible(knobs[i]);
        label(knobLabels[i],i==0?"FEEDBACK":i==1?"TONE":i==2?"OUTPUT":"MODE");
        addAndMakeVisible(knobLabels[i]);
        const auto id=i==0?"feedback":i==1?"tone":i==2?"output":
                      i==3?"extra0":i==4?"extra1":i==5?"extra2":"extra3";
        if(auto* param=p.parameters.getParameter(id))
            knobs[i].setDoubleClickReturnValue(true,param->convertFrom0to1(param->getDefaultValue()));
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
    const char* pageNames[]{"CORE","COLOR","MOTION"};
    for(int i=0;i<3;++i) {
        pages[i].setButtonText(pageNames[i]);addAndMakeVisible(pages[i]);
        pages[i].onClick=[this,i] {
            page=i;
            processor.parameters.state.setProperty("uiPage",page,nullptr);
            updatePage();
            repaint();
        };
    }
    const juce::Component::SafePointer<SkellFlangAudioProcessorEditor> safeEditor(this);
    preset.onClick=[this,safeEditor]{
        juce::PopupMenu menu;
        for(int i=0;i<19;++i)menu.addItem(i+1,presets[i]);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&preset),
            [safeEditor](int selected){
                if(safeEditor&&selected>0)safeEditor->processor.applyFactoryPreset(selected-1);
            });
    };
    settings.onClick=[this,safeEditor]{
        juce::PopupMenu menu;
        menu.addItem(1,"Compact (540 x 338)");menu.addItem(2,"Comfortable (720 x 450)");
        menu.addItem(3,"Large (900 x 563)");menu.addItem(4,"Studio (1080 x 675)");
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&settings),
            [safeEditor](int selection){
                if(safeEditor&&selection>0) {
                    const int widths[]{540,720,900,1080};
                    const int w=widths[selection-1];
                    safeEditor->processor.parameters.state.setProperty("uiWidth",w,nullptr);
                    safeEditor->setSize(w,juce::roundToInt(w*double(baseH)/baseW));
                }
            });
    };
    setResizable(true,true);
    if(auto* c=getConstrainer())c->setFixedAspectRatio(double(baseW)/baseH);
    setResizeLimits(540,338,1080,675);
    const int savedWidth=juce::jlimit(540,1080,int(p.parameters.state.getProperty("uiWidth",540)));
    setSize(savedWidth,juce::roundToInt(savedWidth*double(baseH)/baseW));
    updateMode();
    startTimerHz(24);
}
SkellFlangAudioProcessorEditor::~SkellFlangAudioProcessorEditor(){
    stopTimer();setLookAndFeel(nullptr);
}

void SkellFlangAudioProcessorEditor::resized() {
    const float s=getWidth()/float(baseW);
    // JUCE child components are independent of the graphics scale. Resize text
    // explicitly or compact mode inherits oversized fonts.
    const auto fontSize=juce::jlimit(9.2f,12.5f,12.5f*s);
    for(auto& text:faderLabels)text.setFont(juce::Font(fontSize));
    for(auto& text:knobLabels)text.setFont(juce::Font(fontSize));
    auto place=[s](juce::Component&c,int x,int y,int w,int h) {
        c.setBounds(juce::roundToInt(x*s),juce::roundToInt(y*s),
                    juce::roundToInt(w*s),juce::roundToInt(h*s));
    };
    place(settings,14,12,51,29);place(preset,338,12,231,29);place(power,601,12,104,29);
    for(int i=0;i<6;++i)place(tabs[i],16+i*115,62,109,36);
    place(order,25,151,52,158);
    for(int i=0;i<7;++i) {
        place(faders[i],93+i*62,151,49,158);
        place(faderLabels[i],88+i*62,307,59,21);
    }
    for(int i=0;i<3;++i)place(pages[i],27,354+i*23,81,22);
    for(int i=0;i<7;++i) {
        const int xx=i<3?(247+i*151):(236+(i-3)*111);
        place(knobs[i],xx,351,59,59);
        place(knobLabels[i],xx-23,406,105,17);
    }
    place(target,249,350,130,30);place(morph,395,350,281,29);
    place(shape,249,386,125,29);place(sync,388,385,102,30);
    place(root,249,386,91,28);place(scale,348,386,130,28);
    place(tuneSource,492,386,131,28);
    updatePage();
}
void SkellFlangAudioProcessorEditor::updatePage() {
    for(int i=0;i<3;++i)pages[i].setToggleState(i==page,juce::dontSendNotification);
    for(int i=0;i<7;++i) {
        const bool show=page==(i<3?0:1);
        knobs[i].setVisible(show);knobLabels[i].setVisible(show);
    }
    const bool motion=page==2;
    target.setVisible(motion);morph.setVisible(motion);
    shape.setVisible(motion&&shownMode!=5);
    sync.setVisible(motion&&shownMode!=5);
    root.setVisible(motion&&shownMode==5);
    scale.setVisible(motion&&shownMode==5);
    tuneSource.setVisible(motion&&shownMode==5);
}
void SkellFlangAudioProcessorEditor::updateMode() {
    const auto* raw=processor.parameters.getRawParameterValue("mode");
    const int mode=std::clamp(raw?int(raw->load()):0,0,5);
    if(mode==shownMode)return;
    shownMode=mode;
    skin.accent=juce::Colour::fromRGB(107,226,193);
    for(int i=0;i<6;++i)tabs[i].setToggleState(i==mode,juce::dontSendNotification);
    for(int i=0;i<4;++i)label(knobLabels[3+i],knobNames[mode][i]);
    updatePage();
    repaint();
}
void SkellFlangAudioProcessorEditor::timerCallback(){
    updateMode();
    const juce::String name=processor.parameters.state.getProperty("presetName","INIT").toString();
    if(preset.getButtonText()!=name)preset.setButtonText(name);
    const float scaleFactor=getWidth()/float(baseW);
    repaint(juce::roundToInt(536*scaleFactor),juce::roundToInt(111*scaleFactor),
            juce::roundToInt(171*scaleFactor),juce::roundToInt(221*scaleFactor));
    repaint(0,juce::roundToInt(427*scaleFactor),getWidth(),
            std::max(1,getHeight()-juce::roundToInt(427*scaleFactor)));
}
void SkellFlangAudioProcessorEditor::paint(juce::Graphics& g) {
    const float s=getWidth()/float(baseW);
    g.fillAll(dark);
    g.saveState();
    g.addTransform(juce::AffineTransform::scale(s));

    // Asymmetric "instrument rack" layout: one clean fader deck, one monitor,
    // and context-sensitive controls below. No asset scaling, per-frame blurs,
    // duplicated container frames or continuous full-window gradients.
    g.setColour(juce::Colour::fromRGB(9,17,19));
    g.fillRect(0,0,baseW,52);
    g.setColour(juce::Colour::fromRGB(30,55,52));
    g.drawLine(14.f,51.f,706.f,51.f,.85f);
    // A fine accent under the active mode, rather than six glowing boxes.
    if(shownMode>=0 && shownMode<6) {
        g.setColour(tints[shownMode].withAlpha(.82f));
        g.fillRoundedRectangle(28.f+115.f*shownMode,101.f,84.f,2.f,1.f);
    }

    panelRect(g,14,111,516,221);
    panelRect(g,538,111,169,221);
    panelRect(g,14,342,693,82);

    // The rib emblem is drawn, not a reused skull logo or a PNG.
    const auto glow=skin.accent;
    g.setColour(glow);
    g.drawLine(82.f,18.f,82.f,38.f,1.8f);
    for(int i=0;i<3;++i) {
        const float yy=21.f+float(i)*5.f;
        g.drawLine(82.f,yy,74.f,yy-2.f,1.25f);
        g.drawLine(82.f,yy,90.f,yy-2.f,1.25f);
    }
    g.setColour(bone);g.setFont(juce::Font(20.f,juce::Font::bold));
    g.drawText("SKELL-FLANG",100,8,220,29,juce::Justification::centredLeft);
    g.setColour(glow.withAlpha(.9f));g.setFont(juce::Font(10.5f));
    g.drawText("BEND THE BONES  /  MODULATION ENGINE",102,34,225,12,
               juce::Justification::centredLeft);

    g.setColour(muted);g.setFont(juce::Font(10.5f));
    g.drawText("01  /  CONTROL DECK",27,119,170,18,juce::Justification::left);
    g.drawText("ORD",24,307,54,19,juce::Justification::centred);
    g.drawText("02  /  SIGNAL",550,119,143,18,juce::Justification::left);

    // Small technical divider motifs make the layout distinctive, not busy.
    g.setColour(glow.withAlpha(.50f));
    g.fillRoundedRectangle(27.f,139.f,33.f,1.3f,.5f);
    g.fillRoundedRectangle(550.f,139.f,27.f,1.3f,.5f);

    const juce::Rectangle<float> scope(552.f,153.f,141.f,112.f);
    g.setColour(juce::Colour::fromRGB(8,17,19));
    g.fillRoundedRectangle(scope,5.f);
    g.setColour(juce::Colour::fromRGB(38,68,63));
    g.drawRoundedRectangle(scope,5.f,.8f);
    g.setColour(juce::Colour::fromRGB(38,80,70).withAlpha(.55f));
    g.drawLine(scope.getX()+5.f,scope.getCentreY(),
               scope.getRight()-5.f,scope.getCentreY(),.75f);

    juce::Path signal;
    for(int i=0;i<64;++i) {
        const float value=processor.meters.scope[size_t(i)].load(std::memory_order_relaxed);
        const float xx=558.f+float(i)*(129.f/63.f);
        const float yy=scope.getCentreY()-std::clamp(value,-1.f,1.f)*43.f;
        if(i==0)signal.startNewSubPath(xx,yy);else signal.lineTo(xx,yy);
    }
    g.setColour(glow.withAlpha(.90f));
    g.strokePath(signal,juce::PathStrokeType(1.35f));

    g.setColour(muted);g.setFont(juce::Font(10.5f));
    const juce::String annotation=shownMode==5
        ? ("PITCH  "+juce::String(processor.meters.pitch.load(std::memory_order_relaxed),1)+" Hz")
        : ("MODE  /  "+juce::String(modes[std::clamp(shownMode,0,5)]));
    g.drawText(annotation,554,274,142,16,juce::Justification::left);
    for(int ch=0;ch<2;++ch) {
        const float peak=std::clamp(processor.meters.peaks[ch].load(std::memory_order_relaxed),0.f,1.f);
        const float y=299.f+ch*14.f;
        g.setColour(juce::Colour::fromRGB(27,50,49));
        g.fillRoundedRectangle(554.f,y,136.f,5.f,2.f);
        g.setColour(peak>.94f?juce::Colour::fromRGB(238,161,103):glow);
        if(peak>0.f)g.fillRoundedRectangle(554.f,y,136.f*peak,5.f,2.f);
    }

    g.setColour(muted);g.setFont(juce::Font(10.5f));
    const char* pageCaption=page==0?"CORE CONTROLS":page==1?"MODE CHARACTER":"ROUTING + CLOCK";
    g.drawText(pageCaption,123,357,108,16,juce::Justification::centredLeft);
    if(page==2) {
        g.setColour(glow.withAlpha(.8f));g.setFont(juce::Font(10.f));
        g.drawText("MORPH",247,337,85,13,juce::Justification::left);
        g.drawText(shownMode==5?"SCALE LOCK":"LFO SHAPE",247,374,122,12,
                   juce::Justification::left);
    }

    // Footer remains legible when the editor is scaled to compact mode.
    g.setColour(muted);g.setFont(juce::Font(12.f));
    g.drawText("HOST "+juce::String(processor.meters.bpm.load(std::memory_order_relaxed),1)+
               " BPM    /    PDC "+juce::String(processor.getLatencySamples())+
               "    /    CPU "+juce::String(processor.meters.cpu.load(std::memory_order_relaxed),1)+
               "%    /    "+juce::String(modes[std::clamp(shownMode,0,5)]),
               20,430,685,18,juce::Justification::centredLeft);
    g.restoreState();
}

juce::Font FlangLookAndFeel::getTextButtonFont(juce::TextButton&,int height) {
    return juce::Font(juce::jlimit(10.f,15.f,float(height)*.45f));
}
juce::Font FlangLookAndFeel::getComboBoxFont(juce::ComboBox& box) {
    return juce::Font(juce::jlimit(10.f,14.f,float(box.getHeight())*.46f));
}
