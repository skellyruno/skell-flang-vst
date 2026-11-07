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
    return 0;
}
