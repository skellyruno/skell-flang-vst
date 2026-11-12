#include "../Source/FlangDSP.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <new>

// Count C++ allocations only after prepare(), never in the audio callback.
// This guards the realtime EngineBank code without involving host or JUCE.
static std::atomic<unsigned long long> newCalls{0};
void* operator new(std::size_t n) {
    if(void* p=std::malloc(n?n:1)){newCalls.fetch_add(1,std::memory_order_relaxed);return p;}
    throw std::bad_alloc();
}
void* operator new[](std::size_t n) {
    if(void* p=std::malloc(n?n:1)){newCalls.fetch_add(1,std::memory_order_relaxed);return p;}
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
void operator delete[](void* p,std::size_t) noexcept {std::free(p);}

int main(){
    using namespace skellflang;
    EngineBank bank;
    bank.prepare(48000.);
    EqualPowerBlend blend;
    blend.prepare();
    Controls p;
    p.order=8;p.depth=.88f;p.delay=.84f;p.feedback=.75f;
    p.spread=.84f;p.speed=.42f;
    p.pitch=110.f;p.confidence=.95f;
    const auto baseline=newCalls.load(std::memory_order_relaxed);
    double accumulated=0.;
    for(int mode=0;mode<8;++mode) {
        for(int i=0;i<20000;++i) {
            // Stress jumps every 97 samples, extreme feedback signs and
            // arbitrary mode-specific macros under sustained signal.
            if((i%97)==0) {
                p.feedback=(i/97)%2?-.9f:.9f;
                p.delay=(i/97)%2?1.f:0.f;
                p.order=1+((i/97)%8);
                p.shape=(i/97)%5;
                p.extras[3]=float((i/97)%3)/2.f;
            }
            const float x=i<128?1.f:
                float(.19*std::sin(2*pi*173.*i/48000.));
            const auto y=bank.process(mode,x,-x,p);
            if(!std::isfinite(y[0])||!std::isfinite(y[1]) ||
               std::abs(y[0])>1.001f||std::abs(y[1])>1.001f) {
                std::fprintf(stderr,"Nonfinite/unbounded wet sample: mode=%d at i=%d\n",mode,i);
                return 1;
            }
            accumulated+=double(y[0])*y[0]+double(y[1])*y[1];
        }
    }
    for(int i=0;i<32768;++i){
        p.feedback=i%2?-.9f:.9f;
        const float x=float(.2*std::sin(2*pi*330.*i/48000.));
        const auto a=bank.process(0,x,x,p);
        const auto b=bank.process(6,x,x,p);
        const auto gains=blend.gains(float(i%101)/100.f);
        const float l=a[0]*gains[0]+b[0]*gains[1];
        if(!std::isfinite(l)){std::fputs("Invalid morphed sample\n",stderr);return 2;}
        accumulated+=l*l;
    }
    const auto after=newCalls.load(std::memory_order_relaxed);
    if(after!=baseline){
        std::fprintf(stderr,"Audio path allocated %llu objects\n",
                     static_cast<unsigned long long>(after-baseline));
        return 3;
    }
    if(!(accumulated>0.01&&std::isfinite(accumulated)))return 4;
    std::puts("All 8 DSP modes + morph: zero C++ new calls, bounded finite outputs");
    return 0;
}
