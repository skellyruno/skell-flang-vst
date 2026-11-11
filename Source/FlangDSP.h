#pragma once
// Skell-Flang DSP: allocation is limited to prepare(), never the sample callback.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace skellflang {
constexpr double pi = 3.14159265358979323846;
inline float saturate(float x) noexcept {
    return std::isfinite(x) ? std::tanh(std::clamp(x, -8.f, 8.f)) : 0.f;
}
// Float DAW signal paths must retain headroom > 0 dBFS. Clamp only
// pathological runaway values; never impose a 0 dBFS brickwall on the mix.
inline float safeHostOutput(float x) noexcept {
    return std::isfinite(x) ? std::clamp(x, -16.f, 16.f) : 0.f;
}
inline float bound(float x, float lo=0.f, float hi=1.f) noexcept {
    return std::clamp(std::isfinite(x) ? x : lo, lo, hi);
}
// Crossfeed preserves the mono sum and reaches a true L/R exchange at 100%.
inline std::array<float,2> stereoCrossfeed(float left,float right,float amount) noexcept {
    const float mix=bound(amount);
    return {left*(1.f-mix)+right*mix,right*(1.f-mix)+left*mix};
}
struct Controls {
    int mode=0, order=3, shape=0, root=0, scale=0, tuneSource=0;
    float depth=.45f, speed=.25f, delay=.5f, spread=.5f, cross=.2f;
    float feedback=.1f, tone=.65f, extras[4]{.5f,.5f,.5f,.5f};
    float pitch=220.f, confidence=0.f;
    double bpm=120.0;
    bool sync=false;
};
// SPEED chooses tempo division. LFO SHAPE must affect waveform only, never rate.
inline double lfoFrequency(const Controls& p) noexcept {
    if(!p.sync)return .04*std::exp2(7.*double(bound(p.speed))); // .04–5.12 Hz; useful fine range for chorus
    constexpr double divisions[]{64.,32.,16.,8.,4.,2.,1.,.5,.25};
    const int index=std::clamp(int(bound(p.speed)*8.999f),0,8);
    const double bpm=std::clamp(std::isfinite(p.bpm)?p.bpm:120.,20.,400.);
    return (bpm/60.)/divisions[index];
}
// Immutable per-instance equal-power gain table: no audio-thread trigonometry.
class EqualPowerBlend {
public:
    void prepare() noexcept {
        for(size_t i=0;i<table.size();++i) {
            const double angle=pi*.5*double(i)/double(table.size()-1u);
            table[i]={float(std::cos(angle)),float(std::sin(angle))};
        }
    }
    std::array<float,2> gains(float amount) const noexcept {
        const float position=bound(amount)*float(table.size()-1u);
        const unsigned i=std::min(unsigned(position),unsigned(table.size()-2u));
        const float fraction=position-float(i);
        return {table[i][0]+fraction*(table[i+1u][0]-table[i][0]),
                table[i][1]+fraction*(table[i+1u][1]-table[i][1])};
    }
private:
    std::array<std::array<float,2>,257> table{};
};
// Allocation-free, sample-rate-independent automation smoothing.
// Discrete modes follow the host at block boundaries; DSP values glide per sample.
class ControlSmoother {
public:
    void prepare(double rate) noexcept {
        coefficient=float(1.-std::exp(-1./(std::max(8000.,rate)*.004)));
        initialized=false;
    }
    Controls next(const Controls& target) noexcept {
        if(!initialized){state=target;initialized=true;}
        const auto chase=[this](float& v,float desired) noexcept {
            v+=(std::isfinite(desired)?desired-v:-v)*coefficient;
        };
        chase(state.depth,target.depth); chase(state.speed,target.speed);
        chase(state.delay,target.delay); chase(state.spread,target.spread);
        chase(state.cross,target.cross); chase(state.feedback,target.feedback);
        chase(state.tone,target.tone);
        for(int i=0;i<4;++i)chase(state.extras[i],target.extras[i]);
        state.mode=target.mode;state.order=target.order;
        state.shape=target.shape;state.root=target.root;state.scale=target.scale;
        state.tuneSource=target.tuneSource;state.sync=target.sync;
        state.bpm=target.bpm;state.pitch=target.pitch;state.confidence=target.confidence;
        return state;
    }
private:
    Controls state{};
    float coefficient=.004f;
    bool initialized=false;
};

class Delay {
public:
    void prepare(double rate, double seconds) {
        data.assign(static_cast<size_t>(std::ceil(rate*seconds)) + 12u, 0.f); head=0;
    }
    float read(double samples) const noexcept {
        if(data.empty()) return 0;
        const double d=std::clamp(std::isfinite(samples)?samples:3.0, 3.0, double(data.size()-4));
        const int idx=int(d); const float t=float(d-idx);
        const float a=at(idx-1), b=at(idx), c=at(idx+1), e=at(idx+2);
        const float m0=.5f*(c-a), m1=.5f*(e-b);
        const float t2=t*t, t3=t2*t;
        return (2*t3-3*t2+1)*b+(t3-2*t2+t)*m0+(-2*t3+3*t2)*c+(t3-t2)*m1;
    }
    void write(float x) noexcept {
        if(data.empty()) return;
        data[head]=std::isfinite(x)?std::clamp(x,-2.f,2.f):0.f;
        if(++head==data.size())head=0;
    }
    void clear() noexcept { std::fill(data.begin(),data.end(),0.f);head=0; }
private:
    float at(int lag) const noexcept {
        // The read lag is always in [2, size-2]. It can wrap at most once,
        // so avoid a costly integer division/modulo in every interpolated tap.
        int index=int(head)-lag;
        if(index<0)index+=int(data.size());
        return data[size_t(index)];
    }
    std::vector<float> data;
    size_t head=0;
};
class PitchTracker {
public:
    void prepare(double rate) noexcept {
        sr=std::max(8000.0,rate);
        // Constant ~12 kHz tracker input rate even at 192 kHz sessions.
        // Fixed /4 decimation would lose fundamentals below ~137 Hz at 192 kHz.
        stride=std::max(1,int(std::ceil(sr/12000.0)));
        ring.fill(0);head=ticks=decimate=0;hz=0;confidence=0;
        preFilter=0.f;
        decimationAlpha=float(1.-std::exp(-2.*pi*3500./sr));
    }
    void push(float x) noexcept {
        // Anti-alias before decimation: one pole costs two operations/sample
        // and avoids folding upper harmonics into the pitch detector.
        const float input=std::isfinite(x)?x:0.f;
        preFilter+=(input-preFilter)*decimationAlpha;
        if(++decimate<stride)return;
        decimate=0;ring[head++ & 2047]=preFilter;
        if(++ticks<128)return;
        ticks=0; const double effective=sr/double(stride);
        double energy=0;for(int i=0;i<256;i+=4) {
            const double v=ring[(head-1u-static_cast<unsigned>(i))&2047];
            energy+=v*v;
        }
        if(energy<1.e-6){confidence=0;hz=0;return;}
        const int lo=std::max(8,int(effective/950.0));
        const int hi=std::min(350,int(effective/72.0));
        double best=-1,prev=-2,prev2=-2;int lagBest=0;
        for(int lag=lo;lag<=hi;++lag) {
            double xy=0,xx=1.e-12,yy=1.e-12;
            for(int i=0;i<512;i+=4) {
                const double a=ring[(head-1u-static_cast<unsigned>(i))&2047];
                const double b=ring[(head-1u-static_cast<unsigned>(i+lag))&2047];
                xy+=a*b;xx+=a*a;yy+=b*b;
            }
            const double score=xy/std::sqrt(xx*yy);
            if(lag>lo+1 && prev>prev2 && prev>=score && prev>.84){best=prev;lagBest=lag-1;break;}
            if(score>best){best=score;lagBest=lag;}
            prev2=prev;prev=score;
        }
        confidence=std::clamp(best,0.0,1.0);
        if(lagBest>0 && confidence>.78) {
            const double incoming=effective/lagBest;
            hz=hz>0 ? .72*hz+.28*incoming : incoming;
        }
    }
    float pitch() const noexcept{return float(hz);}
    float conf() const noexcept{return float(confidence);}
private:
    std::array<float,2048> ring{};
    unsigned head=0;int ticks=0,decimate=0,stride=4;
    float preFilter=0.f,decimationAlpha=1.f;
    double sr=48000,hz=0,confidence=0;
};
struct Biquad {
    float b0=0,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
    void setBandpass(float hz,float bandwidth,double sr) noexcept {
        const double f=std::clamp(double(hz),80.,sr*.44);
        const double w=2*pi*f/sr;
        const double q=std::max(.55,f/std::max(24.,double(bandwidth)));
        const double alpha=std::sin(w)/(2*q);
        const double norm=1/(1+alpha);
        b0=float(alpha*norm);b1=0;b2=-b0;
        a1=float(-2*std::cos(w)*norm);a2=float((1-alpha)*norm);
    }
    float process(float x) noexcept {
        const float y=b0*x+z1;
        z1=b1*x-a1*y+z2;z2=b2*x-a2*y;
        if(!std::isfinite(y)){z1=z2=0;return 0;}
        return bound(y,-4.f,4.f);
    }
};
class EngineBank {
    struct Voice {
        Delay delay[2];float low[2]{},dcx[2]{},dcy[2]{},air[2]{};
        double lag[2]{},lagAlpha=.01;float fb[2]{},fbAlpha=.01f;
        bool lagReady[2]{};
        void prepare(double sr) {
            for(auto& d:delay)d.prepare(sr,.08);
            for(auto& x:low)x=0;
            for(auto& x:dcx)x=0;
            for(auto& x:dcy)x=0;
            for(auto& x:air)x=0;
            for(auto& x:lag)x=0;
            for(auto& x:fb)x=0;
            lagReady[0]=false;
            lagReady[1]=false;
            lagAlpha=1.-std::exp(-1./(std::max(8000.,sr)*.0035));
            fbAlpha=float(1.-std::exp(-1./(std::max(8000.,sr)*.008)));
        }
        float tap(int ch,float input,double delaySamples,const Controls& p) noexcept {
            // 3.5 ms one-pole slew prevents fractional-delay zippering on
            // sudden automation while preserving normal chorus/flanger motion.
            const double goal=std::isfinite(delaySamples)?std::max(3.,delaySamples):3.;
            if(!lagReady[ch]){lag[ch]=goal;lagReady[ch]=true;}
            else lag[ch]+=lagAlpha*(goal-lag[ch]);
            const float d=delay[ch].read(lag[ch]);
            const float hp=d-dcx[ch]+.995f*dcy[ch];
            dcx[ch]=d;dcy[ch]=hp;
            low[ch]+=(.035f+.86f*bound(p.tone))*(hp-low[ch]);
            // Feedback automation is independent of tap-time smoothing.
            fb[ch]+=(bound(p.feedback,-.9f,.9f)-fb[ch])*fbAlpha;
            delay[ch].write(saturate(input+low[ch]*fb[ch]));
            return d;
        }
    };
    struct Grain {bool active=false;int age=0,duration=1,voice=0;double start=0;};
public:
    void prepare(double sampleRate) {
        sr=std::max(8000.0,sampleRate);
        for(auto& mode:voices)for(auto& v:mode)v.prepare(sr);
        for(auto& d:reverse)d.prepare(sr,1.5);
        for(size_t i=0;i<reverseWindow.size();++i){
            const double t=double(i)/double(reverseWindow.size()-1u);
            reverseWindow[i]=float(.5-.5*std::cos(2*pi*t));
        }
        keyCacheRoot=-1;keyCacheScale=-1;cachedSpread=-1.;
        tunedRatio=1.;
        for(auto& pair:formants)for(auto& f:pair)f={};
        grains={};phase.fill(0);voicePhase.fill(0);tunedDelay.fill(0);samples.fill(0);grainClock=0;rng=0xFA11B00Du;
    }
    std::array<float,2> process(int which,float l,float r,const Controls& p) noexcept {
        const int mode=std::clamp(which,0,7);
        const int n=std::clamp(p.order,1,8);
        const double hz=lfoFrequency(p);
        const double step=std::min((mode==4?hz*(.5+.9*bound(p.extras[3])):hz)/sr,.2);
        const double cycle=phase[mode];
        phase[mode]+=step;
        if(phase[mode]>=1)phase[mode]-=1;
        ++samples[mode];
        auto mod=[&](double offset) {
            const double q=cycle+offset-std::floor(cycle+offset);
            switch(p.shape) {
                case 1:return float(1.-4.*std::abs(q-.5));
                case 2:return float(2.*q-1.);
                case 3:return float(1.-2.*q);
                case 4:return float(std::tanh(10*std::sin(2*pi*q)));
                default:return float(std::sin(2*pi*q));
            }
        };
        float outL=0,outR=0;
        if(mode==3)return processReverse(l,r,p,n);
        if(mode==2) {
            // Five original, approximate vowel resonances. No third-party phonetic table.
            constexpr float vowel[5][5]{
                {750,1200,2500,3500,4400},{520,1800,2600,3500,4400},
                {290,2300,3000,3800,4550},{510,950,2400,3300,4200},
                {330,780,2200,3100,4100}};
            constexpr float width[5]{90,130,170,260,330};
            const float v=bound(p.extras[0]+p.depth*.4f*mod(0));
            const float where=v*4.f;const int low=int(where),high=std::min(4,low+1);
            const float mix=where-low;
            if((samples[mode]&31u)==0u)for(int ch=0;ch<2;++ch)
                for(int b=0;b<5;++b){
                    const float f=std::exp(std::log(vowel[low][b])*(1-mix)+std::log(vowel[high][b])*mix);
                    formants[ch][b].setBandpass(f*(.7f+.7f*p.extras[1]),width[b]*(1.6f-p.extras[2]),sr);
                }
            const double delay=sr*.001*(1.+13.*bound(p.delay));
            const float dl=voices[2][0].tap(0,l,delay,p);
            const float dr=voices[2][0].tap(1,r,delay,p);
            float vl=0,vr=0;
            for(int b=0;b<std::min(5,n);++b) {
                vl+=formants[0][b].process(dl);
                vr+=formants[1][b].process(dr);
            }
            const float comb=bound(p.extras[3]);
            return {saturate(comb*dl+(1-comb)*vl*.8f),saturate(comb*dr+(1-comb)*vr*.8f)};
        }
        if(mode==5) {
            constexpr int major[]{0,2,4,5,7,9,11};
            constexpr int minor[]{0,2,3,5,7,8,10};
            constexpr int pent[]{0,2,4,7,9};
            const int rootIndex=std::clamp(p.root,0,11);
            const int scaleIndex=std::clamp(p.scale,0,3);
            if(rootIndex!=keyCacheRoot || scaleIndex!=keyCacheScale) {
                const double root=65.406391325*std::exp2(double(rootIndex)/12.);
                for(int i=0;i<8;++i) {
                    const int degree=scaleIndex==0 ? major[i%7]+12*(i/7)
                        :scaleIndex==1 ? minor[i%7]+12*(i/7)
                        :scaleIndex==2 ? pent[i%5]+12*(i/5) : i;
                    keyFrequencies[size_t(i)]=root*std::exp2(double(degree)/12.);
                }
                keyCacheRoot=rootIndex;keyCacheScale=scaleIndex;
            }
            const double newSpread=double(bound(p.spread));
            if(std::abs(newSpread-cachedSpread)>1.e-4) {
                cachedSpread=newSpread;
                tunedRatio=std::exp2(newSpread/12.);
            }
            // Per-sample one-pole coefficient in seconds, not a fixed
            // coefficient: equivalent glide time at 44.1/48/96/192 kHz.
            const double glide=1./(1.+sr*(.00015+.045*bound(p.extras[1])));
            const double harmonics=1.+std::round(3.*bound(p.extras[2]));
            const double track=double(bound(p.extras[0]));
            const bool following=p.confidence>.8f && p.pitch>45.f;
            double voiceRatio=1.;
            for(int i=0;i<n;++i) {
                const double key=keyFrequencies[size_t(i)];
                const double detected=following ? double(p.pitch)*voiceRatio : key;
                voiceRatio*=tunedRatio;
                const double f=p.tuneSource==0 ? detected*track+key*(1.-track) : key;
                const double goal=std::clamp(sr*harmonics/std::max(45.,f),sr*.00015,sr*.040);
                if(tunedDelay[i]<=0)tunedDelay[i]=goal;
                tunedDelay[i]+=(goal-tunedDelay[i])*glide;
                Controls resonant=p;
                resonant.feedback=bound(p.feedback+.25f*p.extras[3],-.9f,.9f);
                outL+=voices[5][i].tap(0,l,tunedDelay[i],resonant);
                outR+=voices[5][i].tap(1,r,tunedDelay[i],resonant);
            }
            return {saturate(outL/n),saturate(outR/n)};
        }
        for(int i=0;i<n;++i) {
            auto& v=voices[size_t(mode)][size_t(i)];
            if(mode==0) { // CHORUS: decorrelated dual-channel multi-voice detuning
                const float location=n>1 ? float(i)*2.f/float(n-1)-1.f : 0.f;
                voicePhase[size_t(i)]+=step*(1.+.055*bound(p.extras[0])*location);
                if(voicePhase[size_t(i)]>=1.)voicePhase[size_t(i)]-=1.;
                const double phaseOffset=double(i)*.173;
                const double leftWave=std::sin(2*pi*(voicePhase[size_t(i)]+phaseOffset));
                const double rightWave=std::sin(2*pi*(voicePhase[size_t(i)]+phaseOffset+
                                  .14+.28*bound(p.extras[1])));
                const double baseMs=9.+15.*bound(p.delay);
                const double depthMs=(.5+3.2*bound(p.depth))*(.60+.40*bound(p.extras[0]));
                const double spreadMs=1.8*bound(p.spread)*location;
                const double dL=sr*.001*(baseMs+depthMs*leftWave+spreadMs);
                const double dR=sr*.001*(baseMs+depthMs*rightWave-spreadMs);
                const float dl=v.tap(0,l,dL,p),dr=v.tap(1,r,dR,p);
                const float smoothing=.30f+.60f*(1.f-bound(p.extras[2]));
                v.air[0]+=smoothing*(dl-v.air[0]);
                v.air[1]+=smoothing*(dr-v.air[1]);
                const float pan=location*bound(p.spread)*bound(p.extras[1])*.28f;
                outL+=v.air[0]*(1.f-pan);
                outR+=v.air[1]*(1.f+pan);
            } else if(mode==1) { // FLANGER: short comb with controllable stereo phase
                const float env=std::min(1.f,.5f*(std::abs(l)+std::abs(r)));
                const float travel=.25f+1.45f*bound(p.extras[0]);
                const float phaseOffset=float(i)*bound(p.spread)/float(n);
                const float a=mod(phaseOffset);
                const float b=mod(phaseOffset+.04f+.35f*bound(p.spread));
                const float baseMs=.16f+3.4f*bound(p.delay)+1.8f*bound(p.extras[2]);
                const float sweepMs=(.12f+3.9f*bound(p.depth))*travel;
                const float follow=1.8f*env*bound(p.extras[3]);
                const double dL=sr*.001*(baseMs+.5f*sweepMs*(1.f+a)+follow);
                const double dR=sr*.001*(baseMs+.5f*sweepMs*(1.f+b)+follow);
                Controls coloured=p;
                coloured.tone=bound(p.tone+(p.extras[1]-.5f)*.40f);
                outL+=v.tap(0,l,dL,coloured);
                outR+=v.tap(1,r,dR,coloured);
            } else if(mode==6) { // FLANGUS: two motion rates over an ensemble of comb delays
                const float location=n>1 ? float(i)/float(n-1) : .5f;
                const double offset=double(i)*bound(p.spread)/std::max(1,n);
                const double slowL=std::sin(2*pi*(cycle+offset));
                const double slowR=std::sin(2*pi*(cycle+offset+.15+.18*bound(p.extras[1])));
                const double fastL=std::sin(2*pi*(cycle*1.71+offset*.71));
                const double fastR=std::sin(2*pi*(cycle*1.71+offset*.71+.19));
                const double base=3.+12.*bound(p.delay)+4.*location*bound(p.spread);
                const double travel=(.5+4.*bound(p.depth))*(.45+.55*bound(p.extras[0]));
                const double jet=(.2+2.2*bound(p.extras[2]))*bound(p.depth);
                Controls coloured=p;
                coloured.tone=bound(p.tone+.24f*(bound(p.extras[3])-.5f));
                outL+=v.tap(0,l,sr*.001*(base+travel*slowL+jet*fastL),coloured);
                outR+=v.tap(1,r,sr*.001*(base+travel*slowR+jet*fastR),coloured);
            } else if(mode==7) { // DIMENSION: slow short-delay stereo doubling
                const float location=n>1 ? float(i)/float(n-1) : .5f;
                const double offset=double(i)/double(n);
                const double waveL=std::sin(2*pi*(cycle*.52+offset));
                const double waveR=std::sin(2*pi*(cycle*.58+offset+.33));
                const double base=11.+18.*bound(p.delay);
                const double motion=(.18+1.8*bound(p.depth))*(.6+.4*bound(p.extras[0]));
                const double separation=5.*bound(p.spread)*location*bound(p.extras[1]);
                const float dl=v.tap(0,l,sr*.001*(base+motion*waveL+separation),p);
                const float dr=v.tap(1,r,sr*.001*(base+motion*waveR-separation),p);
                const float tone=.12f+.75f*bound(p.extras[2]);
                v.air[0]+=tone*(dl-v.air[0]);
                v.air[1]+=tone*(dr-v.air[1]);
                const float pan=(location-.5f)*bound(p.extras[3])*.55f;
                outL+=v.air[0]*(1.f-pan);
                outR+=v.air[1]*(1.f+pan);
            } else { // ORBIT: amplitude / ITD / air-distance approximation.
                const float angle=float(2*pi*(cycle+double(i)/n*p.spread));
                const float x=std::cos(angle),y=std::sin(angle)*(1.f-.6f*bound(p.extras[1]));
                const float radius=.25f+.75f*bound(p.extras[0]);
                const double d=sr*.001*(3.+6.*radius);
                const double itd=sr*.001*(.2+.9*bound(p.cross))*radius*x;
                // Keep each source channel independent. Summing L+R before
                // orbiting erased anti-phase stereo and could go completely silent.
                const float a=v.tap(0,l,d+itd,p)*(1-.45f*x);
                const float b=v.tap(1,r,d-itd,p)*(1+.45f*x);
                const float coeff=.15f+.7f*(1-bound(p.extras[2])*.6f);
                v.air[0]+=coeff*(a-v.air[0]);v.air[1]+=coeff*(b-v.air[1]);
                outL+=v.air[0]*(.9f+.1f*y);outR+=v.air[1]*(.9f+.1f*y);
            }
        }
        return {saturate(outL/n),saturate(outR/n)};
    }
private:
    unsigned random() noexcept{rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
    std::array<float,2> processReverse(float l,float r,const Controls&p,int n) noexcept {
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
            // Hann interpolation is bandlimited enough for a control envelope.
            // Lookup avoids up to 32 cos() calls per audio sample.
            const float position=t*float(reverseWindow.size()-1u);
            const auto index=std::min(size_t(position),reverseWindow.size()-2u);
            const float blend=position-float(index);
            const float window=reverseWindow[index]+
                blend*(reverseWindow[index+1u]-reverseWindow[index]);
            const double d=g.start+2.0*g.age;
            const float pan=(g.voice%2==0?.3f:-.3f);
            const float shimmer=bound(p.extras[3]);
            const float dl=reverse[0].read(d),dr=reverse[1].read(d);
            const float octaveL=reverse[0].read(g.start+3.0*g.age);
            const float octaveR=reverse[1].read(g.start+3.0*g.age);
            a+=window*(dl*(1-shimmer)+octaveL*shimmer)*(1-pan);
            b+=window*(dr*(1-shimmer)+octaveR*shimmer)*(1+pan);
            weight+=window;++g.age;
        }
        if(weight>.1f){a/=weight;b/=weight;}
        reverse[0].write(saturate(l+a*p.feedback*.15f));
        reverse[1].write(saturate(r+b*p.feedback*.15f));
        return {saturate(a),saturate(b)};
    }
    double sr=48000;
    std::array<std::array<Voice,8>,8> voices{};
    std::array<Delay,2> reverse{};
    std::array<std::array<Biquad,5>,2> formants{};
    std::array<Grain,32> grains{};
    std::array<float,2049> reverseWindow{};
    std::array<double,8> keyFrequencies{};
    double cachedSpread=-1.,tunedRatio=1.;
    int keyCacheRoot=-1,keyCacheScale=-1;
    std::array<double,8> phase{};
    std::array<double,8> voicePhase{},tunedDelay{};
    std::array<unsigned,8> samples{};
    int grainClock=0;uint32_t rng=0xFA11B00Du;
};
} // namespace skellflang
