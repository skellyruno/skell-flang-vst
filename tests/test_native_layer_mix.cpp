#include "../Source/NativeLayerMix.h"
#include <cstdio>
#include <cstdlib>
using skellflang::replaceNativeLayer;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(false)
int main(){
    using S=std::array<float,2>;
    const S dry{{.25f,-.5f}},old{{.5f,-.25f}},native{{-.25f,.5f}},other{{.75f,.125f}};
    CHECK(replaceNativeLayer(other,old,native,0.f,1.f)==other);
    CHECK(replaceNativeLayer(old,old,native,1.f,1.f)==native);
    CHECK(replaceNativeLayer(other,old,native,1.f,0.f)==other);
    for(int n=1;n<=5;++n){
        const float weight=1.f/float(n);
        const S rack{{weight*old[0]+(1-weight)*other[0],weight*old[1]+(1-weight)*other[1]}};
        const auto mixed=replaceNativeLayer(rack,old,native,weight,1.f);
        for(int ch=0;ch<2;++ch)CHECK(std::abs(mixed[ch]-(weight*native[ch]+(1-weight)*other[ch]))<1.e-6f);
        // Identical full dry signals stay exactly identical at every stack size.
        CHECK(replaceNativeLayer(dry,dry,dry,weight,1.f)==dry);
    }
    std::puts("Native layer replacement: signed dry, stack weights and bypass PASS");
}
