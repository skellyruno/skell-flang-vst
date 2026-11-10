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

    // Exercise automation jumps: the interpolated delay and feedback control
    // must remain finite under extreme real-time changes and both signs of FB.
    for(const double rate : {44100.,96000.}) {
        EngineBank bank; bank.prepare(rate);
        Controls c; c.order=8; c.speed=.95f; c.depth=.9f;
        double difference=0;
        for(int i=0;i<32768;++i) {
            c.delay=(i/37)%2 ? 0.f : 1.f;
            c.feedback=(i/113)%2 ? -.9f : .9f;
            c.extras[0]=(i/71)%2 ? 0.f : 1.f;
            const float input=float(.15*std::sin(2*pi*330*i/rate));
            const auto result=bank.process(1,input,-input,c);
            REQUIRE(std::isfinite(result[0])&&std::isfinite(result[1]));
            REQUIRE(std::abs(result[0])<=1.001f&&std::abs(result[1])<=1.001f);
            difference+=std::abs(result[0]-result[1]);
        }
        REQUIRE(difference>.01);
    }
    // SWEEP RANGE must audibly (numerically) change the resulting flanger.
    {
        EngineBank narrow,wide;narrow.prepare(48000);wide.prepare(48000);
        Controls a,b;a.order=b.order=3;a.depth=b.depth=.9f;
        a.speed=b.speed=.25f;a.delay=b.delay=.22f;
        a.extras[0]=0.f;b.extras[0]=1.f;
        double delta=0;
        for(int i=0;i<16384;++i){
            const float x=float(.25*std::sin(2*pi*210*i/48000.));
            const auto y=narrow.process(1,x,x,a),z=wide.process(1,x,x,b);
            delta+=std::abs(y[0]-z[0]);
        }
        REQUIRE(delta>0.01);
    }


    // Regression: pitch tracking stays reliable across 96/192 kHz sessions,
    // including the low register that a fixed /4 decimator would miss.
    for(const double sr : {96000.,192000.}) {
        for(const double f : {82.4069,220.0}) {
            PitchTracker tracker; tracker.prepare(sr);
            for(int i=0;i<int(sr*.65);++i)
                tracker.push(float(.6*std::sin(2*pi*f*i/sr)));
            REQUIRE(std::abs(tracker.pitch()-f)<f*.10);
            REQUIRE(tracker.conf()>.78f);
        }
    }

    // Controls must start moving the first sample, but never jump a whole block.
    for(double sr:{44100.,96000.,192000.}) {
        ControlSmoother smoother;smoother.prepare(sr);
        Controls a,b;a.depth=0.f;a.feedback=-.9f;b=a;b.depth=1.f;b.feedback=.9f;
        REQUIRE(smoother.next(a).depth==0.f);
        auto first=smoother.next(b);
        REQUIRE(first.depth>0.f&&first.depth<.02f);
        REQUIRE(first.feedback>-.9f&&first.feedback<-.85f);
        float last=first.depth;
        for(int i=1;i<16;++i){
            const auto p=smoother.next(b);
            REQUIRE(p.depth>=last&&p.depth<1.f);
            last=p.depth;
        }
        for(int i=0;i<int(sr*.030);++i)last=smoother.next(b).depth;
        REQUIRE(last>.99f);
    }
    // A running tuner must follow note changes without buffering the audio.
    // This measures the control-path response, not an artificial output delay.
    {
        PitchTracker tracker;tracker.prepare(48000);
        for(int i=0;i<24000;++i)
            tracker.push(float(.65*std::sin(2*pi*220.*i/48000.)));
        REQUIRE(tracker.pitch()>200.f && tracker.pitch()<240.f);
        int detected=-1;
        for(int i=0;i<14400;++i) {
            tracker.push(float(.65*std::sin(2*pi*440.*i/48000.)));
            if(detected<0&&tracker.pitch()>365.f&&tracker.conf()>.78f)detected=i;
        }
        REQUIRE(detected>=0 && detected<9600); // update in under 200 ms
        REQUIRE(tracker.pitch()>405.f && tracker.pitch()<475.f);
    }
    // Silence must clear pitch confidence; don't tune resonators to stale notes.
    PitchTracker gated;gated.prepare(48000);
    for(int i=0;i<48000;++i)gated.push(float(std::sin(2*pi*220*i/48000.)));
    REQUIRE(gated.conf()>.75f);
    for(int i=0;i<16000;++i)gated.push(0.f);
    REQUIRE(gated.conf()<.1f);
    return 0;
}
