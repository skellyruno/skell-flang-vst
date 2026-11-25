#pragma once
#include "ClassicChorusSettings.h"
#include "NativeLayerMix.h"
#include "core/ChorusFx.h"
#include <array>
#include <algorithm>
#include <cmath>

namespace skellflang {
// Native Classic full-mix output. No old chorus model and no added dry signal.
// Continuous control edits are clocked every 16 samples and slew for 20ms;
// discrete waveform/crossover/wet-only controls retain native switch semantics.
class NativeClassicChorusPath final {
public:
    using State=classicchorus::State;
    using Engine=skelly::fruitychorusfx::Engine;
    static constexpr int chunkSize=512;
    bool prepare(double sr,const State& s,bool active) noexcept {
        ready=false;validCount=0;target=false;position=0;
        if(!std::isfinite(sr)||sr<32000.||sr>192000.||std::abs(sr-std::round(sr))>1e-6||!classicchorus::valid(s))return false;
        engine.setRate(int(sr));engine.load(s);engine.reset();
        current=desired=initial=s;elapsed=duration=std::max(1,int(std::lround(sr*.02)));
        clock=0;ready=true;target=active;position=active?duration:0;return true;
    }
    void reset() noexcept {
        if(ready){engine.load(desired);engine.reset();current=initial=desired;elapsed=duration;clock=0;}
        validCount=0;
    }
    bool available() const noexcept {return ready;}
    void beginBlock(bool active,const State& s) noexcept {
        if(!ready)return;
        if(!classicchorus::valid(s))return;
        if(active&&!target&&position==0){desired=s;reset();}
        if(s!=desired){initial=current;desired=s;elapsed=0;}
        target=active;
    }
    void renderChunk(const float* left,const float* right,int n) noexcept {
        validCount=0;
        if(!ready||(!target&&position==0)||!left||n<=0||n>chunkSize)return;
        for(int i=0;i<n;++i){scratch[2*i]=clean(left[i]);scratch[2*i+1]=clean(right?right[i]:left[i]);}
        int off=0;
        while(off<n){
            if(clock==0)update();
            const int count=std::min(n-off,16-clock);
            engine.render(scratch.data()+2*off,scratch.data()+2*off,count);
            elapsed=std::min(duration,elapsed+count);clock=(clock+count)%16;off+=count;
        }
        validCount=n;
    }
    std::array<float,2> blend(int i,float a,float b,float rackL,float rackR,float dryL,float dryR,float output,float power,float fraction) noexcept {
        if(!ready&&power<=0.f)return {rackL,rackR};
        if(!ready)return replaceNativeLayer({rackL,rackR},{dryL,dryR},{a*(1-power)+power*output*a,b*(1-power)+power*output*b},fraction,1.f);
        if(i<0||i>=validCount)return {rackL,rackR};
        position=target?std::min(duration,position+1):std::max(0,position-1);
        if(power<=0.f)return {rackL,rackR};
        return replaceNativeLayer({rackL,rackR},{dryL,dryR},{a*(1-power)+power*output*clean(scratch[2*i]),b*(1-power)+power*output*clean(scratch[2*i+1])},fraction,float(position)/float(duration));
    }
private:
    static float clean(float x) noexcept {return std::isfinite(x)&&std::abs(x)>=1e-30f?x:0.f;}
    void update() noexcept {
        if(current==desired)return;
        const double t=double(elapsed)/double(duration);
        for(std::size_t k=0;k<current.size();++k){
            const bool discrete=(k>=6&&k<=9)||k==11;
            const int value=discrete?desired[k]:int(std::lround(initial[k]+t*(desired[k]-initial[k])));
            if(value!=current[k]){engine.setParam(int(k),value);current[k]=value;}
        }
    }
    Engine engine;
    State current=classicchorus::defaults,desired=classicchorus::defaults,initial=classicchorus::defaults;
    std::array<float,chunkSize*2> scratch{};
    bool ready=false,target=false;
    int duration=1,position=0,elapsed=1,clock=0,validCount=0;
};
constexpr bool classicContractMatches() noexcept {
    for(std::size_t k=0;k<classicchorus::defaults.size();++k)
        if(classicchorus::maximum[k]!=skelly::fruitychorusfx::rawMax[k]||classicchorus::defaults[k]!=skelly::fruitychorusfx::defaults[k])return false;
    return true;
}
static_assert(classicContractMatches(),"Classic raw contract must match supplied source");
}
