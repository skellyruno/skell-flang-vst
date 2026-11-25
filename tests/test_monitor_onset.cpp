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
    }
}
