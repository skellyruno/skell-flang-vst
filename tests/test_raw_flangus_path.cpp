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
 for(double rate:{32000.,44100.,48000.,96000.,192000.}){
  std::vector<float> l(12000),r(12000);for(std::size_t i=0;i<l.size();++i){l[i]=.2f*std::sin(i*.05);r[i]=.1f*std::cos(i*.06);}
  auto a=l,b=r;P::Adapter ref;CHECK(ref.prepare(int(rate),F::defaults));CHECK(ref.process(a.data(),b.data(),int(a.size())));
  for(int block:{1,32,64,256,512}){P p;CHECK(p.prepare(rate,F::defaults,true));for(int start=0;start<int(l.size());start+=block){int n=std::min(block,int(l.size())-start);p.beginBlock(true,F::defaults);p.renderChunk(l.data()+start,r.data()+start,n);for(int i=0;i<n;++i){auto o=p.blend(i,l[start+i],r[start+i],.07f,-.09f,1.f,1.f);CHECK(o[0]==a[start+i]&&o[1]==b[start+i]);}}}
  P p;CHECK(p.prepare(rate,F::defaults,false));p.beginBlock(false,F::defaults);p.renderChunk(l.data(),r.data(),512);auto o=p.blend(0,l[0],r[0],.07f,-.09f,1.f,1.f);CHECK(o[0]==.07f&&o[1]==-.09f);
  auto dry=F::defaults;dry[6]=128;dry[7]=0;p.beginBlock(true,dry);for(int start=0;start<int(l.size());start+=512){int n=std::min(512,int(l.size())-start);p.renderChunk(l.data()+start,nullptr,n);for(int i=0;i<n;++i){o=p.blend(i,l[start+i],l[start+i],.07f,.07f,1.f,1.f);CHECK(std::isfinite(o[0])&&o[0]==o[1]);if(start+i>int(rate*.025))CHECK(o[0]==l[start+i]);}}
  p.beginBlock(false,dry);for(int start=0;start<int(l.size());start+=512){int n=std::min(512,int(l.size())-start);p.renderChunk(l.data()+start,r.data()+start,n);for(int i=0;i<n;++i){o=p.blend(i,l[start+i],r[start+i],.07f,-.09f,1.f,1.f);if(start+i>int(rate*.025))CHECK(o[0]==.07f&&o[1]==-.09f);}}
 }
 P p;CHECK(!p.prepare(22050.,F::defaults,true));CHECK(!p.available());CHECK(!p.prepare(std::numeric_limits<double>::quiet_NaN(),F::defaults,true));
 std::puts("Raw Flangus path PASS: five rates, five block sizes, source-engine parity, disabled legacy identity, mono and transitions");
}
