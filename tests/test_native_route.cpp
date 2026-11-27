#include "SkellRawFlangusPath.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
using P=skellflang::RawFlangusPath;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(false)
int main(){
    using S=std::array<float,2>;
    const auto state=skell::flangusfx::defaults;
    for(int rate:{32000,44100,48000,96000,192000}){
        P p; P::Adapter ref;
        CHECK(p.prepare(rate,state,true));CHECK(ref.prepare(rate,state));
        std::vector<float> l(512),r(512),nl(512),nr(512);
        for(int chunk=0;chunk<24;++chunk){
            for(int j=0;j<512;++j){l[j]=.35f*float(std::sin((chunk*512+j)*.047));r[j]=.23f*float(std::cos((chunk*512+j)*.071));}
            nl=l;nr=r;CHECK(ref.process(nl.data(),nr.data(),512));
            p.beginBlock(true,state);p.renderChunk(l.data(),r.data(),512);
            const float fraction=float(chunk%5+1)/5.f;
            for(int j=0;j<512;++j){
                const float oldL=.57f*l[j],oldR=.57f*r[j];
                const float otherL=.4f*r[j]+oldL,otherR=.4f*l[j]+oldR;
                const float rackL=fraction*oldL+(1.f-fraction)*otherL;
                const float rackR=fraction*oldR+(1.f-fraction)*otherR;
                const auto y=p.blendLayer(j,l[j],r[j],rackL,rackR,oldL,oldR,1.f,1.f,fraction);
                CHECK(std::abs(y[0]-(fraction*nl[j]+(1.f-fraction)*otherL))<2.e-7f);
                CHECK(std::abs(y[1]-(fraction*nr[j]+(1.f-fraction)*otherR))<2.e-7f);
                if(fraction==1.f)CHECK(y[0]==nl[j]&&y[1]==nr[j]);
            }
        }
    }
    P unavailable;CHECK(!unavailable.prepare(22050.,state,true));
    CHECK((unavailable.blendLayer(0,.25f,-.5f,.13f,-.26f,.13f,-.26f,1.f,1.f,1.f)==S{.25f,-.5f}));
    CHECK((unavailable.blendLayer(0,.25f,-.5f,.13f,-.26f,.13f,-.26f,1.f,1.f,0.f)==S{.13f,-.26f}));
    std::puts("Actual native engine in 1–5 layer weighted mix, solo source parity, unsupported-rate bypass PASS");
}
