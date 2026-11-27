#include "SkellRawFlangusPath.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>
using P=skellflang::RawFlangusPath;
namespace F=skell::flangusfx;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(false)
int main(){
 for(int rate:{32000,44100,48000,96000,192000}){
  const int fade=int(std::lround(rate*.02));
  auto s=F::defaults;s[0]=9;s[6]=-128;s[7]=0;
  for(int hostBlock:{32,64,128,256,512,1024,2048,4096}){
   P p;CHECK(p.prepare(rate,s,false));
   std::vector<float> l(hostBlock,.25f),r(hostBlock,-.5f);
   const int total=2*fade;
   for(int phase=0;phase<2;++phase){
    const bool enabled=phase==0;
    for(int start=0;start<total;start+=hostBlock){
     p.beginBlock(enabled,s);
     const int n=std::min(hostBlock,total-start);
     for(int off=0;off<n;off+=P::chunkSize){
      const int chunk=std::min(P::chunkSize,n-off);
      p.renderChunk(l.data()+off,r.data()+off,chunk);
      for(int i=0;i<chunk;++i){
       const int j=start+off+i+1;
       const float amount=float(enabled?std::min(fade,j):std::max(0,fade-j))/float(fade);
       const auto out=p.blend(i,.25f,-.5f,.25f,-.5f,1.f,1.f);
       CHECK(std::abs(out[0]-(.25f+amount*(-.5f)))<=1.e-7f);
       CHECK(std::abs(out[1]-(-.5f+amount))<=1.e-7f);
       if(j>=fade)CHECK(out[0]==(enabled?-.25f:.25f)&&out[1]==(enabled?.5f:-.5f));
      }
     }
    }
   }
   // Populate delay memory; then reset, compare against a freshly prepared engine.
   s=F::defaults;s[0]=9;
   CHECK(p.prepare(rate,s,true));P fresh;CHECK(fresh.prepare(rate,s,true));
   for(int i=0;i<32;++i){p.beginBlock(true,s);p.renderChunk(l.data(),r.data(),32);for(int j=0;j<32;++j)p.blend(j,l[j],r[j],0,0,1,1);}
   p.reset();auto old=p.blend(0,0,0,.123f,-.321f,1,1);CHECK(old[0]==.123f&&old[1]==-.321f);
   p.beginBlock(true,s);fresh.beginBlock(true,s);
   p.renderChunk(l.data(),r.data(),32);fresh.renderChunk(l.data(),r.data(),32);
   for(int j=0;j<32;++j){auto a=p.blend(j,l[j],r[j],0,0,1,1),b=fresh.blend(j,l[j],r[j],0,0,1,1);CHECK(a==b);}
   // Full bypass must be exactly the legacy passthrough, not mixed again.
   p.beginBlock(true,s);p.renderChunk(l.data(),r.data(),32);
   for(int j=0;j<32;++j){const auto a=p.blend(j,l[j],r[j],l[j],r[j],1,0);CHECK(a[0]==l[j]&&a[1]==r[j]);}
   // Non-finite and subnormal native inputs are sanitized by adapter.
   float badL[3]{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),std::numeric_limits<float>::denorm_min()},badR[3]{};
   p.beginBlock(true,s);p.renderChunk(badL,badR,3);
   for(int j=0;j<3;++j){auto a=p.blend(j,0,0,0,0,1,1);CHECK(std::isfinite(a[0])&&std::isfinite(a[1]));}
   s[6]=-128;s[7]=0;
  }
 }
 std::puts("Raw path lifecycle PASS: exact 20ms endpoints, 5 rates x 8 host buffers, signed polarity, reset, bypass, finite output");
}
