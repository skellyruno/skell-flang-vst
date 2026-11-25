#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>

namespace skellflang::classicchorus {
using State = std::array<int,12>;
inline constexpr State maximum{{1024,5000,1024,5000,5000,5000,1024,1024,1024,100,5000,1024}};
inline constexpr State defaults{{512,2250,341,1500,2500,3500,23,0,0,0,2500,0}};
inline constexpr std::array<const char*,12> ids{{"nativeChorusDelay","nativeChorusDepth","nativeChorusStereo","nativeChorusLfo1Rate","nativeChorusLfo2Rate","nativeChorusLfo3Rate","nativeChorusLfo1Wave","nativeChorusLfo2Wave","nativeChorusLfo3Wave","nativeChorusCrossType","nativeChorusCrossCutoff","nativeChorusWetOnly"}};
inline constexpr std::array<const char*,12> names{{"Delay","Depth","Stereo","LFO 1 rate","LFO 2 rate","LFO 3 rate","LFO 1 wave","LFO 2 wave","LFO 3 wave","Cross type","Cross cutoff","Wet only"}};
inline constexpr std::array<const char*,12> shortNames{{"DELAY","DEPTH","STEREO","LFO 1","LFO 2","LFO 3","WAVE 1","WAVE 2","WAVE 3","CROSS","CUTOFF","WET ONLY"}};
inline bool valid(const State& s) noexcept {for(std::size_t k=0;k<s.size();++k)if(s[k]<0||s[k]>maximum[k])return false;return true;}
inline State migrate(float depth,float speed,float delay,float spread) noexcept {
    const auto unit=[](float v){return std::clamp(std::isfinite(v)?v:0.f,0.f,1.f);};
    State result=defaults;
    result[0]=int(std::lround(1024.f*unit(delay)));
    result[1]=int(std::lround(5000.f*unit(depth)));
    result[2]=int(std::lround(1024.f*unit(spread)));
    const float rate=unit(speed);
    result[3]=int(std::lround(5000.f*rate*.6f));
    result[4]=int(std::lround(5000.f*rate));
    result[5]=int(std::lround(5000.f*std::min(1.f,rate*1.4f)));
    return result;
}
// Native LE version 1 (or 0) + 12 integers. FLPresetImport.h validates the outer FST event envelope.
using Record=std::array<std::uint8_t,52>;
inline bool decode(const void* data,std::size_t n,State& state) noexcept {
    if(!data||n!=52)return false;
    const auto* b=static_cast<const std::uint8_t*>(data);
    const auto le=[&](std::size_t i){std::uint32_t v=0;for(unsigned k=0;k<4;++k)v|=std::uint32_t(b[4*i+k])<<(8*k);return v;};
    if(le(0)>1)return false;
    State s{};for(std::size_t i=0;i<s.size();++i){const auto v=le(i+1);if(v>std::uint32_t(maximum[i]))return false;s[i]=int(v);}
    state=s;return true;
}
inline bool encode(const State& state,Record& record) noexcept {
    if(!valid(state))return false;
    Record r{};r[0]=1;for(std::size_t i=0;i<state.size();++i)for(unsigned k=0;k<4;++k)r[4*(i+1)+k]=std::uint8_t(std::uint32_t(state[i])>>(8*k));
    record=r;return true;
}
}
