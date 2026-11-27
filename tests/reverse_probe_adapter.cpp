#include "ReverseReference.h"
using namespace skellflang;
template<bool Fast> static void run(const float* x,float* y,unsigned n,double fs,int condition){
    skellflang_test::ReverseProbe<Fast> p;p.prepare(fs);Controls c;c.order=6;c.feedback=.5f;c.extras[0]=.4f;c.extras[1]=.7f;c.extras[2]=.3f;
    for(unsigned i=0;i<n;++i){c.extras[3]=condition==0?0.f:condition==1?.5f:(i/1024)%2==0?0.f:.5f;auto z=p.process(x[2*i],x[2*i+1],c,c.order);y[2*i]=z[0];y[2*i+1]=z[1];}
}
extern "C" int reverse_render(int fast,const float* x,float* y,unsigned n,double fs,int condition){if(!x||!y||!n||fs<8000||fs>192000||condition<0||condition>2)return 0;if(fast)run<true>(x,y,n,fs,condition);else run<false>(x,y,n,fs,condition);return 1;}
