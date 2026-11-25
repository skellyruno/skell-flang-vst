#pragma once
#include "SkellFlangusAdapter.h"
#include "RawFlangusSettings.h"
#include "NativeLayerMix.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace skellflang {
// Native full dry/wet path. Replace the corresponding normalized rack contribution.
class RawFlangusPath final {
public:
    using Adapter=skell::private_research::FlangusAdapter;
    using State=Adapter::State;
    static constexpr int chunkSize=512;
    static constexpr std::array<const char*,8> ids{{"skellyRawOrder","skellyRawDepth","skellyRawSpeed","skellyRawDelay","skellyRawSpread","skellyRawCross","skellyRawDry","skellyRawWet"}};
    static constexpr const char* enabledId="skellyRawFlangusEnabled";
    static constexpr bool contractMatches() noexcept {
        for(std::size_t i=0;i<8;++i)
            if(rawsettings::minimum[i]!=skell::flangusfx::rawMin[i] ||
               rawsettings::maximum[i]!=skell::flangusfx::rawMax[i] ||
               rawsettings::defaults[i]!=skell::flangusfx::defaults[i])return false;
        return true;
    }
    bool prepare(double rate,const State& state,bool enabled) {
        ready=false;target=false;rampPosition=0;validChunk=false;validCount=0;
        if(!std::isfinite(rate)||rate<32000.||rate>192000.||std::abs(rate-std::round(rate))>1.e-6)return false;
#if defined(SKELLFLANG_SAMPLE_CLOCK_RAMP)
        if(!adapter.prepare(static_cast<int>(rate),state,true))return false;
#else
        if(!adapter.prepare(static_cast<int>(rate),state))return false;
#endif
        rampLength=std::max(1,int(std::lround(rate*.02)));
        ready=true;target=enabled;rampPosition=enabled?rampLength:0;
        return true;
    }
    bool available() const noexcept{return ready;}
    // Host reset is serialized with processBlock. Retain the selection and
    // transition position, but discard all previously rendered audio.
    void reset() noexcept {
        if(ready)adapter.reset();
        validChunk=false;validCount=0;
    }
    void beginBlock(bool enabled,const State& state) noexcept {
        if(!ready){target=false;rampPosition=0;return;}
        if((enabled||rampPosition>0)&&!adapter.setState(state)){target=false;return;}
        if(enabled&&!target&&rampPosition==0)adapter.reset();
        target=enabled;
    }
    void renderChunk(const float* left,const float* right,int count) noexcept {
        validChunk=false;
        if(!ready||(!target&&rampPosition==0)||!left||count<=0||count>chunkSize)return;
        for(int i=0;i<count;++i){l[i]=left[i];r[i]=right?right[i]:left[i];}
        validChunk=adapter.process(l.data(),r.data(),count);validCount=count;
    }
    std::array<float,2> blend(int offset,float inputL,float inputR,float legacyL,float legacyR,float output,float power) noexcept {
        return blendLayer(offset,inputL,inputR,legacyL,legacyR,legacyL,legacyR,output,power,1.f);
    }
    std::array<float,2> blendLayer(int offset,float inputL,float inputR,float legacyL,float legacyR,
        float legacyLayerL,float legacyLayerR,float output,float power,float fraction) noexcept {
        if(!ready) {
            // Unsupported rate: substitute safe unprocessed audio. Never use
            // a hidden replacement algorithm or silently mute the native layer.
            return replaceNativeLayer({legacyL,legacyR},{legacyLayerL,legacyLayerR},
                {inputL*(1.f-power)+power*output*inputL,inputR*(1.f-power)+power*output*inputR},fraction,1.f);
        }
        if(!validChunk||offset<0||offset>=validCount)return {legacyL,legacyR};
        rampPosition=target?std::min(rampLength,rampPosition+1):std::max(0,rampPosition-1);
        if(rampPosition==0||power<=0.f)return {legacyL,legacyR};
        const float nl=inputL*(1.f-power)+power*output*l[offset];
        const float nr=inputR*(1.f-power)+power*output*r[offset];
        const float amount=float(rampPosition)/float(rampLength);
        return replaceNativeLayer({legacyL,legacyR},{legacyLayerL,legacyLayerR},{nl,nr},fraction,amount);
    }
private:
    Adapter adapter;
    std::array<float,chunkSize> l{},r{};
    int rampPosition=0,rampLength=1,validCount=0;
    bool ready=false,target=false,validChunk=false;
};
static_assert(RawFlangusPath::contractMatches(),"Native engine and settings codec contracts differ");
}
