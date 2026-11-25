#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace skellflang {
// Replace one normalized rack contribution with a complete native dry/wet
// signal. The other layers keep their share of the common dry path.
inline std::array<float,2> replaceNativeLayer(
    std::array<float,2> rack, std::array<float,2> oldLayer,
    std::array<float,2> nativeLayer, float fraction, float ramp) noexcept {
    const float weight=std::clamp(fraction,0.f,1.f)*std::clamp(ramp,0.f,1.f);
    if(weight<=0.f)return rack;
    if(weight>=1.f)return nativeLayer;
    return {rack[0]+weight*(nativeLayer[0]-oldLayer[0]),
            rack[1]+weight*(nativeLayer[1]-oldLayer[1])};
}
}
