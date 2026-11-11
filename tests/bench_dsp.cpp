#include "../Source/FlangDSP.h"
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <iomanip>

// Standalone *test program*, not a standalone plugin. Measures actual DSP work
// in this CI runner; measurements are not Windows/FL Studio CPU guarantees.
int main() {
    using namespace skellflang;
    constexpr int warmup=8192,measure=96000;
    constexpr double rate=48000.;
    volatile double sink=0.;
    const char* modeNames[]{"Chorus","Flanger","Vowel (legacy)","Reverse (legacy)","Orbit","Tuned (legacy)","Flangus","Dimension","Morph 0->6"};
    double worst=0.;
    for(int mode=0;mode<9;++mode){
        EngineBank bank;bank.prepare(rate);
        EqualPowerBlend blend;blend.prepare();
        Controls p;p.order=8;p.depth=.85f;p.delay=.7f;
        p.feedback=.6f;p.speed=.37f;p.pitch=220.f;p.confidence=.95f;
        p.extras[0]=.7f;p.extras[1]=.8f;
        std::uint32_t rng=12345678u;
        double total=0.;
        const auto start=std::chrono::steady_clock::now();
        for(int i=0;i<warmup+measure;++i){
            rng=rng*1664525u+1013904223u;
            const float in=(float((rng>>8u)&0xFFFFu)/32768.f-1.f)*.3f;
            std::array<float,2> value{};
            if(mode==8){
                const auto a=bank.process(0,in,-in,p);
                const auto b=bank.process(6,in,-in,p);
                const auto gain=blend.gains(.5f);
                value={a[0]*gain[0]+b[0]*gain[1],
                       a[1]*gain[0]+b[1]*gain[1]};
            }else{
                value=bank.process(mode,in,-in,p);
            }
            if(i>=warmup){
                if(!std::isfinite(value[0])||!std::isfinite(value[1]))return 2;
                total+=double(value[0])*value[0]+double(value[1])*value[1];
            }
        }
        const double seconds=std::chrono::duration<double>(
            std::chrono::steady_clock::now()-start).count();
        const double measuredPercent=100.*seconds/((warmup+measure)/rate);
        worst=std::max(worst,measuredPercent);
        sink+=total;
        std::cout<<std::fixed<<std::setprecision(2)
                 <<modeNames[mode]<<": "<<measuredPercent
                 <<"% of single-core realtime at 48 kHz (CI machine), energy "
                 <<total<<"\n";
    }
    std::cout<<"Worst measured: "<<worst<<"% (non-gating observational benchmark)\n";
    if(!std::isfinite(double(sink))||double(sink)<=0.)return 3;
    return 0;
}
