#include "../Source/FlangDSP.h"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#define REQUIRE(x) do { if(!(x)) throw std::runtime_error("FAILED: " #x); } while(false)
int main(){
    using namespace skellflang;
    Delay d;d.prepare(48000,1);
    d.write(1);d.write(0);d.write(0);
    REQUIRE(std::abs(d.read(3)-1)<.03f);
    PitchTracker pitch;pitch.prepare(48000);
    for(int i=0;i<48000;++i)pitch.push(float(std::sin(2*pi*220.*i/48000.)));
    std::cout<<"f0="<<pitch.pitch()<<" confidence="<<pitch.conf()<<"\n";
    REQUIRE(pitch.pitch()>200&&pitch.pitch()<240);
    REQUIRE(pitch.conf()>.75);
    for(int m=0;m<6;++m){
        EngineBank bank;bank.prepare(48000);
        Controls p;p.mode=m;p.order=8;p.depth=.95f;p.delay=.8f;p.feedback=.9f;
        p.confidence=.96f;p.pitch=220.f;p.extras[0]=.7f;p.extras[1]=.8f;
        double energy=0;
        for(int i=0;i<96000;++i) {
            const float input=i<120?1.f:float(.25*std::sin(2*pi*110.*i/48000.));
            const auto y=bank.process(m,input,input,p);
            for(float a:y){REQUIRE(std::isfinite(a));REQUIRE(std::abs(a)<=1.001f);energy+=a*a;}
        }
        REQUIRE(energy>.01);
        std::cout<<"mode "<<m<<" energy "<<energy<<"\n";
    }
    // Exercise feedback extremes, alternate sample rates and abrupt tuning changes.
    for(const double sr:{44100.,96000.,192000.}) {
        EngineBank bank;bank.prepare(sr);
        Controls p;p.order=8;p.feedback=-.9f;p.extras[0]=.95f;p.extras[1]=.98f;p.extras[3]=1.f;
        p.pitch=220;p.confidence=.99f;p.tuneSource=0;p.scale=0;
        for(int m=0;m<6;++m) {
            double peak=0;
            for(int i=0;i<8192;++i) {
                const float x=i<64?1.f:float(.2*std::sin(2*pi*220*i/sr));
                if(i>4096)p.pitch=440;
                const auto y=bank.process(m,x,-x,p);
                for(const float sample:y){REQUIRE(std::isfinite(sample));peak=std::max(peak,double(std::abs(sample)));}
            }
            REQUIRE(peak<=1.001);
        }
    }
    // CROSS must actually swap the wet channels at 100%, preserve mono energy,
    // and stay finite for all supported automation values.
    for(float x:{0.f,.2f,.5f,1.f}) {
        const auto cross=stereoCrossfeed(.75f,-.25f,x);
        REQUIRE(std::isfinite(cross[0]) && std::isfinite(cross[1]));
        REQUIRE(std::abs(cross[0]+cross[1]-.5f)<1.e-5f);
    }
    const auto swapped=stereoCrossfeed(.75f,-.25f,1.f);
    REQUIRE(std::abs(swapped[0]+.25f)<1.e-5f);
    REQUIRE(std::abs(swapped[1]-.75f)<1.e-5f);
    // Silence must clear pitch confidence; don't tune resonators to stale notes.
    PitchTracker gated;gated.prepare(48000);
    for(int i=0;i<48000;++i)gated.push(float(std::sin(2*pi*220*i/48000.)));
    REQUIRE(gated.conf()>.75f);
    for(int i=0;i<16000;++i)gated.push(0.f);
    REQUIRE(gated.conf()<.1f);
    return 0;
}
