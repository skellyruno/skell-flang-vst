#pragma once
#include "SkellFlangusAdapter.h"
#include "RawFlangusSettings.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace skellflang {
// Opt-in, independent full dry/wet path. Legacy mixer must not re-mix its output.
class RawFlangusPath final {
public:
    using Adapter=skell::private_research::FlangusAdapter;
    using State=Adapter::State;
    static constexpr int chunkSize=512;
    static constexpr std::array<const char*,8> ids{{"skellyRawOrder","skellyRawDepth","skellyRawSpeed","skellyRawDelay","skellyRawSpread","skellyRawCross","skellyRawDry","skellyRawWet"}};
    static constexpr const char* enabledId="skellyRawFlangusEnabled";
    bool prepare(double rate,const State& state,bool enabled) {
        ready=false;target=false;amount=0.f;validChunk=false;
        if(!std::isfinite(rate)||rate<32000.||rate>192000.||std::abs(rate-std::round(rate))>1.e-6)return false;
#if defined(SKELLFLANG_SAMPLE_CLOCK_RAMP)
        if(!adapter.prepare(static_cast<int>(rate),state,true))return false;
#else
        if(!adapter.prepare(static_cast<int>(rate),state))return false;
#endif
        step=static_cast<float>(1./(rate*.02));ready=true;target=enabled;amount=enabled?1.f:0.f;
        return true;
    }
    bool available() const noexcept{return ready;}
    void beginBlock(bool enabled,const State& state) noexcept {
        if(!ready){target=false;amount=0.f;return;}
        if((enabled||amount>0.f)&&!adapter.setState(state)){target=false;return;}
        if(enabled&&!target&&amount==0.f)adapter.reset();
        target=enabled;
    }
    void renderChunk(const float* left,const float* right,int count) noexcept {
        validChunk=false;
        if(!ready||(!target&&amount<=0.f)||!left||count<=0||count>chunkSize)return;
        for(int i=0;i<count;++i){l[i]=left[i];r[i]=right?right[i]:left[i];}
        validChunk=adapter.process(l.data(),r.data(),count);validCount=count;
    }
    std::array<float,2> blend(int offset,float inputL,float inputR,float legacyL,float legacyR,float output,float power) noexcept {
        if(!validChunk||offset<0||offset>=validCount)return {legacyL,legacyR};
        amount=target?std::min(1.f,amount+step):std::max(0.f,amount-step);
        if(amount<=0.f||power<=0.f)return {legacyL,legacyR};
        const float nl=inputL*(1.f-power)+power*output*l[offset];
        const float nr=inputR*(1.f-power)+power*output*r[offset];
        if(amount>=1.f)return {nl,nr};
        return {legacyL+amount*(nl-legacyL),legacyR+amount*(nr-legacyR)};
    }
private:
    Adapter adapter;
    std::array<float,chunkSize> l{},r{};
    float amount=0.f,step=0.f;int validCount=0;
    bool ready=false,target=false,validChunk=false;
};
}
