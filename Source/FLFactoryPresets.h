#pragma once
#include "ClassicChorusSettings.h"
#include "RawFlangusSettings.h"
namespace skellflang::flfactory {
inline constexpr int chorusDefaultIndex=16,chorusLowEnhanceIndex=17,flangusDefaultIndex=18;
inline constexpr const char* chorusDefaultName="FL Chorus Default";
inline constexpr const char* chorusLowEnhanceName="FL Low Enhance";
inline constexpr const char* flangusDefaultName="FL Flangus Default";
// Exact native integers from supplied FL 26.1.6 single-effect presets.
inline constexpr classicchorus::State chorusDefault{{512,2250,341,1500,2500,3500,23,0,0,0,2500,0}};
inline constexpr classicchorus::State chorusLowEnhance{{243,4336,416,0,0,781,0,0,1024,65,4635,96}};
inline constexpr rawsettings::State flangusDefault{{4,512,768,8,128,-32,96,96}};
}
