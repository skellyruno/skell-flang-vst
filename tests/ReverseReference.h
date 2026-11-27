#pragma once
#include "../Source/FlangDSP.h"
namespace skellflang_test {
// Frozen original owned Reverse path. Test-only control for the silent-tap optimization.
template<bool Fast> class ReverseProbe {
    using Controls=skellflang::Controls;
    using Delay=skellflang::Delay;
    struct Grain {bool active=false;int age=0,duration=1,voice=0;double start=0;};
    double sr=48000;
    std::array<Delay,2> reverse{};
    std::array<Grain,32> grains{};
    std::array<float,2049> reverseWindow{};
    int grainClock=0;uint32_t rng=0xFA11B00Du;
    unsigned random() noexcept{rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
public:
    void prepare(double rate){sr=std::max(8000.,rate);for(auto& d:reverse)d.prepare(sr,1.5);for(size_t i=0;i<reverseWindow.size();++i){const double t=double(i)/double(reverseWindow.size()-1u);reverseWindow[i]=float(.5-.5*std::cos(2*skellflang::pi*t));}grains={};grainClock=0;rng=0xFA11B00Du;}
    std::array<float,2> process(float l,float r,const Controls&p,int n) noexcept {
        if constexpr(Fast){if(skellflang::bound(p.extras[3])==0.f)return render<false>(l,r,p,n);}
        return render<true>(l,r,p,n);
    }
private:
    template<bool ReadOctave> std::array<float,2> render(float l,float r,const Controls&p,int n) noexcept {
        using skellflang::bound;using skellflang::saturate;
        const int duration=std::clamp(int(sr*.001*(30.+370.*p.extras[0])),32,int(sr*.40));
        const float overlap=1.f+7.f*bound(p.extras[1]);
        const int period=std::max(16,int(duration/(overlap*n*(.6f+2.f*p.speed))));
        if(++grainClock>=period) {
            grainClock=0;
            for(auto& g:grains)if(!g.active){
                g.active=true;g.age=0;g.duration=duration;g.voice=int(random()%unsigned(n));
                const double jitter=double(int(random()%2001)-1000)/1000.;
                g.start=std::max(4.,sr*.001*(20.+160.*p.delay)+jitter*.1*sr*p.extras[2]);
                break;
            }
        }
        float a=0,b=0,weight=0;
        for(auto& g:grains)if(g.active) {
            if(g.age>=g.duration){g.active=false;continue;}
            const float t=float(g.age)/g.duration;
            const float position=t*float(reverseWindow.size()-1u);
            const auto index=std::min(size_t(position),reverseWindow.size()-2u);
            const float blend=position-float(index);
            const float window=reverseWindow[index]+
                blend*(reverseWindow[index+1u]-reverseWindow[index]);
            const double d=g.start+2.0*g.age;
            const float pan=(g.voice%2==0?.3f:-.3f);
            const float shimmer=bound(p.extras[3]);
            const float dl=reverse[0].read(d),dr=reverse[1].read(d);
            const float octaveL=ReadOctave?reverse[0].read(g.start+3.0*g.age):0.f;
            const float octaveR=ReadOctave?reverse[1].read(g.start+3.0*g.age):0.f;
            a+=window*(dl*(1-shimmer)+octaveL*shimmer)*(1-pan);
            b+=window*(dr*(1-shimmer)+octaveR*shimmer)*(1+pan);
            weight+=window;++g.age;
        }
        if(weight>.1f){a/=weight;b/=weight;}
        reverse[0].write(saturate(l+a*p.feedback*.15f));
        reverse[1].write(saturate(r+b*p.feedback*.15f));
        return {saturate(a),saturate(b)};
    }
};
}
