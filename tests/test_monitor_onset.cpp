#include "FlangDSP.h"
#include "NativeClassicChorusPath.h"
#include "SkellRawFlangusPath.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#define REQUIRE(x) do {if(!(x)) {std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(false)
int main() {
    for(int rate:{44100,48000,96000,192000}){
        skelly::fruitychorusfx::Engine chorus;chorus.setRate(rate);chorus.load(skelly::fruitychorusfx::defaults);chorus.reset();
        std::vector<float> in(2*rate),out(2*rate),l(rate),r(rate);
        in[0]=in[1]=l[0]=r[0]=.5f;
        chorus.render(in.data(),out.data(),rate);
        REQUIRE(out[0]>.349f&&out[0]<.351f);
        int c=-1;
        for(int i=1;i<rate;++i)if(std::abs(out[2*i])>1.e-6f){c=i;break;}
        REQUIRE(c>=int(rate*.016)&&c<=int(rate*.017));
        skell::private_research::FlangusAdapter native;
        REQUIRE(native.prepare(rate,skell::flangusfx::defaults));REQUIRE(native.process(l.data(),r.data(),rate));
        REQUIRE(l[0]>.25f&&l[0]<.255f);
        int f=-1;
        for(int i=1;i<rate;++i)if(std::abs(l[i])>1.e-6f){f=i;break;}
        REQUIRE(f>=int(rate*.00035)&&f<=int(rate*.0008));
        std::printf("%d Hz: immediate dry both; first wet Classic %.4f ms, Flangus %.4f ms\n",rate,1000.*c/rate,1000.*f/rate);
        for(int mode:{1,4,7}) {
            skellflang::Controls p;p.mode=mode;
            const char* name=nullptr;double lo=0.,hi=0.;
            if(mode==1){
                name="Satin Flanger";p.order=2;p.depth=.38f;p.speed=.43f;p.delay=.33f;p.spread=.40f;p.cross=.06f;p.feedback=.25f;p.tone=.80f;
                p.extras[0]=.56f;p.extras[1]=.55f;p.extras[2]=.43f;p.extras[3]=.05f;lo=1.;hi=6.;
            } else if(mode==4) {
                name="Orbit Halo";p.order=4;p.depth=.60f;p.speed=.31f;p.delay=.54f;p.spread=.61f;p.cross=.20f;p.feedback=.16f;p.tone=.67f;
                p.extras[0]=.58f;p.extras[1]=.40f;p.extras[2]=.68f;p.extras[3]=.56f;p.motionQuality=1.f;lo=7.;hi=14.;
            } else {
                name="Dimension Air";p.order=3;p.depth=.31f;p.speed=.32f;p.delay=.52f;p.spread=.80f;p.cross=.08f;p.feedback=.01f;p.tone=.89f;
                p.extras[0]=.62f;p.extras[1]=.78f;p.extras[2]=.93f;p.extras[3]=.79f;lo=15.;hi=25.;
            }
            skellflang::EngineBank bank;bank.prepare(rate);
            const int frames=rate/10;std::vector<std::array<float,2>> impulse(size_t(frames),{});
            int first=-1;float peak=0.f;
            for(int i=0;i<frames;++i){
                const auto y=bank.process(mode,i==0?.5f:0.f,i==0?.5f:0.f,p);
                impulse[size_t(i)]=y;
                REQUIRE(std::isfinite(y[0])&&std::isfinite(y[1]));
                const auto magnitude=std::max(std::abs(y[0]),std::abs(y[1]));
                peak=std::max(peak,magnitude);
                if(first<0&&magnitude>1.e-6f)first=i;
            }
            const double ms=1000.*first/rate;
            REQUIRE(first>0&&ms>=lo&&ms<=hi);
            std::printf("%d Hz: %s wet onset %.4f ms, impulse peak %.6f\n",rate,name,ms,peak);
            // Charge delay lines with nonzero audio, then simulate a dormant
            // layer returning. No stale tail and identical fresh response.
            for(int i=0;i<frames;++i)bank.process(mode,.2f,-.1f,p);
            bank.resetMode(mode);
            for(int i=0;i<frames;++i) {
                const auto y=bank.process(mode,i==0?.5f:0.f,i==0?.5f:0.f,p);
                REQUIRE(y==impulse[size_t(i)]);
            }
            bank.resetMode(mode);
            for(int i=0;i<frames;++i) {
                const auto y=bank.process(mode,0.f,0.f,p);
                REQUIRE(y[0]==0.f&&y[1]==0.f);
            }
        }
    }
}
