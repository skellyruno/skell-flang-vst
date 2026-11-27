#include "NativeClassicChorusPath.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <limits>
#include <chrono>
using P=skellflang::NativeClassicChorusPath;
namespace C=skellflang::classicchorus;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(false)
std::vector<float> run(int rate,int block,const C::State& s,bool automate){
    P p;CHECK(p.prepare(rate,s,true));std::vector<float> y(rate*2);
    std::array<float,512> l{},r{};int pos=0;auto state=s;
    while(pos<rate){
        if(automate&&pos==rate/2){state[0]=1024;state[1]=5000;state[2]=1024;state[3]=5000;state[4]=0;state[5]=5000;state[6]=1024;state[9]=100;state[10]=5000;state[11]=1024;}
        int n=std::min({512,block,rate-pos});if(automate&&pos<rate/2)n=std::min(n,rate/2-pos);
        p.beginBlock(true,state);
        for(int i=0;i<n;++i){l[i]=.2f*float(std::sin((pos+i)*.031));r[i]=.3f*float(std::cos((pos+i)*.037));}
        p.renderChunk(l.data(),r.data(),n);
        for(int i=0;i<n;++i){auto z=p.blend(i,l[i],r[i],l[i],r[i],l[i],r[i],1,1,1);CHECK(std::isfinite(z[0])&&std::isfinite(z[1]));y[2*(pos+i)]=z[0];y[2*(pos+i)+1]=z[1];}
        pos+=n;
    }return y;
}
int main(){
    C::Record record{};C::State out{};
    CHECK(C::encode(C::defaults,record));CHECK(C::decode(record.data(),record.size(),out)&&out==C::defaults);
    int total=0;for(std::size_t k=0;k<12;++k)for(int v=0;v<=C::maximum[k];++v){auto s=C::defaults;s[k]=v;CHECK(C::encode(s,record));CHECK(C::decode(record.data(),record.size(),out)&&out==s);++total;}
    record[0]=2;CHECK(!C::decode(record.data(),52,out));CHECK(!C::decode(record.data(),51,out));
    std::printf("Classic codec %d individual raw values round-tripped\n",total);
    for(int rate:{44100,48000,96000,192000}){
        P p;P::Engine direct;auto s=C::defaults;CHECK(p.prepare(rate,s,true));direct.setRate(rate);direct.load(s);direct.reset();
        std::array<float,512> l{},r{};std::array<float,1024> in{},ref{};float maxError=0;
        for(int block=0;block<128;++block){for(int i=0;i<512;++i){l[i]=.3f*float(std::sin((block*512+i)*.047));r[i]=.2f*float(std::cos((block*512+i)*.061));in[2*i]=l[i];in[2*i+1]=r[i];}direct.render(in.data(),ref.data(),512);p.beginBlock(true,s);p.renderChunk(l.data(),r.data(),512);
            for(int i=0;i<512;++i){auto z=p.blend(i,l[i],r[i],l[i],r[i],l[i],r[i],1,1,1);maxError=std::max({maxError,std::abs(z[0]-ref[2*i]),std::abs(z[1]-ref[2*i+1])});}}
        CHECK(maxError<1e-9f);
        p.reset();direct.reset();p.renderChunk(l.data(),r.data(),512);direct.render(in.data(),ref.data(),512);for(int i=0;i<512;++i){auto z=p.blend(i,l[i],r[i],l[i],r[i],l[i],r[i],1,1,1);CHECK(z[0]==ref[2*i]&&z[1]==ref[2*i+1]);}
        float blockError=0;const auto a=run(rate,64,s,true);for(int block:{32,127,256,512,4096}){const auto b=run(rate,block,s,true);for(size_t i=0;i<a.size();++i)blockError=std::max(blockError,std::abs(a[i]-b[i]));}CHECK(blockError<1e-7f);
        for(const auto state:{C::State{},C::maximum}){auto y=run(rate,127,state,false);for(float x:y)CHECK(std::isfinite(x));}
        std::printf("%dHz direct parity max %.9g, automation/block max %.9g, reset/min/max PASS\n",rate,maxError,blockError);
    }
    P p;CHECK(!p.prepare(22050,C::defaults,true));auto z=p.blend(0,.25f,-.5f,0,0,0,0,1,1,1);CHECK(z[0]==.25f&&z[1]==-.5f);
}
