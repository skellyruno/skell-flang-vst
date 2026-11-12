#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr int baseW=720,baseH=500;
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
void panelRect(juce::Graphics&g,float x,float y,float w,float h,
               const FlangLookAndFeel& skin){
    g.setColour(skin.surface);
    g.fillRoundedRectangle(x,y,w,h,9.f);
    g.setColour(skin.outline);
    g.drawRoundedRectangle(x,y,w,h,9.f,.9f);
}
}

void FlangLookAndFeel::setTheme(int style) {
    themeIndex=juce::jlimit(0,2,style);
    if(themeIndex==0) { // midnight matte
        background=juce::Colour::fromRGB(7,12,14);
        surface=juce::Colour::fromRGB(17,26,29);
        foreground=juce::Colour::fromRGB(226,239,230);
        secondary=juce::Colour::fromRGB(143,169,164);
        outline=juce::Colour::fromRGB(35,60,56);
        recess=juce::Colour::fromRGB(8,17,19);
    } else if(themeIndex==1) { // graphite metal
        background=juce::Colour::fromRGB(27,33,40);
        surface=juce::Colour::fromRGB(40,48,57);
        foreground=juce::Colour::fromRGB(240,241,236);
        secondary=juce::Colour::fromRGB(175,185,185);
        outline=juce::Colour::fromRGB(76,92,102);
        recess=juce::Colour::fromRGB(22,31,38);
    } else { // warm bone / daylight
        background=juce::Colour::fromRGB(225,221,208);
        surface=juce::Colour::fromRGB(245,242,230);
        foreground=juce::Colour::fromRGB(35,42,42);
        secondary=juce::Colour::fromRGB(88,103,102);
        outline=juce::Colour::fromRGB(152,162,150);
        recess=juce::Colour::fromRGB(205,206,194);
    }
    setColour(juce::PopupMenu::backgroundColourId,surface);
    setColour(juce::PopupMenu::textColourId,foreground);
    setColour(juce::PopupMenu::headerTextColourId,foreground);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,accent.withAlpha(.23f));
    setColour(juce::PopupMenu::highlightedTextColourId,foreground);
}

void FlangLookAndFeel::drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,
                                       float pos,float,float,juce::Slider::SliderStyle style,juce::Slider& control) {
    // LinearHorizontal is used by MORPH. It cannot share a vertical thumb
    // coordinate: JUCE's slider position is an X coordinate in this mode.
    if(style==juce::Slider::LinearHorizontal) {
        const float centreY=float(y)+float(h)*.5f;
        const float start=float(x)+11.f;
        const float finish=float(x+w)-11.f;
        const float thumbX=std::clamp(pos,start,finish);
        g.setColour(recess);
        g.fillRoundedRectangle(start,centreY-3.f,finish-start,6.f,3.f);
        g.setColour(outline);
        g.drawRoundedRectangle(start,centreY-3.f,finish-start,6.f,3.f,.8f);
        for(int i=0;i<=8;++i) {
            const float px=start+(finish-start)*float(i)/8.f;
            g.setColour(secondary.withAlpha(.50f));
            g.drawLine(px,centreY-8.f,px,centreY-6.f,.75f);
        }
        if(thumbX>start) {
            g.setColour(accent.withAlpha(.90f));
            g.fillRoundedRectangle(start,centreY-2.f,thumbX-start,4.f,2.f);
        }
        const auto handle=juce::Rectangle<float>(thumbX-7.f,centreY-9.f,14.f,18.f);
        g.setColour(recess.brighter(.18f));
        g.fillRoundedRectangle(handle,3.6f);
        g.setColour(accent.withAlpha(.93f));
        g.drawRoundedRectangle(handle,3.6f,1.1f);
        g.setColour(foreground);
        g.drawLine(thumbX,centreY-4.f,thumbX,centreY+4.f,1.4f);
        return;
    }
    // Reference-style miniature hardware console fader. All geometry is
    // vector-drawn in local slider coordinates, so every JUCE editor size
    // stays sharp, and the host/APVTS value and gesture remain untouched.
    const float cx=float(x)+float(w)*.5f;
    const float railTop=float(y)+20.f;
    const float railBottom=float(y+h)-15.f;
    const float railLength=std::max(1.f,railBottom-railTop);
    const bool daylight=themeIndex==2;
    const juce::Colour wellTop=daylight?juce::Colour::fromRGB(203,207,199)
                                       :juce::Colour::fromRGB(24,34,34);
    const juce::Colour wellBottom=daylight?juce::Colour::fromRGB(187,193,185)
                                          :juce::Colour::fromRGB(10,18,20);
    const auto well=juce::Rectangle<float>(float(x)+1.f,float(y)+1.f,
                                           float(w)-2.f,float(h)-2.f);

    // Recessed individual channel, with a restrained rim and inner shadow.
    g.setGradientFill(juce::ColourGradient(wellTop,well.getX(),well.getY(),
                                           wellBottom,well.getX(),well.getBottom(),false));
    g.fillRoundedRectangle(well,6.f);
    g.setColour(daylight?juce::Colour::fromRGB(149,156,148)
                        :juce::Colour::fromRGB(43,63,59));
    g.drawRoundedRectangle(well,6.f,.85f);
    g.setColour((daylight?juce::Colours::white:juce::Colour::fromRGB(119,150,141))
                    .withAlpha(daylight?.35f:.15f));
    g.drawLine(well.getX()+5.f,well.getY()+1.7f,
               well.getRight()-5.f,well.getY()+1.7f,.75f);

    // Fine calibration ticks to both sides of the sliding metal shuttle.
    // They remain under the cap rather than competing with the position.
    for(int mark=0;mark<=16;++mark) {
        const float yy=railTop+float(mark)*railLength/16.f;
        const float tick=(mark%4==0)?7.f:4.f;
        g.setColour((daylight?juce::Colour::fromRGB(76,93,91)
                              :juce::Colour::fromRGB(124,153,144))
                        .withAlpha(mark%4==0?.34f:.19f));
        g.drawLine(cx-8.f-tick,yy,cx-8.f,yy,.7f);
        g.drawLine(cx+8.f,yy,cx+8.f+tick,yy,.7f);
    }
    // Precision etched rail, with a tiny specular edge.
    g.setColour(daylight?juce::Colour::fromRGB(118,128,121)
                        :juce::Colour::fromRGB(6,12,14));
    g.drawLine(cx,railTop,cx,railBottom,2.15f);
    g.setColour((daylight?juce::Colours::white:juce::Colour::fromRGB(120,145,137))
                    .withAlpha(daylight?.35f:.26f));
    g.drawLine(cx-.7f,railTop,cx-.7f,railBottom,.65f);

    const float capWidth=juce::jlimit(20.f,43.f,float(w)*.64f);
    const float capHeight=juce::jlimit(23.f,43.f,capWidth*1.18f);
    const float thumbY=std::clamp(pos,railTop+capHeight*.5f,
                                      railBottom-capHeight*.5f);
    // The position cue runs beneath the shuttle, just like the reference.
    g.setColour(accent.withAlpha(daylight?.87f:.96f));
    g.drawLine(cx,thumbY+capHeight*.48f,cx,railBottom,1.55f);

    const float left=cx-capWidth*.5f,top=thumbY-capHeight*.5f;
    juce::Path metal;
    metal.startNewSubPath(left+3.f,top);
    metal.lineTo(left+capWidth-3.f,top);
    metal.lineTo(left+capWidth,top+3.5f);
    metal.lineTo(left+capWidth,top+capHeight-6.f);
    metal.lineTo(left+capWidth-5.f,top+capHeight);
    metal.lineTo(left+5.f,top+capHeight);
    metal.lineTo(left,top+capHeight-6.f);
    metal.lineTo(left,top+3.5f);
    metal.closeSubPath();
    // Real silhouette first: dark cast shadow, bevelled graphite face, and
    // two narrow specular planes rather than a flat rounded rectangle.
    g.setColour(juce::Colours::black.withAlpha(daylight?.18f:.43f));
    g.fillPath(metal,juce::AffineTransform::translation(0.f,2.2f));
    g.setGradientFill(juce::ColourGradient(
        daylight?juce::Colour::fromRGB(116,124,121)
                :juce::Colour::fromRGB(106,119,126),
        left,top,
        daylight?juce::Colour::fromRGB(65,75,74)
                :juce::Colour::fromRGB(27,35,40),
        left,top+capHeight,false));
    g.fillPath(metal);
    g.setColour(daylight?juce::Colour::fromRGB(65,77,74)
                        :juce::Colour::fromRGB(9,16,22));
    g.strokePath(metal,juce::PathStrokeType(1.f));

    // Folded top edge and angled side facets give the cap physical depth.
    juce::Path topFacet;
    topFacet.startNewSubPath(left+3.7f,top+1.7f);
    topFacet.lineTo(left+capWidth-3.7f,top+1.7f);
    topFacet.lineTo(left+capWidth-6.f,top+6.f);
    topFacet.lineTo(left+6.f,top+6.f);
    topFacet.closeSubPath();
    g.setColour((daylight?juce::Colours::white:
                           juce::Colour::fromRGB(201,218,217)).withAlpha(.25f));
    g.fillPath(topFacet);
    g.setColour(juce::Colours::black.withAlpha(.24f));
    g.drawLine(left+3.f,top+capHeight-7.2f,
               left+capWidth-3.f,top+capHeight-7.2f,.8f);
    g.setColour(juce::Colours::white.withAlpha(daylight?.30f:.17f));
    g.drawLine(left+2.4f,top+7.f,left+2.4f,top+capHeight-8.f,.85f);
    g.setColour(juce::Colours::black.withAlpha(.32f));
    g.drawLine(left+capWidth-2.4f,top+7.f,
               left+capWidth-2.4f,top+capHeight-8.f,.8f);

    // Single luminous horizontal engraving, colour-linked to the effect.
    g.setColour(juce::Colours::black.withAlpha(.35f));
    g.drawLine(left+4.f,thumbY+1.7f,left+capWidth-4.f,thumbY+1.7f,1.f);
    g.setColour(accent.withAlpha(daylight?.89f:.98f));
    g.drawLine(left+4.5f,thumbY,left+capWidth-4.5f,thumbY,1.15f);

    // Keep the host parameter's numeric feedback, tucked into the channel
    // header without changing the reference's silhouette.
    const bool orderFader=control.getComponentID()=="fader-order";
    const auto readout=orderFader
        ? juce::String(juce::roundToInt(control.getValue()))
        : juce::String(juce::roundToInt(control.getValue()*100.))+"%";
    g.setColour((daylight?juce::Colour::fromRGB(63,76,75)
                          :juce::Colour::fromRGB(165,187,179)).withAlpha(.82f));
    g.setFont(juce::Font(juce::jlimit(8.f,10.3f,float(w)*.215f),
                         juce::Font::bold));
    g.drawText(readout,x+1,y+2,w-2,12,juce::Justification::centred);

}

void FlangLookAndFeel::drawRotarySlider(juce::Graphics&g,int x,int y,int w,int h,
                                        float position,float start,float end,juce::Slider&) {
    // Compact machined hardware dial: real stepped edge, light-catching bevels,
    // satin graphite face and engraved index. Pure vector drawing at all sizes.
    const float cx=float(x)+float(w)*.5f,cy=float(y)+float(h)*.5f;
    const float radius=float(std::min(w,h))*.318f;
    const float rimRadius=radius+3.0f;
    const bool boneTheme=themeIndex==2;
    const auto graphite=boneTheme?juce::Colour::fromRGB(89,99,95)
                                 :juce::Colour::fromRGB(47,60,63);
    const auto edgeLight=boneTheme?juce::Colour::fromRGB(203,210,195)
                                  :juce::Colour::fromRGB(145,165,166);
    const auto edgeDark=boneTheme?juce::Colour::fromRGB(70,83,79)
                                 :juce::Colour::fromRGB(17,27,31);

    // Fine, recessed calibration arc. A short accent sweep replaces the
    // oversized neon circle; ten fixed etched ticks make setting repeatable.
    juce::Path track;
    track.addCentredArc(cx,cy,rimRadius+3.5f,rimRadius+3.5f,
                        0.f,start,end,true);
    g.setColour(outline.withAlpha(.93f));
    g.strokePath(track,juce::PathStrokeType(1.7f,
                 juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    for(int i=0;i<=10;++i) {
        const float a=start+(end-start)*float(i)/10.f;
        const float inner=rimRadius+4.4f;
        const float outer=inner+(i%5==0?3.0f:1.4f);
        g.setColour(secondary.withAlpha(i%5==0?.54f:.26f));
        g.drawLine(cx+std::sin(a)*inner,cy-std::cos(a)*inner,
                   cx+std::sin(a)*outer,cy-std::cos(a)*outer,.72f);
    }
    juce::Path sweep;
    sweep.addCentredArc(cx,cy,rimRadius+3.5f,rimRadius+3.5f,0.f,
                        start,start+(end-start)*position,true);
    g.setColour(accent.withAlpha(.92f));
    g.strokePath(sweep,juce::PathStrokeType(2.05f,
                 juce::PathStrokeType::curved,juce::PathStrokeType::rounded));

    // The cast shadow is intentionally below the rim; this grounds the dial.
    g.setColour(juce::Colours::black.withAlpha(boneTheme?.19f:.52f));
    g.fillEllipse(cx-rimRadius,cy-rimRadius+2.4f,
                  rimRadius*2.f,rimRadius*2.f);
    g.setGradientFill(juce::ColourGradient(
        edgeLight,cx-rimRadius*.5f,cy-rimRadius*.9f,
        edgeDark,cx+rimRadius*.7f,cy+rimRadius,false));
    g.fillEllipse(cx-rimRadius,cy-rimRadius,rimRadius*2.f,rimRadius*2.f);
    g.setColour(edgeDark);
    g.drawEllipse(cx-rimRadius,cy-rimRadius,rimRadius*2.f,rimRadius*2.f,1.1f);

    // Small regular radial grooves read as a knurled metal grip, not an LED.
    for(int i=0;i<28;++i) {
        const float a=juce::MathConstants<float>::twoPi*float(i)/28.f;
        const float inner=radius+.8f,outer=radius+2.2f;
        g.setColour(i>=16&&i<=23?juce::Colours::black.withAlpha(.34f)
                                :juce::Colours::white.withAlpha(boneTheme?.15f:.20f));
        g.drawLine(cx+std::sin(a)*inner,cy-std::cos(a)*inner,
                   cx+std::sin(a)*outer,cy-std::cos(a)*outer,.8f);
    }

    // Broad sloping shoulder and a smaller brushed-metal central face.
    g.setGradientFill(juce::ColourGradient(
        boneTheme?juce::Colour::fromRGB(190,197,184)
                 :juce::Colour::fromRGB(116,135,135),
        cx-radius*.65f,cy-radius*.85f,
        graphite.darker(.38f),cx+radius*.65f,cy+radius*.85f,false));
    g.fillEllipse(cx-radius,cy-radius,radius*2.f,radius*2.f);
    g.setColour(edgeLight.withAlpha(.50f));
    g.drawEllipse(cx-radius+.5f,cy-radius+.5f,radius*2.f-1.f,radius*2.f-1.f,.85f);
    const float inset=radius*.79f;
    g.setGradientFill(juce::ColourGradient(
        boneTheme?juce::Colour::fromRGB(118,129,121)
                 :juce::Colour::fromRGB(53,72,73),
        cx-inset*.45f,cy-inset*.9f,
        boneTheme?juce::Colour::fromRGB(79,92,86)
                 :juce::Colour::fromRGB(16,27,30),
        cx+inset*.7f,cy+inset,false));
    g.fillEllipse(cx-inset,cy-inset,inset*2.f,inset*2.f);
    g.setColour(juce::Colours::black.withAlpha(.43f));
    g.drawEllipse(cx-inset,cy-inset,inset*2.f,inset*2.f,.8f);

    // Illuminated but narrow, recessed index slot. No central green dot.
    const float a=start+(end-start)*position;
    const float dx=std::sin(a),dy=-std::cos(a);
    g.setColour(juce::Colours::black.withAlpha(.60f));
    g.drawLine(cx+dx*inset*.28f+.5f,cy+dy*inset*.28f+.5f,
               cx+dx*inset*.90f+.5f,cy+dy*inset*.90f+.5f,3.0f);
    g.setColour(boneTheme?juce::Colour::fromRGB(236,242,225)
                        :juce::Colour::fromRGB(220,238,231));
    g.drawLine(cx+dx*inset*.28f,cy+dy*inset*.28f,
               cx+dx*inset*.87f,cy+dy*inset*.87f,1.7f);
    g.setColour(accent.withAlpha(.85f));
    g.drawLine(cx+dx*inset*.52f,cy+dy*inset*.52f,
               cx+dx*inset*.88f,cy+dy*inset*.88f,.9f);

    g.setColour(juce::Colours::white.withAlpha(boneTheme?.24f:.11f));
    g.drawLine(cx-inset*.38f,cy-inset*.66f,
               cx+inset*.30f,cy-inset*.73f,.65f);
}

void FlangLookAndFeel::drawButtonBackground(juce::Graphics&g,juce::Button&button,
                                             const juce::Colour&,bool hover,bool down) {
    const auto b=button.getLocalBounds().toFloat().reduced(1.f);
    const bool selected=button.getToggleState();
    const auto id=button.getComponentID();
    const bool modeButton=id.startsWith("mode-");
    const bool macroButton=id.startsWith("macro-");
    const bool daylight=themeIndex==2;

    if(id=="preset") {
        // One unobstructed sound-library selector, split into a wide name
        // display and a deliberately narrow dropdown trigger.
        g.setColour(juce::Colours::black.withAlpha(daylight?.10f:.27f));
        g.fillRoundedRectangle(b.translated(0.f,1.4f),5.5f);
        g.setGradientFill(juce::ColourGradient(
            daylight?juce::Colour::fromRGB(251,251,243):surface.brighter(.21f),
            b.getX(),b.getY(),
            daylight?juce::Colour::fromRGB(228,232,219):recess,
            b.getX(),b.getBottom(),false));
        g.fillRoundedRectangle(b,5.5f);
        g.setColour((hover?accent:outline).withAlpha(hover?.68f:.88f));
        g.drawRoundedRectangle(b,5.5f,.9f);
        g.setColour(accent.withAlpha(.92f));
        g.fillRoundedRectangle(b.getX()+1.4f,b.getY()+6.f,
                               2.1f,b.getHeight()-12.f,1.f);
        const float divider=b.getRight()-29.f;
        g.setColour(outline.withAlpha(.92f));
        g.drawLine(divider,b.getY()+5.f,divider,b.getBottom()-5.f,.85f);
        const float ax=b.getRight()-15.f,ay=b.getCentreY()+.6f;
        juce::Path arrow;
        arrow.startNewSubPath(ax-3.5f,ay-1.7f);
        arrow.lineTo(ax,ay+1.8f);
        arrow.lineTo(ax+3.5f,ay-1.7f);
        g.setColour(hover?accent:secondary);
        g.strokePath(arrow,juce::PathStrokeType(1.5f,
                     juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        if(down) {
            g.setColour(accent.withAlpha(.10f));
            g.fillRoundedRectangle(b.reduced(2.f),4.f);
        }
        return;
    }

    if(id=="skull"||id=="settings") {
        // Compact, precisely bounded utility buttons; no oversized square
        // around the gear. The skull remains a functional three-theme switch.
        g.setGradientFill(juce::ColourGradient(
            hover?surface.brighter(.28f):surface.brighter(.06f),
            b.getX(),b.getY(),recess,b.getRight(),b.getBottom(),false));
        g.fillRoundedRectangle(b,6.f);
        g.setColour((hover?accent:outline).withAlpha(hover?.71f:.82f));
        g.drawRoundedRectangle(b,6.f,.85f);
        if(down) {
            g.setColour(accent.withAlpha(.12f));
            g.fillRoundedRectangle(b.reduced(2.f),4.3f);
        }
        const float cx=b.getCentreX(),cy=b.getCentreY();
        if(id=="skull") {
            // Original Flang skull crest: metallic crown and jaw with short
            // phase-wave ornaments. No skeleton-key geometry is reused.
            const float u=std::min(b.getWidth()/39.f,b.getHeight()/35.f);
            juce::Path skullShape;
            skullShape.startNewSubPath(cx-7.7f*u,cy-4.f*u);
            skullShape.cubicTo(cx-10.3f*u,cy-12.6f*u,cx-3.7f*u,cy-12.f*u,
                               cx,cy-12.f*u);
            skullShape.cubicTo(cx+3.7f*u,cy-12.f*u,cx+10.3f*u,cy-12.6f*u,
                               cx+7.7f*u,cy-4.f*u);
            skullShape.lineTo(cx+8.f*u,cy+2.f*u);
            skullShape.lineTo(cx+5.3f*u,cy+4.2f*u);
            skullShape.lineTo(cx+5.3f*u,cy+9.3f*u);
            skullShape.lineTo(cx-5.3f*u,cy+9.3f*u);
            skullShape.lineTo(cx-5.3f*u,cy+4.2f*u);
            skullShape.lineTo(cx-8.f*u,cy+2.f*u);
            skullShape.closeSubPath();
            g.setGradientFill(juce::ColourGradient(
                daylight?juce::Colour::fromRGB(121,135,126)
                        :juce::Colour::fromRGB(215,233,224),
                cx-7.f*u,cy-10.f*u,
                daylight?juce::Colour::fromRGB(69,81,76)
                        :juce::Colour::fromRGB(105,139,130),
                cx+7.f*u,cy+9.f*u,false));
            g.fillPath(skullShape);
            g.setColour(accent.withAlpha(.72f));
            g.strokePath(skullShape,juce::PathStrokeType(.9f*u));
            g.setColour(recess);
            juce::Path eyes;
            eyes.startNewSubPath(cx-6.7f*u,cy-2.6f*u);
            eyes.lineTo(cx-1.5f*u,cy-1.2f*u);
            eyes.lineTo(cx-2.4f*u,cy+2.7f*u);
            eyes.lineTo(cx-6.0f*u,cy+1.6f*u);
            eyes.closeSubPath();
            eyes.startNewSubPath(cx+6.7f*u,cy-2.6f*u);
            eyes.lineTo(cx+1.5f*u,cy-1.2f*u);
            eyes.lineTo(cx+2.4f*u,cy+2.7f*u);
            eyes.lineTo(cx+6.f*u,cy+1.6f*u);
            eyes.closeSubPath();
            g.fillPath(eyes);
            juce::Path nose;
            nose.addTriangle(cx,cy+1.3f*u,cx-1.9f*u,cy+4.4f*u,
                             cx+1.9f*u,cy+4.4f*u);
            g.fillPath(nose);
            g.setColour(recess.withAlpha(.92f));
            g.drawLine(cx-4.6f*u,cy+5.2f*u,cx+4.6f*u,cy+5.2f*u,1.f*u);
            for(int n=-1;n<=1;++n) {
                const float tx=cx+float(n)*2.25f*u;
                g.drawLine(tx,cy+5.2f*u,tx,cy+8.8f*u,.7f*u);
            }
            g.setColour(accent.withAlpha(.85f));
            for(int side:{-1,1}) {
                juce::Path wave;
                const float sx=float(side);
                wave.startNewSubPath(cx+sx*10.8f*u,cy-3.5f*u);
                wave.quadraticTo(cx+sx*14.1f*u,cy-7.2f*u,
                                 cx+sx*15.4f*u,cy-3.5f*u);
                wave.quadraticTo(cx+sx*17.2f*u,cy+.7f*u,
                                 cx+sx*18.0f*u,cy-3.5f*u);
                g.strokePath(wave,juce::PathStrokeType(1.f*u));
            }
        } else {
            // Eight-tooth machined gear with a real centre aperture.
            const float r=7.9f;
            juce::Path gear;
            for(int i=0;i<32;++i) {
                const float a=juce::MathConstants<float>::twoPi*float(i)/32.f;
                const bool tooth=(i%4==1||i%4==2);
                const float rr=tooth?r+1.1f:r-1.0f;
                const float px=cx+std::cos(a)*rr,py=cy+std::sin(a)*rr;
                if(i==0)gear.startNewSubPath(px,py);else gear.lineTo(px,py);
            }
            gear.closeSubPath();
            g.setGradientFill(juce::ColourGradient(
                foreground.withAlpha(.98f),cx-6.f,cy-6.f,
                secondary.withAlpha(.91f),cx+7.f,cy+8.f,false));
            g.fillPath(gear);
            g.setColour(recess);
            g.fillEllipse(cx-3.4f,cy-3.4f,6.8f,6.8f);
            g.setColour(accent.withAlpha(.87f));
            g.drawEllipse(cx-3.0f,cy-3.0f,6.0f,6.0f,.85f);
        }
        return;
    }

    const float radius=modeButton?5.3f:(macroButton?4.f:5.f);
    g.setColour(juce::Colours::black.withAlpha(daylight?.05f:.25f));
    g.fillRoundedRectangle(b.translated(0.f,1.f),radius);
    const auto top=selected?surface.brighter(.32f):
                   (hover?surface.brighter(.22f):surface.brighter(.10f));
    const auto bottom=selected?accent.withAlpha(daylight?.08f:.12f):
                      (hover?surface:recess);
    g.setGradientFill(juce::ColourGradient(
        top,b.getX(),b.getY(),bottom,b.getX(),b.getBottom(),false));
    g.fillRoundedRectangle(b,radius);
    g.setColour(selected?accent.withAlpha(modeButton?.73f:.82f):
                hover?accent.withAlpha(.46f):outline.withAlpha(.77f));
    g.drawRoundedRectangle(b,radius,selected?1.13f:.82f);
    g.setColour(juce::Colours::white.withAlpha(daylight?.24f:.08f));
    g.drawLine(b.getX()+6.f,b.getY()+1.4f,
               b.getRight()-6.f,b.getY()+1.4f,.65f);
    if(modeButton&&selected) {
        g.setColour(accent.withAlpha(.89f));
        g.fillRoundedRectangle(b.getX()+5.1f,b.getCentreY()-4.5f,
                               1.7f,9.f,.85f);
    }
    if(down) {
        g.setColour(accent.withAlpha(.12f));
        g.fillRoundedRectangle(b.reduced(2.f),radius-1.f);
    }
}

void FlangLookAndFeel::drawButtonText(juce::Graphics& g,
                                     juce::TextButton& button,bool hover,bool down) {
    const auto id=button.getComponentID();
    if(id=="skull"||id=="settings")return;
    const auto bounds=button.getLocalBounds();
    if(id=="preset") {
        const float height=float(bounds.getHeight());
        const int pad=13;
        g.setColour(secondary.withAlpha(.90f));
        g.setFont(juce::Font(juce::jlimit(7.8f,9.5f,height*.265f),
                             juce::Font::bold));
        g.drawFittedText("SOUND LIBRARY",pad,3,bounds.getWidth()-pad-40,
                         juce::jmax(8,juce::roundToInt(height*.32f)),
                         juce::Justification::centredLeft,1);
        g.setColour(foreground.withAlpha(down?.79f:1.f));
        g.setFont(juce::Font(juce::jlimit(11.f,14.f,height*.38f),
                             juce::Font::bold));
        g.drawFittedText(button.getButtonText(),pad,juce::roundToInt(height*.39f),
                          bounds.getWidth()-pad-43,
                          juce::jmax(12,juce::roundToInt(height*.53f)),
                          juce::Justification::centredLeft,1);
        return;
    }
    if(id.startsWith("macro-")) {
        g.setColour(button.getToggleState()?foreground:secondary);
        g.setFont(juce::Font(juce::jlimit(9.f,11.f,float(bounds.getHeight())*.43f),
                             button.getToggleState()?juce::Font::bold:juce::Font::plain));
        g.drawFittedText(button.getButtonText(),bounds.reduced(3,2),
                         juce::Justification::centred,1);
        return;
    }
    if(id.startsWith("mode-")) {
        g.setColour(button.getToggleState()?foreground:secondary.brighter(.26f));
        g.setFont(juce::Font(juce::jlimit(10.f,11.8f,float(bounds.getHeight())*.42f),
                            button.getToggleState()?juce::Font::bold:juce::Font::plain));
        g.drawFittedText(button.getButtonText(),bounds.reduced(8,2),
                         juce::Justification::centred,1);
        return;
    }
    juce::LookAndFeel_V4::drawButtonText(g,button,hover,down);
}

juce::Font FlangLookAndFeel::getPopupMenuFont() {
    return juce::Font(12.2f);
}
void FlangLookAndFeel::drawPopupMenuBackground(juce::Graphics&g,int w,int h) {
    g.setGradientFill(juce::ColourGradient(
        surface.brighter(.04f),0.f,0.f,
        surface.darker(.12f),float(w),float(h),false));
    g.fillRect(0,0,w,h);
    g.setColour(outline);
    g.drawRect(0,0,w,h,1);
    g.setColour(accent.withAlpha(.75f));
    g.fillRect(1,1,std::max(1,w-2),2);
}
void FlangLookAndFeel::drawPopupMenuSectionHeader(juce::Graphics&g,
        const juce::Rectangle<int>& area,const juce::String& title) {
    g.setColour(recess.withAlpha(.80f));
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
        g.setColour(outline);
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
    juce::Colour fg=overrideColour?*overrideColour:foreground;
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
    skin.setTheme(theme);
    for(int i=0;i<7;++i) {
        styleSlider(faders[i],true);
        faders[i].setComponentID("fader-"+juce::String(lanes[i]).toLowerCase());
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
    order.setComponentID("fader-order");
    order.setRange(1,8,1);
    order.setDoubleClickReturnValue(true,3);
    order.setTooltip("Number of stacked voices: 1 to 8");
    addAndMakeVisible(order);
    orderLink=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.parameters,"order",order);

    for(int i=0;i<7;++i) {
        styleSlider(knobs[i],false);
        knobs[i].setComponentID("tone-"+juce::String(i));
        addAndMakeVisible(knobs[i]);
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
    for(int i=0;i<4;++i) {
        auto& button=macroTabs[size_t(i)];
        button.setComponentID("macro-"+juce::String(i));
        button.setTooltip("Select a character parameter to edit");
        button.onClick=[this,i]{
            selectedMacro=i;
            updateMacroSelection();
        };
        addAndMakeVisible(button);
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
            selectedMacro=0;
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
        updateMode(); // Theme independent of selected effect and macro.
    };
    const juce::Component::SafePointer<SkellFlangAudioProcessorEditor> safeEditor(this);
    preset.onClick=[this,safeEditor] {
        juce::PopupMenu menu;
        // PopupMenu is its own top-level component in a VST host. Explicit
        // setLookAndFeel prevents the stock OS-style popup in FL Studio.
        menu.setLookAndFeel(&skin);
        menu.addSectionHeader("SKELL SOUND LIBRARY");
        const auto selected=processor.parameters.state.getProperty("presetName","INIT").toString();
        menu.addItem(1,"INIT - Neutral Start",true,selected=="INIT");
        menu.addSeparator();
        const char* groups[]{"CHORUS / ENSEMBLE","FLANGER / COMB","FLANGUS / HYBRID",
                             "ORBIT / SPATIAL","DIMENSION / DOUBLE"};
        const int offsets[]{1,4,7,10,13};
        for(int group=0;group<5;++group) {
            juce::PopupMenu sounds;
            sounds.setLookAndFeel(&skin);
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
        menu.setLookAndFeel(&skin);
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
        menu.addSeparator();
        menu.addSectionHeader("SKELLWORLD");
        menu.addItem(50,processor.worldLink.requested()
            ? "Leave SkellWorld" : "Join SkellWorld");
        menu.addItem(51,processor.worldLink.online()
            ? ("Connected - "+juce::String(processor.worldLink.peerCount())+" peers")
            : (processor.worldLink.requested()?"Connecting to local hub":"Offline - opt in to connect"),
            false,false);
        menu.showMenuAsync(juce::PopupMenu::Options()
                               .withTargetComponent(&settings).withMinimumWidth(242)
                               .withMaximumNumColumns(1),
            [safeEditor](int item){
                if(!safeEditor)return;
                if(item==50) {
                    // UI thread only; never send networking or thread actions
                    // through processBlock or JUCE parameter automation.
                    if(safeEditor->processor.worldLink.requested())
                        safeEditor->processor.worldLink.leave();
                    else
                        safeEditor->processor.worldLink.join();
                } else if(item==1) {
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
    setResizeLimits(540,375,1080,750);
    const int savedWidth=juce::jlimit(540,1080,
        int(p.parameters.state.getProperty("uiWidth",540)));
    setSize(savedWidth,juce::roundToInt(savedWidth*double(baseH)/baseW));
    updateMode();
    startTimerHz(30);
}
SkellFlangAudioProcessorEditor::~SkellFlangAudioProcessorEditor(){
    stopTimer();
    juce::PopupMenu::dismissAllActiveMenus();
    setLookAndFeel(nullptr);
}

void SkellFlangAudioProcessorEditor::resized() {
    const int currentWidth=getWidth();
    if(currentWidth>=540&&currentWidth<=1080&&
       int(processor.parameters.state.getProperty("uiWidth",540))!=currentWidth)
        processor.parameters.state.setProperty("uiWidth",currentWidth,nullptr);
    const float scale=currentWidth/float(baseW);
    const auto faderFont=juce::jlimit(9.3f,12.f,12.f*scale);
    const auto knobFont=juce::jlimit(9.f,11.5f,11.5f*scale);
    for(auto& text:faderLabels)text.setFont(juce::Font(faderFont));
    for(auto& text:knobLabels)text.setFont(juce::Font(knobFont));
    auto place=[scale](juce::Component& c,int x,int y,int w,int h){
        c.setBounds(juce::roundToInt(x*scale),juce::roundToInt(y*scale),
                    juce::roundToInt(w*scale),juce::roundToInt(h*scale));
    };
    // Same overall instrument-rack layout, with three primary tone knobs.
    // Four contextual macro parameters remain available by selection.
    place(skull,17,12,40,35);
    place(preset,389,10,251,39);
    place(settings,662,12,41,35);
    for(int i=0;i<5;++i)
        place(tabs[size_t(i)],27+i*138,77,111,29);
    place(order,25,169,52,142);
    for(int i=0;i<7;++i) {
        place(faders[size_t(i)],93+i*62,169,49,142);
        place(faderLabels[size_t(i)],88+i*62,316,59,21);
    }
    for(int i=0;i<3;++i) {
        const int x=114+i*91;
        place(knobs[size_t(i)],x,363,59,59);
        place(knobLabels[size_t(i)],x-6,426,71,17);
    }
    for(int i=3;i<7;++i) {
        place(knobs[size_t(i)],618,363,64,63);
        place(knobLabels[size_t(i)],604,427,92,17);
    }
    for(int i=0;i<4;++i) {
        const int column=i%2, row=i/2;
        place(macroTabs[size_t(i)],383+column*108,374+row*29,98,25);
    }
    place(target,53,453,130,26);
    place(morph,238,453,183,26);
    place(shape,478,453,97,26);
    place(sync,591,453,100,26);
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
    const bool modeChanged=selected!=shownMode;
    if(!modeChanged&&theme==shownTheme)return;
    shownMode=selected;
    shownTheme=theme;
    // Every effect retains a separate identity color regardless of the skull skin.
    skin.setTheme(theme);
    skin.accent=tints[selected]; // unchanged on skull click
    skin.setColour(juce::PopupMenu::highlightedBackgroundColourId,skin.accent.withAlpha(.23f));
    // Components need actual palette updates; changing just the mode tint
    // previously left the entire UI in the original midnight theme.
    for(auto& l:faderLabels)l.setColour(juce::Label::textColourId,skin.secondary);
    for(auto& l:knobLabels)l.setColour(juce::Label::textColourId,skin.secondary);
    for(auto* drop:{&target,&shape}) {
        drop->setColour(juce::ComboBox::backgroundColourId,skin.recess);
        drop->setColour(juce::ComboBox::textColourId,skin.foreground);
        drop->setColour(juce::ComboBox::outlineColourId,skin.outline);
        drop->setColour(juce::ComboBox::arrowColourId,skin.accent);
    }
    for(auto& t:tabs){
        t.setColour(juce::TextButton::textColourOffId,skin.foreground);
        t.setColour(juce::TextButton::textColourOnId,skin.foreground);
    }
    for(auto& t:macroTabs) {
        t.setColour(juce::TextButton::textColourOffId,skin.secondary);
        t.setColour(juce::TextButton::textColourOnId,skin.foreground);
        t.repaint();
    }
    for(auto* t:{&preset,&settings,&skull,&sync}){
        t->setColour(juce::TextButton::textColourOffId,skin.foreground);
        t->setColour(juce::TextButton::textColourOnId,skin.foreground);
    }
    for(auto& slider:faders)slider.repaint();
    for(auto& slider:knobs)slider.repaint();
    for(auto& tab:tabs)tab.repaint();
    for(auto* button:{&skull,&settings,&preset,&sync})button->repaint();
    order.repaint();morph.repaint();
    for(int i=0;i<5;++i)
        tabs[size_t(i)].setToggleState(visibleModes[size_t(i)]==selected,
                                       juce::dontSendNotification);
    // The user's selected macro remains attached to its original VST3 ID.
    // Host automation still updates all four macros, even if not visible.
    if(modeChanged)selectedMacro=0;
    for(int i=0;i<4;++i) {
        label(knobLabels[size_t(3+i)],knobNames[selected][i]);
        knobLabels[size_t(3+i)].setFont(juce::Font(
            juce::jlimit(9.f,11.f,11.f*getWidth()/float(baseW))));
        knobLabels[size_t(3+i)].setColour(juce::Label::textColourId,skin.secondary);
        macroTabs[size_t(i)].setButtonText(knobNames[selected][i]);
    }
    updateMacroSelection();
    repaint();
}
void SkellFlangAudioProcessorEditor::updateMacroSelection() {
    for(int i=0;i<4;++i) {
        const bool selected=(i==selectedMacro);
        macroTabs[size_t(i)].setToggleState(selected,juce::dontSendNotification);
        macroTabs[size_t(i)].repaint();
        knobs[size_t(3+i)].setVisible(selected);
        knobLabels[size_t(3+i)].setVisible(selected);
        if(selected)knobs[size_t(3+i)].toFront(false);
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
    const juce::String worldTip=processor.worldLink.requested()
        ? (processor.worldLink.online()
            ? "SkellWorld connected - "+juce::String(processor.worldLink.peerCount())+" peers"
            : "SkellWorld waiting for local hub")
        : "Settings / window size / optional SkellWorld";
    if(settings.getTooltip()!=worldTip)settings.setTooltip(worldTip);
    const float s=getWidth()/float(baseW);
    repaint(0,juce::roundToInt(50*s),getWidth(),juce::roundToInt(15*s));
    repaint(juce::roundToInt(536*s),juce::roundToInt(124*s),
            juce::roundToInt(171*s),juce::roundToInt(219*s));
    repaint(0,juce::roundToInt(485*s),getWidth(),
            std::max(1,getHeight()-juce::roundToInt(426*s)));
}
void SkellFlangAudioProcessorEditor::paint(juce::Graphics& g) {
    const float s=getWidth()/float(baseW);
    g.fillAll(skin.background);
    g.saveState();
    g.addTransform(juce::AffineTransform::scale(s));

    // Asymmetric "instrument rack" layout: one clean fader deck, one monitor,
    // and context-sensitive controls below. No asset scaling, per-frame blurs,
    // duplicated container frames or continuous full-window gradients.
    g.setColour(skin.surface.withAlpha(skin.themeIndex==2?.98f:.42f));
    g.fillRect(0,0,baseW,48);
    g.setColour(skin.outline);
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
                       :skin.secondary.withAlpha(.29f));
        g.fillRoundedRectangle(x,54.f+(4.f-h)*.5f,
                               slot-2.3f,h,.75f);
    }
    // Selected mode is highlighted solely by its illuminated button surface.
    // Removed the redundant two- and three-line underline ornaments.
    // The control deck must be neutral machined charcoal, not a separate
    // blue/purple module. Use the same active theme palette as the rack.
    const juce::Rectangle<float> deck(14.f,124.f,516.f,218.f);
    g.setGradientFill(juce::ColourGradient(
        skin.themeIndex==2?skin.surface.darker(.03f)
                          :skin.surface.brighter(.015f),
        deck.getX(),deck.getY(),
        skin.themeIndex==2?skin.recess.brighter(.04f)
                          :skin.surface.darker(.12f),
        deck.getRight(),deck.getBottom(),false));
    g.fillRoundedRectangle(deck,9.f);
    g.setColour(skin.outline.withAlpha(.94f));
    g.drawRoundedRectangle(deck,9.f,.9f);
    g.setColour(skin.foreground.withAlpha(skin.themeIndex==2?.13f:.055f));
    g.drawLine(26.f,125.6f,519.f,125.6f,.7f);
    panelRect(g,538,124,169,218,skin);
    panelRect(g,14,351,693,132,skin);

    // The clickable vector skull lives in the header; no duplicate ornament.
    const auto glow=skin.accent;
    g.setColour(skin.foreground);g.setFont(juce::Font(20.f,juce::Font::bold));
    g.drawText("SKELL-FLANG",74,8,245,29,juce::Justification::centredLeft);
    g.setColour(glow.withAlpha(.9f));g.setFont(juce::Font(10.5f));
    g.drawText("BEND THE BONES  /  MODULATION ENGINE",76,34,244,12,
               juce::Justification::centredLeft);

    g.setColour(skin.foreground.withAlpha(.86f));
    g.setFont(juce::Font(10.5f,juce::Font::bold));
    g.drawText("01  /  CONTROL DECK",27,130,170,16,juce::Justification::left);
    g.setColour(skin.secondary);
    g.setFont(juce::Font(10.5f));
    g.drawText("ORD",24,316,54,20,juce::Justification::centred);
    g.drawText("02  /  SIGNAL",550,130,143,16,juce::Justification::left);

    // Small technical divider motifs make the layout distinctive, not busy.
    g.setColour(glow.withAlpha(.50f));
    g.fillRoundedRectangle(27.f,154.f,33.f,1.3f,.5f);
    g.fillRoundedRectangle(550.f,154.f,27.f,1.3f,.5f);

    const juce::Rectangle<float> scope(552.f,162.f,141.f,135.f);
    g.setColour(skin.recess);
    g.fillRoundedRectangle(scope,5.f);
    g.setColour(skin.outline);
    g.drawRoundedRectangle(scope,5.f,.8f);
    // Ghost ribs and central spine remain faint under a live waveform.
    // They are static original vectors, not simulated audio or a bitmap.
    const float centreX=scope.getCentreX();
    g.setColour(skin.secondary.withAlpha(.20f));
    g.drawLine(centreX,scope.getY()+11.f,centreX,scope.getBottom()-11.f,1.f);
    for(int i=0;i<6;++i) {
        const float yy=scope.getY()+22.f+float(i)*17.2f;
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
    g.setColour(skin.secondary.withAlpha(.28f));
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

    g.setColour(skin.secondary);g.setFont(juce::Font(10.5f));
    const juce::String annotation=shownMode==5
        ? ("PITCH  "+juce::String(processor.meters.pitch.load(std::memory_order_relaxed),1)+" Hz")
        : ("MODE  /  "+juce::String(modes[std::clamp(shownMode,0,7)]));
    g.drawText(annotation,554,310,142,17,juce::Justification::left);

    // Every parameter is available without entering a separate MOD page.
    g.setColour(skin.secondary);
    g.setFont(juce::Font(10.5f,juce::Font::bold));
    g.drawFittedText("03 / TONE",27,367,84,15,
                     juce::Justification::centredLeft,1);
    g.setFont(juce::Font(9.4f));
    g.drawFittedText("ESSENTIALS",27,394,92,13,
                     juce::Justification::centredLeft,1);
    // Separate the three always-visible tone knobs from one focused character
    // macro. Four selector pads keep every existing DSP parameter reachable.
    g.setColour(skin.outline);
    g.drawLine(372.f,365.f,372.f,440.f,.9f);
    g.setColour(skin.secondary);
    g.setFont(juce::Font(10.f,juce::Font::bold));
    g.drawText("CHARACTER",383,357,150,15,juce::Justification::centredLeft);
    g.setColour(skin.accent.withAlpha(.78f));
    g.fillRoundedRectangle(383.f,373.f,32.f,1.2f,.6f);
    g.setColour(skin.outline);
    g.fillRoundedRectangle(27.f,446.f,669.f,1.f,.5f);
    // Inline captions do not overlap the knob labels above at compact scale.
    g.setColour(skin.accent.withAlpha(.78f));
    g.setFont(juce::Font(9.8f,juce::Font::bold));
    g.drawText("TO",29,453,23,26,juce::Justification::centredLeft);
    g.drawText("MORPH",188,453,50,26,juce::Justification::centredLeft);
    g.drawText("WAVE",429,453,47,26,juce::Justification::centredLeft);

    // Footer remains legible when the editor is scaled to compact mode.
    g.setColour(skin.secondary);g.setFont(juce::Font(11.f));
    g.drawText("LIVE  /  "+juce::String(processor.meters.bpm.load(std::memory_order_relaxed),1)+
               " BPM   /   PDC "+juce::String(processor.getLatencySamples())+
               "   /   CPU "+juce::String(processor.meters.cpu.load(std::memory_order_relaxed),1)+
               "%   /   "+juce::String(modes[std::clamp(shownMode,0,7)]),
               20,486,685,14,juce::Justification::centredLeft);
    g.restoreState();
}

juce::Font FlangLookAndFeel::getTextButtonFont(juce::TextButton&,int height) {
    return juce::Font(juce::jlimit(10.f,15.f,float(height)*.45f));
}
juce::Font FlangLookAndFeel::getComboBoxFont(juce::ComboBox& box) {
    return juce::Font(juce::jlimit(10.f,14.f,float(box.getHeight())*.46f));
}
