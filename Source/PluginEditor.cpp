#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr int baseW=720,baseH=450;
const juce::Colour dark=juce::Colour::fromRGB(7,12,14);
const juce::Colour panel=juce::Colour::fromRGB(17,26,29);
const juce::Colour bone=juce::Colour::fromRGB(226,239,230);
const juce::Colour muted=juce::Colour::fromRGB(143,169,164);
const char* modes[]{"CHORUS","FLANGER","VOWEL (LEGACY)","REVERSE (LEGACY)",
                     "ORBIT","TUNED (LEGACY)","FLANGUS","DIMENSION"};
constexpr std::array<int,5> visibleModes{0,1,6,4,7};
const char* lanes[]{"DEPTH","SPEED","DELAY","SPREAD","CROSS","DRY","WET"};
const char* knobNames[][4]{
    {"DETUNE","WIDTH","WARMTH","ENSEMBLE"},
    {"SWEEP","COLOR","MANUAL","ENV"},
    {"VOWEL","GENDER","SHARP","COMB"},
    {"GRAIN","OVERLAP","SMEAR","SHIMMER"},
    {"RADIUS","TILT","AIR","SPIN"},
    {"TRACK","GLIDE","HARMONIC","RESONANCE"},
    {"TRAVEL","WIDTH","JET","SHADE"},
    {"DETUNE","WIDTH","SOFTNESS","PAN"}
};
const juce::Colour tints[]{
    juce::Colour::fromRGB(75,230,172),  // chorus / seafoam
    juce::Colour::fromRGB(60,203,245),  // flanger / ice cyan
    juce::Colour::fromRGB(154,219,179), // legacy vowel
    juce::Colour::fromRGB(124,207,207), // legacy reverse
    juce::Colour::fromRGB(138,160,255), // orbit / electric periwinkle
    juce::Colour::fromRGB(177,222,156), // legacy tuned
    juce::Colour::fromRGB(154,244,77),  // flangus / skell neon lime
    juce::Colour::fromRGB(246,202,131)  // dimension / warm ivory-gold
};
constexpr std::array<const char*,16> presets{
    "INIT","Velvet Chorus","Wide Vocal Double","Neon Ensemble",
    "Satin Flanger","Jet Sweep","Polarity Comb",
    "Flangus Glow","Flangus Motion","Flangus Jet",
    "Orbit Halo","Orbit Carousel","Orbit Pulse",
    "Dimension Air","Dimension Doubler","Dimension Width"
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
    g.setColour(juce::Colour::fromRGB(33,54,51));g.drawRoundedRectangle(x,y,w,h,9.f,.8f);
}
}

void FlangLookAndFeel::drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,
                                       float pos,float,float,juce::Slider::SliderStyle style,juce::Slider&) {
    // LinearHorizontal is used by MORPH. It cannot share a vertical thumb
    // coordinate: JUCE's slider position is an X coordinate in this mode.
    if(style==juce::Slider::LinearHorizontal) {
        const float centreY=float(y)+float(h)*.5f;
        const float start=float(x)+11.f;
        const float finish=float(x+w)-11.f;
        const float thumbX=std::clamp(pos,start,finish);
        g.setColour(juce::Colour::fromRGB(7,17,19));
        g.fillRoundedRectangle(start,centreY-3.f,finish-start,6.f,3.f);
        g.setColour(juce::Colour::fromRGB(47,75,69));
        g.drawRoundedRectangle(start,centreY-3.f,finish-start,6.f,3.f,.8f);
        for(int i=0;i<=8;++i) {
            const float px=start+(finish-start)*float(i)/8.f;
            g.setColour(juce::Colour::fromRGB(99,139,127).withAlpha(.65f));
            g.drawLine(px,centreY-8.f,px,centreY-6.f,.75f);
        }
        if(thumbX>start) {
            g.setColour(accent.withAlpha(.90f));
            g.fillRoundedRectangle(start,centreY-2.f,thumbX-start,4.f,2.f);
        }
        const auto handle=juce::Rectangle<float>(thumbX-7.f,centreY-9.f,14.f,18.f);
        g.setColour(juce::Colour::fromRGB(24,44,44));
        g.fillRoundedRectangle(handle,3.6f);
        g.setColour(accent.withAlpha(.93f));
        g.drawRoundedRectangle(handle,3.6f,1.1f);
        g.setColour(juce::Colour::fromRGB(206,232,220));
        g.drawLine(thumbX,centreY-4.f,thumbX,centreY+4.f,1.4f);
        return;
    }
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
    g.setColour(juce::Colour::fromRGB(197,217,207));
    g.fillEllipse(mid-2.f,capY-4.5f,4.f,4.f);
    g.setColour(accent);g.fillRoundedRectangle(mid-9.f,capY+2.f,18.f,1.8f,.9f);
}

void FlangLookAndFeel::drawRotarySlider(juce::Graphics&g,int x,int y,int w,int h,
                                        float position,float start,float end,juce::Slider&) {
    const float cx=float(x)+w*.5f,cy=float(y)+h*.5f;
    const float radius=std::min(w,h)*.325f;
    const auto outer=juce::Rectangle<float>(cx-radius-3.f,cy-radius-3.f,
                                           2.f*radius+6.f,2.f*radius+6.f);
    const auto face=juce::Rectangle<float>(cx-radius,cy-radius,2.f*radius,2.f*radius);
    // Sculpted dark-steel ring and a restrained light catching the upper rim.
    // This is vector JUCE painting, not a scaled image or per-frame blur.
    g.setColour(juce::Colour::fromRGB(3,10,12));
    g.fillEllipse(outer.translated(0.f,1.f));
    g.setColour(juce::Colour::fromRGB(65,91,86));
    g.drawEllipse(outer,1.f);
    juce::Path unfilled;
    unfilled.addCentredArc(cx,cy,radius+4.f,radius+4.f,0.f,start,end,true);
    g.setColour(juce::Colour::fromRGB(38,66,62));
    g.strokePath(unfilled,juce::PathStrokeType(2.2f));
    juce::Path active;
    active.addCentredArc(cx,cy,radius+4.f,radius+4.f,0.f,
                         start,start+(end-start)*position,true);
    g.setColour(accent.withAlpha(.95f));
    g.strokePath(active,juce::PathStrokeType(2.4f,juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));
    g.setGradientFill(juce::ColourGradient(
        juce::Colour::fromRGB(54,79,79),face.getX(),face.getY(),
        juce::Colour::fromRGB(15,29,32),face.getRight(),face.getBottom(),false));
    g.fillEllipse(face);
    g.setColour(juce::Colour::fromRGB(119,148,138).withAlpha(.76f));
    g.drawEllipse(face.reduced(.7f),.9f);
    g.setColour(juce::Colour::fromRGB(13,28,30));
    g.fillEllipse(face.reduced(radius*.42f));
    const float angle=start+(end-start)*position;
    const float dx=std::sin(angle),dy=-std::cos(angle);
    g.setColour(bone);
    g.drawLine(cx+dx*radius*.34f,cy+dy*radius*.34f,
               cx+dx*radius*.76f,cy+dy*radius*.76f,2.5f);
    g.setColour(accent.withAlpha(.9f));
    g.fillEllipse(cx-1.65f,cy-1.65f,3.3f,3.3f);
}

void FlangLookAndFeel::drawButtonBackground(juce::Graphics&g,juce::Button&button,
                                             const juce::Colour&,bool hover,bool down) {
    const auto b=button.getLocalBounds().toFloat().reduced(1.f);
    const bool selected=button.getToggleState();
    const auto id=button.getComponentID();
    const bool modeButton=id.startsWith("mode-");
    const bool topButton=id=="skull"||id=="settings"||id=="preset";
    const float radius=topButton?6.f:5.f;
    const auto surface=juce::Colour::fromRGB(13,25,29);
    // Crisp translucent fill on the currently selected mode, neutral inactive
    // cards, and an understated metallic top highlight for rack continuity.
    g.setColour(selected?accent.withAlpha(modeButton?.24f:.18f):
                hover?juce::Colour::fromRGB(26,43,45):surface);
    g.fillRoundedRectangle(b,radius);
    g.setColour(selected?accent.withAlpha(.96f):
                 hover?accent.withAlpha(.45f):juce::Colour::fromRGB(46,74,70));
    g.drawRoundedRectangle(b,radius,selected?1.55f:.85f);
    g.setColour(selected?accent.withAlpha(.75f):
                 juce::Colour::fromRGB(88,123,117).withAlpha(.32f));
    g.drawLine(b.getX()+7.f,b.getY()+1.8f,
               b.getRight()-7.f,b.getY()+1.8f,.7f);
    if(selected&&modeButton) {
        g.setColour(accent.withAlpha(.87f));
        g.fillRoundedRectangle(b.getX()+11.f,b.getBottom()-3.2f,
                               b.getWidth()-22.f,1.8f,.9f);
    }
    if(down) {
        g.setColour(accent.withAlpha(.18f));
        g.fillRoundedRectangle(b.reduced(2.f),4.f);
    }
    if(id=="preset") {
        const float yy=b.getCentreY()+.5f;
        const float xx=b.getRight()-13.f;
        juce::Path downArrow;
        downArrow.startNewSubPath(xx-3.f,yy-2.f);
        downArrow.lineTo(xx,yy+1.f);
        downArrow.lineTo(xx+3.f,yy-2.f);
        g.setColour(accent.withAlpha(.80f));
        g.strokePath(downArrow,juce::PathStrokeType(1.25f));
    }
    if(id=="skull") {
        // Small original bone crest; vector drawn at any editor scale.
        const float cx=b.getCentreX(),cy=b.getCentreY();
        const float unit=std::min(b.getWidth()/38.f,b.getHeight()/28.f);
        auto p=[&](float v){return v*unit;};
        g.setColour(accent.withAlpha(.95f));
        g.fillEllipse(cx-p(10.f),cy-p(11.f),p(20.f),p(19.f));
        g.fillRoundedRectangle(cx-p(7.f),cy+p(1.f),p(14.f),p(10.f),p(2.f));
        g.setColour(juce::Colour::fromRGB(9,21,23));
        g.fillEllipse(cx-p(7.5f),cy-p(4.f),p(6.5f),p(6.f));
        g.fillEllipse(cx+p(1.f),cy-p(4.f),p(6.5f),p(6.f));
        juce::Path nose;
        nose.addTriangle(cx,cy+p(2.f),cx-p(2.2f),cy+p(5.f),cx+p(2.2f),cy+p(5.f));
        g.fillPath(nose);
        g.setColour(juce::Colour::fromRGB(9,21,23).withAlpha(.85f));
        g.drawLine(cx-p(4.5f),cy+p(7.6f),cx+p(4.5f),cy+p(7.6f),p(.8f));
        g.drawLine(cx,cy+p(5.6f),cx,cy+p(10.f),p(.8f));
    } else if(id=="settings") {
        const float cx=b.getCentreX(),cy=b.getCentreY();
        const float rr=std::min(b.getWidth(),b.getHeight())*.23f;
        g.setColour(accent.withAlpha(.9f));
        for(int k=0;k<8;++k) {
            const float theta=float(2*skellflang::pi*k/8.);
            g.drawLine(cx+std::cos(theta)*rr,cy+std::sin(theta)*rr,
                       cx+std::cos(theta)*rr*1.43f,cy+std::sin(theta)*rr*1.43f,1.6f);
        }
        g.drawEllipse(cx-rr,cy-rr,2*rr,2*rr,1.7f);
        g.fillEllipse(cx-rr*.34f,cy-rr*.34f,rr*.68f,rr*.68f);
    }
}

juce::Font FlangLookAndFeel::getPopupMenuFont() {
    return juce::Font(12.2f);
}
void FlangLookAndFeel::drawPopupMenuBackground(juce::Graphics&g,int w,int h) {
    g.setGradientFill(juce::ColourGradient(
        juce::Colour::fromRGB(19,31,34),0.f,0.f,
        juce::Colour::fromRGB(10,20,24),float(w),float(h),false));
    g.fillRect(0,0,w,h);
    g.setColour(juce::Colour::fromRGB(55,83,80));
    g.drawRect(0,0,w,h,1);
    g.setColour(accent.withAlpha(.75f));
    g.fillRect(1,1,std::max(1,w-2),2);
}
void FlangLookAndFeel::drawPopupMenuSectionHeader(juce::Graphics&g,
        const juce::Rectangle<int>& area,const juce::String& title) {
    g.setColour(juce::Colour::fromRGB(31,49,50).withAlpha(.65f));
    g.fillRect(area.reduced(1,2));
    g.setColour(accent.withAlpha(.90f));
    g.setFont(juce::Font(10.2f,juce::Font::bold));
    g.drawFittedText(title.toUpperCase(),area.reduced(14,1),
                     juce::Justification::centredLeft,1);
}
void FlangLookAndFeel::drawPopupMenuItem(juce::Graphics&g,
       const juce::Rectangle<int>& area,bool separator,bool enabled,bool highlighted,
       bool ticked,bool submenu,const juce::String& caption,const juce::String&,
       const juce::Drawable*,const juce::Colour* overrideColour) {
    if(separator) {
        g.setColour(juce::Colour::fromRGB(49,83,75));
        g.drawHorizontalLine(area.getCentreY(),float(area.getX()+12),float(area.getRight()-12));
        return;
    }
    const auto inner=area.reduced(5,1);
    if(highlighted&&enabled) {
        g.setColour(accent.withAlpha(.13f));
        g.fillRoundedRectangle(inner.toFloat(),4.f);
        g.setColour(accent.withAlpha(.88f));
        g.fillRoundedRectangle(float(inner.getX()),float(inner.getY()+4),
                               2.f,float(std::max(1,inner.getHeight()-8)),1.f);
    }
    juce::Colour fg=overrideColour?*overrideColour:bone;
    if(!enabled)fg=fg.withAlpha(.38f);
    g.setColour(fg);
    g.setFont(juce::Font(12.f,highlighted?juce::Font::bold:juce::Font::plain));
    g.drawFittedText(caption,area.withTrimmedLeft(28).withTrimmedRight(20),
                     juce::Justification::centredLeft,1);
    if(ticked) {
        g.setColour(accent);
        g.fillEllipse(float(area.getX()+11),float(area.getCentreY()-3),6.f,6.f);
    }
    if(submenu) {
        juce::Path arrow;
        const float ax=float(area.getRight()-13),ay=float(area.getCentreY());
        arrow.startNewSubPath(ax-3,ay-4);arrow.lineTo(ax+1,ay);
        arrow.lineTo(ax-3,ay+4);
        g.setColour(accent.withAlpha(.8f));g.strokePath(arrow,juce::PathStrokeType(1.6f));
    }
}
void FlangLookAndFeel::getIdealPopupMenuItemSize(
        const juce::String& text,bool separator,int,int& width,int& height) {
    width=separator?30:juce::jmax(198,juce::roundToInt(juce::Font(12.f).getStringWidthFloat(text))+65);
    height=separator?8:29;
}
SkellFlangAudioProcessorEditor::SkellFlangAudioProcessorEditor(SkellFlangAudioProcessor& p)
: AudioProcessorEditor(&p),processor(p) {
    setLookAndFeel(&skin);
    for(auto* dropdown:{&target,&shape}) {
        dropdown->setColour(juce::ComboBox::backgroundColourId,juce::Colour::fromRGB(10,27,29));
        dropdown->setColour(juce::ComboBox::textColourId,bone);
        dropdown->setColour(juce::ComboBox::outlineColourId,juce::Colour::fromRGB(47,82,72));
        dropdown->setColour(juce::ComboBox::arrowColourId,juce::Colour::fromRGB(116,212,172));
    }
    skin.setColour(juce::PopupMenu::backgroundColourId,juce::Colour::fromRGB(9,20,23));
    skin.setColour(juce::PopupMenu::textColourId,bone);
    skin.setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour::fromRGB(28,61,55));
    skin.setColour(juce::PopupMenu::highlightedTextColourId,bone);
    theme=juce::jlimit(0,2,int(p.parameters.state.getProperty("uiTheme",0)));
    for(int i=0;i<7;++i) {
        styleSlider(faders[i],true);
        addAndMakeVisible(faders[i]);
        const auto id=juce::StringArray{"depth","speed","delay","spread","cross","dry","wet"}[i];
        if(auto* param=p.parameters.getParameter(id))
            faders[i].setDoubleClickReturnValue(true,param->convertFrom0to1(param->getDefaultValue()));
        label(faderLabels[i],lanes[i]);addAndMakeVisible(faderLabels[i]);
        faders[i].onValueChange=[this,i] {
            faders[i].setTooltip(juce::String(lanes[i])+" : "+juce::String(faders[i].getValue(),2));
        };
        faderLinks[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            p.parameters,id,faders[i]);
    }
    styleSlider(order,true);
    order.setRange(1,8,1);
    order.setDoubleClickReturnValue(true,3);
    order.setTooltip("Number of stacked voices: 1 to 8");
    addAndMakeVisible(order);
    orderLink=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.parameters,"order",order);

    for(int i=0;i<7;++i) {
        styleSlider(knobs[i],false);addAndMakeVisible(knobs[i]);
        label(knobLabels[i],i==0?"FEEDBACK":i==1?"TONE":i==2?"OUTPUT":"COLOR");
        addAndMakeVisible(knobLabels[i]);
        knobs[i].onValueChange=[this,i] {
            knobs[i].setTooltip(knobLabels[i].getText()+" : "+juce::String(knobs[i].getValue(),2));
        };
        const auto id=i==0?"feedback":i==1?"tone":i==2?"output":
                      i==3?"extra0":i==4?"extra1":i==5?"extra2":"extra3";
        if(auto* param=p.parameters.getParameter(id))
            knobs[i].setDoubleClickReturnValue(true,param->convertFrom0to1(param->getDefaultValue()));
        knobLinks[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            p.parameters,id,knobs[i]);
    }
    const char* hints[]{
        "Multi-voice detuned stereo chorus",
        "Comb flanger with stereo phase modulation",
        "Stacked chorus-flanger hybrid",
        "Rotating stereo field with independent L/R",
        "Slow stereo doubler and dimensional widening"
    };
    for(int i=0;i<5;++i) {
        const int mode=visibleModes[size_t(i)];
        tabs[size_t(i)].setButtonText(modes[mode]);
        tabs[size_t(i)].setTooltip(hints[i]);
        tabs[size_t(i)].setComponentID("mode-"+juce::String(mode));
        tabs[size_t(i)].setColour(juce::TextButton::textColourOffId,bone);
        tabs[size_t(i)].setColour(juce::TextButton::textColourOnId,bone);
        addAndMakeVisible(tabs[size_t(i)]);
        tabs[size_t(i)].onClick=[this,mode]{
            if(auto* param=processor.parameters.getParameter("mode")) {
                param->beginChangeGesture();
                param->setValueNotifyingHost(param->convertTo0to1(float(mode)));
                param->endChangeGesture();
            }
            updateMode();
        };
    }

    // The host retains all eight original choice positions. The compact UI
    // intentionally offers only the five current effects, with direct IDs.
    for(const auto mode:visibleModes)target.addItem(modes[mode],mode+1);
    // A non-attached ComboBox must explicitly synchronize its first value
    // before the initial paint; otherwise hosts see a blank destination.
    target.setTextWhenNothingSelected("LEGACY TARGET");
    if(const auto* current=p.parameters.getRawParameterValue("morphTarget")) {
        const int wanted=std::clamp(int(current->load(std::memory_order_relaxed)),0,7);
        if(std::find(visibleModes.begin(),visibleModes.end(),wanted)!=visibleModes.end())
            target.setSelectedId(wanted+1,juce::dontSendNotification);
    }
    target.onChange=[this]{
        const int selected=target.getSelectedId()-1;
        if(selected>=0)
            if(auto* param=processor.parameters.getParameter("morphTarget")) {
                param->beginChangeGesture();
                param->setValueNotifyingHost(param->convertTo0to1(float(selected)));
                param->endChangeGesture();
            }
    };
    for(int i=0;i<5;++i)
        shape.addItem(juce::StringArray{"Sine","Triangle","Saw+","Saw-","Square"}[i],i+1);
    target.setComponentID("morph-target");
    target.setTooltip("Morph destination effect");
    shape.setComponentID("lfo-wave");
    shape.setTooltip("LFO waveform");
    sync.setComponentID("lfo-sync");
    addAndMakeVisible(target);addAndMakeVisible(shape);
    shapeLink=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        p.parameters,"lfoShape",shape);
    morph.setSliderStyle(juce::Slider::LinearHorizontal);
    morph.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
    morph.setDoubleClickReturnValue(true,0.0);
    morph.setTooltip("Crossfade to the selected destination effect");
    morph.setComponentID("morph");
    addAndMakeVisible(morph);
    morphLink=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.parameters,"morph",morph);
    sync.setClickingTogglesState(true);
    sync.setTooltip("Sync LFO motion to your host tempo");
    addAndMakeVisible(sync);
    syncLink=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.parameters,"lfoSync",sync);
    // Keep the automatable power parameter and attachment, but no ACTIVE/BYPASS
    // banner. Processing can be enabled in the gear menu or by host automation.
    power.setClickingTogglesState(true);
    power.setVisible(false);
    powerLink=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.parameters,"power",power);

    skull.setComponentID("skull");skull.setTooltip("Cycle Skell-Flang skins");
    settings.setComponentID("settings");settings.setTooltip("Plugin settings and window size");
    preset.setComponentID("preset");preset.setTooltip("Open the Skell-Flang sound library");
    addAndMakeVisible(skull);addAndMakeVisible(settings);
    addAndMakeVisible(preset);
    skull.onClick=[this]{
        theme=(theme+1)%3;
        processor.parameters.state.setProperty("uiTheme",theme,nullptr);
        shownMode=-1;updateMode();
    };
    const juce::Component::SafePointer<SkellFlangAudioProcessorEditor> safeEditor(this);
    preset.onClick=[this,safeEditor] {
        juce::PopupMenu menu;
        menu.addSectionHeader("SKELL SOUND LIBRARY");
        const auto selected=processor.parameters.state.getProperty("presetName","INIT").toString();
        menu.addItem(1,"INIT - Neutral Start",true,selected=="INIT");
        menu.addSeparator();
        const char* groups[]{"CHORUS / ENSEMBLE","FLANGER / COMB","FLANGUS / HYBRID",
                             "ORBIT / SPATIAL","DIMENSION / DOUBLE"};
        const int offsets[]{1,4,7,10,13};
        for(int group=0;group<5;++group) {
            juce::PopupMenu sounds;
            for(int j=0;j<3;++j) {
                const int index=offsets[group]+j;
                sounds.addItem(index+1,presets[size_t(index)],true,
                               selected==presets[size_t(index)]);
            }
            menu.addSubMenu(groups[group],sounds,true);
        }
        menu.showMenuAsync(juce::PopupMenu::Options()
                             .withTargetComponent(&preset).withMinimumWidth(256)
                             .withMaximumNumColumns(1),
            [safeEditor](int result){
                if(safeEditor&&result>=1&&result<=16) {
                    safeEditor->processor.applyFactoryPreset(result-1);
                    safeEditor->updateMode();
                }
            });
    };
    settings.onClick=[this,safeEditor]{
        juce::PopupMenu menu;
        menu.addSectionHeader("SKELL-FLANG SETTINGS");
        const auto* active=processor.parameters.getRawParameterValue("power");
        const bool powered=active&&active->load(std::memory_order_relaxed)>.5f;
        menu.addItem(1,"Effect enabled",true,powered);
        menu.addSeparator();
        menu.addSectionHeader("WINDOW SIZE");
        const char* names[]{"Compact - 540 px","Comfort - 720 px",
                            "Large - 900 px","Studio - 1080 px"};
        const int widths[]{540,720,900,1080};
        for(int i=0;i<4;++i)
            menu.addItem(10+i,names[i],true,juce::roundToInt(getWidth())==widths[i]);
        menu.showMenuAsync(juce::PopupMenu::Options()
                               .withTargetComponent(&settings).withMinimumWidth(242)
                               .withMaximumNumColumns(1),
            [safeEditor](int item){
                if(!safeEditor)return;
                if(item==1) {
                    if(auto* v=safeEditor->processor.parameters.getParameter("power")) {
                        v->beginChangeGesture();
                        v->setValueNotifyingHost(v->convertTo0to1(
                            v->getValue()>.5f?0.f:1.f));
                        v->endChangeGesture();
                    }
                } else if(item>=10&&item<14) {
                    const int width[]={540,720,900,1080};
                    const int w=width[item-10];
                    safeEditor->processor.parameters.state.setProperty("uiWidth",w,nullptr);
                    safeEditor->setSize(w,juce::roundToInt(w*double(baseH)/baseW));
                }
            });
    };

    setResizable(true,true);
    if(auto* c=getConstrainer())c->setFixedAspectRatio(double(baseW)/baseH);
    setResizeLimits(540,338,1080,675);
    const int savedWidth=juce::jlimit(540,1080,
        int(p.parameters.state.getProperty("uiWidth",540)));
    setSize(savedWidth,juce::roundToInt(savedWidth*double(baseH)/baseW));
    updateMode();
    startTimerHz(30);
}
SkellFlangAudioProcessorEditor::~SkellFlangAudioProcessorEditor(){
    stopTimer();setLookAndFeel(nullptr);
}

void SkellFlangAudioProcessorEditor::resized() {
    const int currentWidth=getWidth();
    if(currentWidth>=540&&currentWidth<=1080&&
       int(processor.parameters.state.getProperty("uiWidth",540))!=currentWidth)
        processor.parameters.state.setProperty("uiWidth",currentWidth,nullptr);
    const float s=currentWidth/float(baseW);
    const auto fontSize=juce::jlimit(9.1f,12.f,12.f*s);
    for(auto& text:faderLabels)text.setFont(juce::Font(fontSize));
    for(auto& text:knobLabels)text.setFont(juce::Font(juce::jlimit(9.f,11.f,11.f*s)));
    auto place=[s](juce::Component& c,int x,int y,int w,int h){
        c.setBounds(juce::roundToInt(x*s),juce::roundToInt(y*s),
                    juce::roundToInt(w*s),juce::roundToInt(h*s));
    };
    place(skull,14,10,43,29);
    place(preset,342,10,222,29);
    place(settings,644,10,62,29);
    for(int i=0;i<5;++i)place(tabs[size_t(i)],16+i*139,69,132,34);
    place(order,25,147,52,116);
    for(int i=0;i<7;++i) {
        place(faders[size_t(i)],93+i*62,147,49,116);
        place(faderLabels[size_t(i)],88+i*62,268,59,21);
        const int x=125+i*80;
        place(knobs[size_t(i)],x,311,57,56);
        place(knobLabels[size_t(i)],x-7,370,72,18);
    }
    // Permanent integrated morph row. No page or popup switches; all main
    // audio and character parameters remain visible at 540 px and larger.
    place(target,53,395,130,25);
    place(morph,238,395,183,25);
    place(shape,478,395,97,25);
    place(sync,591,395,100,25);
}
void SkellFlangAudioProcessorEditor::updateMode() {
    const auto* raw=processor.parameters.getRawParameterValue("mode");
    const int selected=std::clamp(raw?int(raw->load(std::memory_order_relaxed)):0,0,7);
    // Also refresh MORPH TARGET when presets or host automation change it,
    // including cases where the active effect tab has not changed.
    if(const auto* current=processor.parameters.getRawParameterValue("morphTarget")) {
        const int wanted=std::clamp(int(current->load(std::memory_order_relaxed)),0,7);
        const int id=std::find(visibleModes.begin(),visibleModes.end(),wanted)
                     !=visibleModes.end()?wanted+1:0;
        if(target.getSelectedId()!=id)
            target.setSelectedId(id,juce::dontSendNotification);
    }
    if(selected==shownMode)return;
    shownMode=selected;
    // Every effect always retains its own color identity. The skull cycles
    // brighter / neutral / restrained accent intensity, not random colors.
    skin.accent=theme==0?tints[selected]
        :theme==1?tints[selected].brighter(.24f)
                  :tints[selected].withMultipliedSaturation(.73f).darker(.10f);
    for(auto& slider:faders)slider.repaint();
    for(auto& slider:knobs)slider.repaint();
    for(auto& tab:tabs)tab.repaint();
    for(auto* button:{&skull,&settings,&preset,&sync})button->repaint();
    order.repaint();morph.repaint();
    for(int i=0;i<5;++i)
        tabs[size_t(i)].setToggleState(visibleModes[size_t(i)]==selected,
                                       juce::dontSendNotification);
    for(int i=0;i<4;++i) {
        label(knobLabels[size_t(3+i)],knobNames[selected][i]);
        knobLabels[size_t(3+i)].setFont(juce::Font(
            juce::jlimit(9.f,11.f,11.f*getWidth()/float(baseW))));
    }
    repaint();
}
void SkellFlangAudioProcessorEditor::timerCallback() {
    updateMode();
    // Skell Key's segment-strip concept: logarithmic level, fast attack and
    // slow release. This state is UI-only; never touches the audio callback.
    const float rawPeak=std::max(processor.meters.peaks[0].load(std::memory_order_relaxed),
                                 processor.meters.peaks[1].load(std::memory_order_relaxed));
    const float peak=std::isfinite(rawPeak)?juce::jlimit(1.0e-6f,4.f,rawPeak):1.0e-6f;
    const float db=20.f*std::log10(peak);
    const float level=juce::jlimit(0.f,1.f,(db+54.f)/54.f);
    headerLevel+=(level-headerLevel)*(level>headerLevel?.60f:.12f);
    const juce::String name=processor.parameters.state
        .getProperty("presetName","INIT").toString();
    if(preset.getButtonText()!=name)preset.setButtonText(name);
    const float s=getWidth()/float(baseW);
    repaint(0,juce::roundToInt(50*s),getWidth(),juce::roundToInt(15*s));
    repaint(juce::roundToInt(536*s),juce::roundToInt(114*s),
            juce::roundToInt(171*s),juce::roundToInt(181*s));
    repaint(0,juce::roundToInt(428*s),getWidth(),
            std::max(1,getHeight()-juce::roundToInt(426*s)));
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
    g.drawLine(14.f,47.f,706.f,47.f,.75f);
    // Skell Key-inspired discrete feedback strip: 52 restrained segments,
    // illuminated directly by actual stereo peak signal, no decorative L/R.
    constexpr int meterSegments=52;
    const float slot=692.f/float(meterSegments);
    const int active=juce::jlimit(0,meterSegments,
                                  juce::roundToInt(headerLevel*meterSegments));
    for(int i=0;i<meterSegments;++i) {
        const float x=14.f+float(i)*slot;
        const bool lit=i<active;
        const float h=lit?3.8f:1.8f;
        g.setColour(lit?skin.accent.withAlpha(.56f+.39f*i/meterSegments)
                       :juce::Colour::fromRGB(51,72,67).withAlpha(.65f));
        g.fillRoundedRectangle(x,54.f+(4.f-h)*.5f,
                               slot-2.3f,h,.75f);
    }
    // A mode-specific identity glow follows selection only, not idle hover.
    for(int i=0;i<5;++i)if(shownMode==visibleModes[size_t(i)]) {
        g.setColour(skin.accent.withAlpha(.93f));
        g.fillRoundedRectangle(31.f+139.f*i,105.f,101.f,2.f,1.f);
        break;
    }

    panelRect(g,14,114,516,183);
    panelRect(g,538,114,169,183);
    panelRect(g,14,303,693,122);

    // The clickable vector skull lives in the header; no duplicate ornament.
    const auto glow=skin.accent;
    g.setColour(bone);g.setFont(juce::Font(20.f,juce::Font::bold));
    g.drawText("SKELL-FLANG",74,8,245,29,juce::Justification::centredLeft);
    g.setColour(glow.withAlpha(.9f));g.setFont(juce::Font(10.5f));
    g.drawText("BEND THE BONES  /  MODULATION ENGINE",76,34,244,12,
               juce::Justification::centredLeft);

    g.setColour(muted);g.setFont(juce::Font(10.5f));
    g.drawText("01  /  CONTROL DECK",27,120,170,16,juce::Justification::left);
    g.drawText("ORD",24,268,54,20,juce::Justification::centred);
    g.drawText("02  /  SIGNAL",550,120,143,16,juce::Justification::left);

    // Small technical divider motifs make the layout distinctive, not busy.
    g.setColour(glow.withAlpha(.50f));
    g.fillRoundedRectangle(27.f,139.f,33.f,1.3f,.5f);
    g.fillRoundedRectangle(550.f,139.f,27.f,1.3f,.5f);

    const juce::Rectangle<float> scope(552.f,151.f,141.f,104.f);
    g.setColour(juce::Colour::fromRGB(8,17,19));
    g.fillRoundedRectangle(scope,5.f);
    g.setColour(juce::Colour::fromRGB(38,68,63));
    g.drawRoundedRectangle(scope,5.f,.8f);
    // Ghost ribs and central spine remain faint under a live waveform.
    // They are static original vectors, not simulated audio or a bitmap.
    const float centreX=scope.getCentreX();
    g.setColour(juce::Colour::fromRGB(51,107,94).withAlpha(.25f));
    g.drawLine(centreX,scope.getY()+11.f,centreX,scope.getBottom()-11.f,1.f);
    for(int i=0;i<6;++i) {
        const float yy=scope.getY()+17.f+float(i)*12.6f;
        const float reach=36.f+5.f*std::sin(float(i)*.55f);
        juce::Path rib;
        rib.startNewSubPath(centreX-3.f,yy);
        rib.quadraticTo(centreX-reach*.52f,yy-7.f,
                        centreX-reach,yy+4.f);
        rib.startNewSubPath(centreX+3.f,yy);
        rib.quadraticTo(centreX+reach*.52f,yy-7.f,
                        centreX+reach,yy+4.f);
        g.strokePath(rib,juce::PathStrokeType(.75f));
    }
    g.setColour(juce::Colour::fromRGB(38,80,70).withAlpha(.55f));
    g.drawLine(scope.getX()+5.f,scope.getCentreY(),
               scope.getRight()-5.f,scope.getCentreY(),.75f);

    juce::Path signal;
    const auto next=processor.meters.scopeNext.load(std::memory_order_acquire);
    for(int i=0;i<64;++i) {
        const auto index=(next+unsigned(i))%64u;
        const float value=processor.meters.scope[index].load(std::memory_order_relaxed);
        const float xx=558.f+float(i)*(129.f/63.f);
        const float yy=scope.getCentreY()-std::clamp(value,-1.f,1.f)*40.f;
        if(i==0)signal.startNewSubPath(xx,yy);else signal.lineTo(xx,yy);
    }
    g.setColour(glow.withAlpha(.90f));
    g.strokePath(signal,juce::PathStrokeType(1.35f));

    g.setColour(muted);g.setFont(juce::Font(10.5f));
    const juce::String annotation=shownMode==5
        ? ("PITCH  "+juce::String(processor.meters.pitch.load(std::memory_order_relaxed),1)+" Hz")
        : ("MODE  /  "+juce::String(modes[std::clamp(shownMode,0,7)]));
    g.drawText(annotation,554,267,142,16,juce::Justification::left);

    // Every parameter is available without entering a separate MOD page.
    g.setColour(muted);
    g.setFont(juce::Font(10.5f,juce::Font::bold));
    g.drawFittedText("03 / TONE",27,321,84,15,
                     juce::Justification::centredLeft,1);
    g.setFont(juce::Font(9.5f));
    g.drawFittedText("CHARACTER",27,346,92,13,
                     juce::Justification::centredLeft,1);
    g.setColour(juce::Colour::fromRGB(44,71,66));
    g.fillRoundedRectangle(27.f,389.f,669.f,1.f,.5f);
    // Inline captions do not overlap the knob labels above at compact scale.
    g.setColour(skin.accent.withAlpha(.78f));
    g.setFont(juce::Font(9.8f,juce::Font::bold));
    g.drawText("TO",29,395,23,25,juce::Justification::centredLeft);
    g.drawText("MORPH",188,395,50,25,juce::Justification::centredLeft);
    g.drawText("WAVE",429,395,47,25,juce::Justification::centredLeft);

    // Footer remains legible when the editor is scaled to compact mode.
    g.setColour(muted);g.setFont(juce::Font(12.f));
    g.drawText("LIVE  /  "+juce::String(processor.meters.bpm.load(std::memory_order_relaxed),1)+
               " BPM   /   PDC "+juce::String(processor.getLatencySamples())+
               "   /   CPU "+juce::String(processor.meters.cpu.load(std::memory_order_relaxed),1)+
               "%   /   "+juce::String(modes[std::clamp(shownMode,0,7)]),
               20,430,685,18,juce::Justification::centredLeft);
    g.restoreState();
}

juce::Font FlangLookAndFeel::getTextButtonFont(juce::TextButton&,int height) {
    return juce::Font(juce::jlimit(10.f,15.f,float(height)*.45f));
}
juce::Font FlangLookAndFeel::getComboBoxFont(juce::ComboBox& box) {
    return juce::Font(juce::jlimit(10.f,14.f,float(box.getHeight())*.46f));
}
