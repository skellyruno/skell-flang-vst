#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace {
using State=juce::AudioProcessorValueTreeState;
using PID=juce::ParameterID;
void parameter(State::ParameterLayout& p,const char* id,const char* title,float def,float lo=0,float hi=1){
    p.add(std::make_unique<juce::AudioParameterFloat>(PID{id,1},title,juce::NormalisableRange<float>(lo,hi),def));
}
void boolean(State::ParameterLayout& p,const char* id,const char* title,bool def=false){
    p.add(std::make_unique<juce::AudioParameterBool>(PID{id,1},title,def));
}
void choice(State::ParameterLayout& p,const char* id,const char* title,juce::StringArray values,int def=0){
    p.add(std::make_unique<juce::AudioParameterChoice>(PID{id,1},title,values,def));
}
float curve(float v) {return v<.002f?0.f:std::pow(std::clamp(v,0.f,1.f),1.5f);}
}
State::ParameterLayout SkellFlangAudioProcessor::createLayout(){
    State::ParameterLayout p;
    boolean(p,"power","Power",true);
    choice(p,"mode","Mode",{"Chorus","Flanger","Vowel","Reverse","Orbit","Tuned"});
    choice(p,"morphTarget","Morph Target",{"Chorus","Flanger","Vowel","Reverse","Orbit","Tuned"},1);
    parameter(p,"morph","Morph",0);
    p.add(std::make_unique<juce::AudioParameterInt>(PID{"order",1},"Order",1,8,3));
    parameter(p,"depth","Depth",.5f);parameter(p,"speed","Speed",.3f);
    parameter(p,"delay","Delay",.5f);parameter(p,"spread","Spread",.5f);
    parameter(p,"cross","Crossfeed",.2f);parameter(p,"dry","Dry",.72f);
    parameter(p,"wet","Wet",.55f);parameter(p,"feedback","Feedback",.15f,-.9f,.9f);
    parameter(p,"tone","Tone",.67f);parameter(p,"output","Output",0,-12,12);
    for(int i=0;i<4;++i)parameter(p,("extra"+juce::String(i)).toRawUTF8(),
                                  ("Mode Control "+juce::String(i+1)).toRawUTF8(),.5f);
    choice(p,"lfoShape","LFO Shape",{"Sine","Triangle","Saw Up","Saw Down","Square"});
    boolean(p,"lfoSync","Tempo Sync");
    choice(p,"tnSource","Tune Source",{"Follow","Key"});
    choice(p,"tnRoot","Root",{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"});
    choice(p,"tnScale","Scale",{"Major","Minor","Pentatonic","Chromatic"});
    return p;
}
SkellFlangAudioProcessor::SkellFlangAudioProcessor()
: AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true)
                                .withOutput("Output",juce::AudioChannelSet::stereo(),true)),
  parameters(*this,nullptr,"SKELLFLANG",createLayout()) {
    parameters.state.setProperty("presetName","INIT",nullptr);
}
bool SkellFlangAudioProcessor::isBusesLayoutSupported(const BusesLayout& l)const {
    const auto input=l.getMainInputChannelSet();
    return (input==juce::AudioChannelSet::mono()||input==juce::AudioChannelSet::stereo())
             && l.getMainOutputChannelSet()==input;
}
void SkellFlangAudioProcessor::prepareToPlay(double sr,int) {
    rate=std::max(8000.0,sr);
    engines.prepare(rate);pitchTracker.prepare(rate);
    for(auto& smoother:controlSmoothers)smoother.prepare(rate);
    dryMix.reset(rate,.006);wetMix.reset(rate,.006);outputMix.reset(rate,.006);
    powerMix.reset(rate,.005);morphMix.reset(rate,.025);
    crossMix.reset(rate,.006);
    dryMix.setCurrentAndTargetValue(.75f);wetMix.setCurrentAndTargetValue(.55f);
    outputMix.setCurrentAndTargetValue(1.f);powerMix.setCurrentAndTargetValue(1.f);
    morphMix.setCurrentAndTargetValue(0.f);
    crossMix.setCurrentAndTargetValue(std::clamp(raw("cross"),0.f,1.f));
    active=std::clamp(int(raw("mode")),0,5);previous=active;remaining=0;
    transition=std::max(1,int(rate*.04));
    holdL=holdR=0;sampleCount=0;
    meters.scopeNext.store(0,std::memory_order_relaxed);
    for(auto& v:meters.scope)v.store(0.f,std::memory_order_relaxed);
    setLatencySamples(0); // No artificial PDC mismatch between dry and wet.
}
float SkellFlangAudioProcessor::raw(const char* id)const noexcept {
    const auto* v=parameters.getRawParameterValue(id);
    return v?v->load(std::memory_order_relaxed):0;
}
skellflang::Controls SkellFlangAudioProcessor::settings(int mode)const noexcept {
    skellflang::Controls p;
    p.mode=mode;p.order=std::clamp(int(raw("order")),1,8);
    p.depth=raw("depth");p.speed=raw("speed");p.delay=raw("delay");
    p.spread=raw("spread");p.cross=raw("cross");
    p.feedback=raw("feedback");p.tone=raw("tone");
    p.shape=int(raw("lfoShape"));p.sync=raw("lfoSync")>.5f;
    p.tuneSource=int(raw("tnSource"));p.root=int(raw("tnRoot"));p.scale=int(raw("tnScale"));
    p.extras[0]=raw("extra0");p.extras[1]=raw("extra1");
    p.extras[2]=raw("extra2");p.extras[3]=raw("extra3");
    p.bpm=meters.bpm.load(std::memory_order_relaxed);
    p.pitch=pitchTracker.pitch();p.confidence=pitchTracker.conf();
    return p;
}
void SkellFlangAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&){
    juce::ScopedNoDenormals noDenormals;
    const auto stamp=juce::Time::getHighResolutionTicks();
    const int channels=buffer.getNumChannels(),length=buffer.getNumSamples();
    if(channels<1||length<=0)return;
    if(auto* ph=getPlayHead())if(auto pos=ph->getPosition())
        if(auto bpm=pos->getBpm())if(std::isfinite(*bpm)&&*bpm>=20&&*bpm<=400)
            meters.bpm.store(float(*bpm),std::memory_order_relaxed);
    const int selected=std::clamp(int(raw("mode")),0,5);
    if(selected!=active){previous=active;active=selected;remaining=transition;}
    const int target=std::clamp(int(raw("morphTarget")),0,5);
    const auto primary=settings(active), secondary=settings(target), former=settings(previous);
    dryMix.setTargetValue(curve(raw("dry")));wetMix.setTargetValue(curve(raw("wet")));
    outputMix.setTargetValue(juce::Decibels::decibelsToGain(raw("output")));
    powerMix.setTargetValue(raw("power")>.5f?1.f:0.f);
    const float requestedMorph=std::clamp(raw("morph"),0.f,1.f);
    morphMix.setTargetValue(requestedMorph);
    crossMix.setTargetValue(std::clamp(raw("cross"),0.f,1.f));
    float peakL=0,peakR=0;
    auto* left=buffer.getWritePointer(0);
    auto* right=channels>1?buffer.getWritePointer(1):nullptr;
    for(int i=0;i<length;++i) {
        const float a=std::isfinite(left[i])?left[i]:0.f;
        const float b=right?(std::isfinite(right[i])?right[i]:0.f):a;
        if(active==5||(target==5 && requestedMorph>.0001f)||(previous==5&&remaining>0))
            pitchTracker.push(.5f*(a+b));
        auto main=controlSmoothers[size_t(active)].next(primary);
        main.pitch=pitchTracker.pitch();main.confidence=pitchTracker.conf();
        auto wet=engines.process(active,a,b,main);
        const float amount=morphMix.getNextValue();
        std::array<float,2> alternate{};
        bool haveAlternate=false;
        if(target!=active && amount>.0001f) {
            auto alt=controlSmoothers[size_t(target)].next(secondary);
            alt.pitch=main.pitch;alt.confidence=main.confidence;
            alternate=engines.process(target,a,b,alt);
            haveAlternate=true;
            const float c=std::cos(amount*float(skellflang::pi*.5));
            const float s=std::sin(amount*float(skellflang::pi*.5));
            wet={wet[0]*c+alternate[0]*s,wet[1]*c+alternate[1]*s};
        }
        if(remaining>0) {
            if(previous!=active){
                auto old=controlSmoothers[size_t(previous)].next(former);
                old.pitch=main.pitch;old.confidence=main.confidence;
                const auto previousOut=(haveAlternate && target==previous)?alternate:
                    engines.process(previous,a,b,old);
                const float t=1.f-float(remaining)/transition;
                const float c=std::cos(t*float(skellflang::pi*.5));
                const float s=std::sin(t*float(skellflang::pi*.5));
                wet={previousOut[0]*c+wet[0]*s,previousOut[1]*c+wet[1]*s};
            }
            --remaining;
        }
        // Continuous smoothing avoids zipper clicks under fast DAW automation.
        // ORBIT handles its own ear-to-ear positioning, but advance the ramp.
        const float cross=crossMix.getNextValue();
        if(active!=4)wet=skellflang::stereoCrossfeed(wet[0],wet[1],cross);
        const float dry=dryMix.getNextValue(),wetGain=wetMix.getNextValue();
        const float output=outputMix.getNextValue(),power=powerMix.getNextValue();
        const float l=a*(1-power)+power*output*(dry*a+wetGain*wet[0]);
        const float r=b*(1-power)+power*output*(dry*b+wetGain*wet[1]);
        // A fully bypassed plugin must preserve above-0dBFS input unchanged.
        const float safeL=power<=0.f?a:(std::isfinite(l)?std::clamp(l,-1.f,1.f):0.f);
        const float safeR=power<=0.f?b:(std::isfinite(r)?std::clamp(r,-1.f,1.f):0.f);
        left[i]=right?safeL:.5f*(safeL+safeR);
        if(right)right[i]=safeR;
        peakL=std::max(peakL,std::abs(safeL));peakR=std::max(peakR,std::abs(safeR));
        if((++sampleCount&31u)==0u) {
            const auto index=unsigned((sampleCount/32u)%64u);
            meters.scope[index].store(.5f*(safeL+safeR),std::memory_order_relaxed);
            meters.scopeNext.store((index+1u)%64u,std::memory_order_release);
        }
    }
    holdL=std::max(peakL,holdL*.93f);holdR=std::max(peakR,holdR*.93f);
    meters.peaks[0].store(holdL,std::memory_order_relaxed);
    meters.peaks[1].store(holdR,std::memory_order_relaxed);
    meters.pitch.store(pitchTracker.pitch(),std::memory_order_relaxed);
    meters.confidence.store(pitchTracker.conf(),std::memory_order_relaxed);
    const double processing=juce::Time::highResolutionTicksToSeconds(
        juce::Time::getHighResolutionTicks()-stamp);
    meters.cpu.store(float(std::min(999.,100.*processing/(length/rate))),std::memory_order_relaxed);
}
void SkellFlangAudioProcessor::getStateInformation(juce::MemoryBlock& m) {
    if(auto xml=parameters.copyState().createXml())copyXmlToBinary(*xml,m);
}
void SkellFlangAudioProcessor::setStateInformation(const void* ptr,int size) {
    if(auto xml=getXmlFromBinary(ptr,size)) {
        auto tree=juce::ValueTree::fromXml(*xml);
        if(tree.isValid() && tree.hasType(parameters.state.getType()))parameters.replaceState(tree);
    }
}
void SkellFlangAudioProcessor::applyFactoryPreset(int number) {
    struct Preset {const char* name;int mode,voices;float depth,speed,delay,wet,feedback;};
    static constexpr Preset library[]{
        {"INIT",0,3,.4f,.3f,.5f,.35f,0.f},
        {"Lush Choir Bus",0,6,.55f,.14f,.5f,.7f,.04f},
        {"Vocal Double Wash",0,4,.3f,.36f,.45f,.62f,.06f},
        {"Crypt Ensemble",0,8,.72f,.09f,.7f,.81f,.15f},
        {"Jet Skeleton",1,4,.81f,.59f,.15f,.9f,.73f},
        {"Through-Zero Whoosh",1,3,.85f,.1f,.65f,.95f,-.4f},
        {"Grave Ring",1,6,.92f,.78f,.12f,.82f,.82f},
        {"Talking Bones",2,5,.66f,.21f,.46f,.8f,.35f},
        {"Ghost Choir Sweep",2,5,.81f,.12f,.71f,.91f,.24f},
        {"Wah Reaper",2,3,.7f,.6f,.28f,.85f,.61f},
        {"Bloom Backwards",3,4,.31f,.32f,.55f,.96f,.1f},
        {"Spectral Tide",3,6,.54f,.4f,.75f,1.f,.22f},
        {"Rage Swell",3,8,.73f,.61f,.87f,1.f,.4f},
        {"Skull Carousel",4,4,.6f,.31f,.54f,.87f,.16f},
        {"Haunted Merry-Go-Round",4,6,.76f,.58f,.7f,.91f,.23f},
        {"Slow Moon",4,3,.28f,.06f,.47f,.76f,.05f},
        {"Key-Locked Jet",5,3,.66f,.41f,.47f,.77f,.64f},
        {"Pitch Ghost",5,2,.33f,.19f,.68f,.94f,.43f},
        {"Harmonic Chain",5,8,.59f,.27f,.52f,.95f,.8f}
    };
    const auto& p=library[std::clamp(number,0,18)];
    struct Change{const char* id;float value;};
    const Change edits[]{
        {"mode",float(p.mode)},{"order",float(p.voices)},{"depth",p.depth},
        {"speed",p.speed},{"delay",p.delay},{"wet",p.wet},{"feedback",p.feedback}
    };
    for(const auto& change:edits)if(auto* v=parameters.getParameter(change.id)) {
        v->beginChangeGesture();v->setValueNotifyingHost(v->convertTo0to1(change.value));
        v->endChangeGesture();
    }
    parameters.state.setProperty("presetName",p.name,nullptr);
}
juce::AudioProcessorEditor* SkellFlangAudioProcessor::createEditor() {
    return new SkellFlangAudioProcessorEditor(*this);
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new SkellFlangAudioProcessor();
}
