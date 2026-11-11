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
    juce::Colour::fromRGB(81,222,188),juce::Colour::fromRGB(65,207,216),
    juce::Colour::fromRGB(154,219,179),juce::Colour::fromRGB(124,207,207),
    juce::Colour::fromRGB(93,192,220),juce::Colour::fromRGB(177,222,156),
    juce::Colour::fromRGB(133,224,181),juce::Colour::fromRGB(147,204,245)
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
    g.setColour(selected?accent.withAlpha(.17f):
                hover?juce::Colour::fromRGB(26,47,47):juce::Colour::fromRGB(14,24,27));
    g.fillRoundedRectangle(b,5.f);
    g.setColour(selected?accent.withAlpha(.92f):juce::Colour::fromRGB(46,79,73));
    g.drawRoundedRectangle(b,5.f,selected?1.3f:.85f);
    if(down){g.setColour(accent.withAlpha(.18f));g.fillRoundedRectangle(b.reduced(2.f),4.f);}
    const auto id=button.getComponentID();
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
    return juce::Font(13.f);
}
void FlangLookAndFeel::drawPopupMenuBackground(juce::Graphics&g,int w,int h) {
    g.fillAll(juce::Colour::fromRGB(9,20,23));
    g.setColour(juce::Colour::fromRGB(52,87,80));
    g.drawRect(0,0,w,h,1);
}
void FlangLookAndFeel::drawPopupMenuSectionHeader(juce::Graphics&g,
        const juce::Rectangle<int>& area,const juce::String& title) {
    g.setColour(accent.withAlpha(.87f));
    g.setFont(juce::Font(10.5f,juce::Font::bold));
    g.drawFittedText(title.toUpperCase(),area.reduced(13,0),
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
    const auto inner=area.reduced(4,1);
    if(highlighted&&enabled) {
        g.setColour(accent.withAlpha(.15f));g.fillRoundedRectangle(inner.toFloat(),4.f);
        g.setColour(accent.withAlpha(.58f));g.fillRoundedRectangle(float(inner.getX()),
                                             float(inner.getY()+4),2.f,float(inner.getHeight()-8),1.f);
    }
    juce::Colour fg=overrideColour?*overrideColour:bone;
    if(!enabled)fg=fg.withAlpha(.38f);
    g.setColour(fg);
    g.setFont(juce::Font(12.5f,highlighted?juce::Font::bold:juce::Font::plain));
    g.drawFittedText(caption,area.withTrimmedLeft(27).withTrimmedRight(18),
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
    width=separator?30:juce::jmax(180,juce::roundToInt(juce::Font(12.5f).getStringWidthFloat(text))+62);
    height=separator?9:30;
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
    // Old sessions can restore uiPage=2; translate that to the new single MOD drawer.
    modulationOpen=int(p.parameters.state.getProperty("uiPage",0))==2;
    for(int i=0;i<7;++i) {
        styleSlider(faders[i],true);
        addAndMakeVisible(faders[i]);
        const auto id=juce::StringArray{"depth","speed","delay","spread","cross","dry","wet"}[i];
        if(auto* param=p.parameters.getParameter(id))
            faders[i].setDoubleClickReturnValue(true,param->convertFrom0to1(param->getDefaultValue()));
        label(faderLabels[i],lanes[i]);addAndMakeVisible(faderLabels[i]);
        faders[i].onValueChange=[this,i] {
            faders[i].setTooltip(juce::String(lanes[i])+" · "+juce::String(faders[i].getValue(),2));
        };
        faderLinks[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            p.parameters,id,faders[i]);
    }
    styleSlider(order,true);
    order.setRange(1,8,1);
    order.setDoubleClickReturnValue(true,3);
    order.setTooltip("Number of stacked voices (1–8)");
    addAndMakeVisible(order);
    orderLink=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.parameters,"order",order);

    for(int i=0;i<7;++i) {
        styleSlider(knobs[i],false);addAndMakeVisible(knobs[i]);
        label(knobLabels[i],i==0?"FEEDBACK":i==1?"TONE":i==2?"OUTPUT":"COLOR");
        addAndMakeVisible(knobLabels[i]);
        knobs[i].onValueChange=[this,i] {
            knobs[i].setTooltip(knobLabels[i].getText()+" · "+juce::String(knobs[i].getValue(),2));
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
    addAndMakeVisible(target);addAndMakeVisible(shape);
    comboLinks[1]=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
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
    mod.setComponentID("mod");mod.setTooltip("Open modulation and morph controls");
    preset.setComponentID("preset");preset.setTooltip("Open the Skell-Flang sound library");
    addAndMakeVisible(skull);addAndMakeVisible(settings);
    addAndMakeVisible(mod);addAndMakeVisible(preset);
    mod.onClick=[this]{
        modulationOpen=!modulationOpen;
        processor.parameters.state.setProperty("uiPage",modulationOpen?2:0,nullptr);
        updatePage();repaint();
    };
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
        menu.addItem(1,"INIT · Neutral Start",true,selected=="INIT");
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
            menu.addSubMenu(groups[group],sounds);
        }
        menu.showMenuAsync(juce::PopupMenu::Options()
                             .withTargetComponent(&preset).withMinimumWidth(244),
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
        const char* names[]{"Compact · 540","Comfort · 720",
                            "Large · 900","Studio · 1080"};
        const int widths[]{540,720,900,1080};
        for(int i=0;i<4;++i)
            menu.addItem(10+i,names[i],true,juce::roundToInt(getWidth())==widths[i]);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&settings),
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
    updatePage();
    startTimerHz(24);
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
    const auto fontSize=juce::jlimit(9.2f,12.5f,12.5f*s);
    for(auto& text:faderLabels)text.setFont(juce::Font(fontSize));
    for(auto& text:knobLabels)text.setFont(juce::Font(juce::jlimit(9.f,11.f,11.f*s)));
    auto place=[s](juce::Component&c,int x,int y,int w,int h){
        c.setBounds(juce::roundToInt(x*s),juce::roundToInt(y*s),
                    juce::roundToInt(w*s),juce::roundToInt(h*s));
    };
    place(skull,14,12,43,29);
    place(preset,339,12,220,29);
    place(mod,568,12,64,29);
    place(settings,643,12,62,29);
    for(int i=0;i<5;++i)place(tabs[size_t(i)],16+i*139,62,132,36);
    place(order,25,151,52,158);
    for(int i=0;i<7;++i) {
        place(faders[size_t(i)],93+i*62,151,49,158);
        place(faderLabels[size_t(i)],88+i*62,307,59,21);
        const int x=121+i*82;
        place(knobs[size_t(i)],x,351,60,53);
        place(knobLabels[size_t(i)],x-7,405,76,18);
    }
    // Proper rows: no text, labels or dropdowns may overlap. Same layout
    // regardless of current mode (including legacy states restored by a host).
    place(target,148,359,164,25);
    place(morph,356,359,319,25);
    place(shape,148,397,164,24);
    place(sync,331,397,108,24);
    updatePage();
}
void SkellFlangAudioProcessorEditor::updatePage() {
    mod.setButtonText(modulationOpen?"BACK":"MOD");
    mod.setToggleState(modulationOpen,juce::dontSendNotification);
    for(int i=0;i<7;++i) {
        knobs[size_t(i)].setVisible(!modulationOpen);
        knobLabels[size_t(i)].setVisible(!modulationOpen);
    }
    target.setVisible(modulationOpen);
    morph.setVisible(modulationOpen);
    shape.setVisible(modulationOpen);
    sync.setVisible(modulationOpen);
}
void SkellFlangAudioProcessorEditor::updateMode() {
    const auto* raw=processor.parameters.getRawParameterValue("mode");
    const int selected=std::clamp(raw?int(raw->load(std::memory_order_relaxed)):0,0,7);
    if(selected==shownMode)return;
    shownMode=selected;
    skin.accent=theme==0?tints[selected]:
                theme==1?juce::Colour::fromRGB(120,241,113):
                         juce::Colour::fromRGB(237,210,153);
    for(auto& slider:faders)slider.repaint();
    for(auto& slider:knobs)slider.repaint();
    for(auto& tab:tabs)tab.repaint();
    for(auto* button:{&skull,&settings,&mod,&preset,&sync})button->repaint();
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
    const juce::String name=processor.parameters.state
        .getProperty("presetName","INIT").toString();
    if(preset.getButtonText()!=name)preset.setButtonText(name);
    if(const auto* current=processor.parameters.getRawParameterValue("morphTarget")) {
        const int targetMode=std::clamp(int(current->load(std::memory_order_relaxed)),0,7);
        if(target.getSelectedId()!=targetMode+1)
            target.setSelectedId(targetMode+1,juce::dontSendNotification);
    }
    const float s=getWidth()/float(baseW);
    repaint(juce::roundToInt(536*s),juce::roundToInt(111*s),
            juce::roundToInt(171*s),juce::roundToInt(221*s));
    repaint(0,juce::roundToInt(426*s),getWidth(),
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
    g.drawLine(14.f,51.f,706.f,51.f,.85f);
    // Distinct five-mode navigation; legacy project modes remain in the DSP
    // but do not appear as selectable tabs.
    for(int i=0;i<5;++i)if(shownMode==visibleModes[size_t(i)]) {
        g.setColour(skin.accent.withAlpha(.90f));
        g.fillRoundedRectangle(31.f+139.f*i,101.f,101.f,2.f,1.f);
        break;
    }

    panelRect(g,14,111,516,221);
    panelRect(g,538,111,169,221);
    panelRect(g,14,342,693,82);

    // The clickable vector skull lives in the header; no duplicate ornament.
    const auto glow=skin.accent;
    g.setColour(bone);g.setFont(juce::Font(20.f,juce::Font::bold));
    g.drawText("SKELL-FLANG",74,8,245,29,juce::Justification::centredLeft);
    g.setColour(glow.withAlpha(.9f));g.setFont(juce::Font(10.5f));
    g.drawText("BEND THE BONES  /  MODULATION ENGINE",76,34,244,12,
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
    // Ghost ribs and central spine remain faint under a live waveform.
    // They are static original vectors, not simulated audio or a bitmap.
    const float centreX=scope.getCentreX();
    g.setColour(juce::Colour::fromRGB(51,107,94).withAlpha(.25f));
    g.drawLine(centreX,scope.getY()+11.f,centreX,scope.getBottom()-11.f,1.f);
    for(int i=0;i<6;++i) {
        const float yy=scope.getY()+19.f+float(i)*14.4f;
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
        const float yy=scope.getCentreY()-std::clamp(value,-1.f,1.f)*43.f;
        if(i==0)signal.startNewSubPath(xx,yy);else signal.lineTo(xx,yy);
    }
    g.setColour(glow.withAlpha(.90f));
    g.strokePath(signal,juce::PathStrokeType(1.35f));

    g.setColour(muted);g.setFont(juce::Font(10.5f));
    const juce::String annotation=shownMode==5
        ? ("PITCH  "+juce::String(processor.meters.pitch.load(std::memory_order_relaxed),1)+" Hz")
        : ("MODE  /  "+juce::String(modes[std::clamp(shownMode,0,7)]));
    g.drawText(annotation,554,274,142,16,juce::Justification::left);
    for(int ch=0;ch<2;++ch) {
        const float peak=std::clamp(processor.meters.peaks[ch].load(std::memory_order_relaxed),0.f,1.f);
        const float y=299.f+ch*14.f;
        g.setColour(muted);g.setFont(juce::Font(9.5f));
        g.drawText(ch==0?"L":"R",553,juce::roundToInt(y-5.f),13,15,
                   juce::Justification::centred);
        g.setColour(juce::Colour::fromRGB(27,50,49));
        g.fillRoundedRectangle(569.f,y,121.f,5.f,2.f);
        g.setColour(peak>.94f?juce::Colour::fromRGB(238,161,103):glow);
        if(peak>0.f)g.fillRoundedRectangle(569.f,y,121.f*peak,5.f,2.f);
    }

    g.setColour(muted);
    if(!modulationOpen) {
        g.setFont(juce::Font(11.f,juce::Font::bold));
        g.drawFittedText("03 / TONE",27,357,84,20,
                         juce::Justification::centredLeft,1);
        g.setFont(juce::Font(9.6f));
        g.drawFittedText("SOUND SHAPE",27,383,90,18,
                         juce::Justification::centredLeft,1);
    } else {
        g.setFont(juce::Font(11.f,juce::Font::bold));
        g.drawFittedText("03 / MOD",27,359,96,18,
                         juce::Justification::centredLeft,1);
        g.setFont(juce::Font(9.5f));
        g.drawFittedText("MORPH + LFO",27,388,99,16,
                         juce::Justification::centredLeft,1);
        g.setColour(glow.withAlpha(.92f));
        g.drawText("DESTINATION",148,345,142,12,juce::Justification::centredLeft);
        g.drawText("MORPH",356,345,110,12,juce::Justification::centredLeft);
        g.drawText("WAVEFORM",148,384,142,11,juce::Justification::centredLeft);
        g.drawText("TEMPO",331,384,100,11,juce::Justification::centredLeft);
    }

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
