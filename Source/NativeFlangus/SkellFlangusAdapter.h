// Copyright retained by the Skelly team and respective rights holders.
// Skelly Licensing: provisional ownership notice; definitive terms pending.
// No additional redistribution permission is granted by this notice.
#pragma once
#include "core/FlangusFastDist.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
namespace skell::private_research {
// Local opt-in adapter only; no production parameter IDs or preset migration.
class FlangusAdapter final {
public:
 using State=flangusfx::State;
 static bool valid(const State& s) noexcept { for(int i=0;i<flangusfx::params;++i) if(s[i]<flangusfx::rawMin[i]||s[i]>flangusfx::rawMax[i])return false;return true; }
 static bool decodeState(const std::uint8_t* p,std::size_t size,State& out) noexcept {
  if(!p||size!=32)return false;
  State s{};for(int i=0;i<8;++i){std::uint32_t u=0;for(int j=0;j<4;++j)u|=std::uint32_t(p[4*i+j])<<(8*j);const auto v=static_cast<std::int64_t>(u)-(u>=0x80000000u?0x100000000LL:0LL);if(v<flangusfx::rawMin[i]||v>flangusfx::rawMax[i])return false;s[i]=static_cast<int>(v);}out=s;return true;
 }
 static std::array<std::uint8_t,32> encodeState(const State& s) noexcept {std::array<std::uint8_t,32> a{};for(int i=0;i<8;++i){const auto u=static_cast<std::uint32_t>(s[i]);for(int j=0;j<4;++j)a[4*i+j]=static_cast<std::uint8_t>(u>>(8*j));}return a;}
 bool prepare(int sampleRate,const State& s,bool deterministicGainRamp=false){if(sampleRate<32000||sampleRate>192000||!valid(s))return false;engine.prepare(sampleRate);engine.setDeterministicGainRamp(deterministicGainRamp);engine.setBlockSize(512);engine.load(s);engine.flush();current=s;ready=true;return true;}
 // Changes are audio-thread-owned, caller serializes access. Same-value updates don't reset phase.
 bool setState(const State& s) noexcept {if(!ready||!valid(s))return false;for(int i=0;i<8;++i)if(s[i]!=current[i])engine.setParam(i,s[i]);current=s;return true;}
 void reset() noexcept {if(ready)engine.flush();}
 bool process(float* left,float* right,int n) noexcept {
  if(!ready||!left||!right||left==right||n<0)return false;
  for(int o=0;o<n;){const int m=std::min(512,n-o);for(int i=0;i<m;++i){scratch[2*i]=clean(left[o+i]);scratch[2*i+1]=clean(right[o+i]);}engine.render(scratch.data(),scratch.data(),m);for(int i=0;i<m;++i){left[o+i]=clean(scratch[2*i]);right[o+i]=clean(scratch[2*i+1]);}o+=m;}return true;
 }
 static constexpr int latencySamples() noexcept{return 0;}
private:
 static float clean(float x) noexcept {return std::isfinite(x)&&std::abs(x)>=1e-30f?x:0.f;}
 flangusfx::Engine engine;State current{};std::array<float,1024> scratch{};bool ready=false;
};
}
