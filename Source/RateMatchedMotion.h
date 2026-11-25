#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
namespace skellflang {
// Match the original 48 kHz filter decay in seconds at other host rates.
// The table is prepared outside the audio thread; the hot path only interpolates.
class RateMatchedPole {
public:
    void prepare(double rate) noexcept {
        const double ratio=48000./std::max(8000.,rate);
        exact=ratio==1.;
        dcPole=float(std::pow(.995,ratio));
        for(std::size_t i=0;i<table.size();++i)
            table[i]=float(1.-std::pow(1.-double(i)/double(table.size()-1),ratio));
    }
    float coefficient(float original,float quality) const noexcept {
        if(quality<=0.f||exact)return original;
        const float index=std::clamp(original,0.f,1.f)*float(table.size()-1);
        const auto k=std::min(std::size_t(index),table.size()-2);
        const float matched=table[k]+(index-float(k))*(table[k+1]-table[k]);
        return original+quality*(matched-original);
    }
    float dc(float quality) const noexcept {return quality<=0.f||exact?.995f:.995f+quality*(dcPole-.995f);}
private:
    std::array<float,1025> table{};
    float dcPole=.995f;
    bool exact=true;
};
inline std::array<float,2> movingPan(float legacyPan,float motion,float quality) noexcept {
    if(quality<=0.f)return {1.f-legacyPan,1.f+legacyPan};
    const float pan=std::clamp(legacyPan+motion,-.85f,.85f);
    const float left=1.f-legacyPan,right=1.f+legacyPan;
    return {left+quality*(std::sqrt(1.f-pan)-left),right+quality*(std::sqrt(1.f+pan)-right)};
}
}
