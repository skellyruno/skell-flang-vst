#include "../Source/FlangDSP.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
using namespace skellflang;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(false)
struct Stats {double energy=0.,sideEnergy=0.,panTravel=0.;float peak=0.f;};
Stats orbit(int rate,float quality,float depth,int voices,bool antiPhase) {
 EngineBank e;e.prepare(rate);
 Controls p;p.motionQuality=quality;p.order=voices;p.depth=depth;p.speed=.6f;p.spread=1.f;p.cross=.8f;p.delay=.6f;p.feedback=0.f;p.extras[0]=.8f;p.extras[1]=.3f;p.extras[2]=.8f;p.extras[3]=.6f;
 Stats s;
 for(int i=0;i<rate*4;++i) {
  const float l=.2f*float(std::sin(2*pi*440.*i/rate));
  const auto y=e.process(4,l,antiPhase?-l:l,p);
  CHECK(std::isfinite(y[0])&&std::isfinite(y[1]));
  s.peak=std::max(s.peak,std::max(std::abs(y[0]),std::abs(y[1])));
  if(i>rate/2){
   s.energy+=double(y[0])*y[0]+double(y[1])*y[1];
   s.sideEnergy+=double(y[0]-y[1])*(y[0]-y[1]);
   s.panTravel+=std::abs(double(y[0])*y[0]-double(y[1])*y[1]);
  }
 }
 CHECK(s.energy>1.&&s.peak<.5f);return s;
}
int main(){
 for(int rate:{32000,44100,48000,96000,192000}){
  const auto old=orbit(rate,0,1,8,false);
  const auto modern=orbit(rate,1,1,8,false);
  const auto shallow=orbit(rate,1,0,8,false);
  const auto anti=orbit(rate,1,1,8,true);
  CHECK(modern.panTravel/modern.energy>old.panTravel/old.energy);
  CHECK(modern.panTravel/modern.energy>shallow.panTravel/shallow.energy+.1);
  CHECK(anti.energy>modern.energy*.7);
  std::printf("%d Hz Orbit pan travel: legacy %.4f enhanced %.4f shallow %.4f; anti-phase energy %.4f\n",rate,old.panTravel/old.energy,modern.panTravel/modern.energy,shallow.panTravel/shallow.energy,anti.energy/modern.energy);
  for(int mode:{1,4,7}){
   EngineBank engine;engine.prepare(rate);
   ControlSmoother smooth;smooth.prepare(rate);
   Controls p;p.mode=mode;p.order=8;p.speed=1.f;p.depth=1.f;p.spread=1.f;p.feedback=.8f;
   for(int i=0;i<rate;++i){
    p.motionQuality=(i>rate/4&&i<rate*3/4)?1.f:0.f;
    const auto c=smooth.next(p);
    const float x=.25f*float(std::sin(2*pi*660.*i/rate));
    const auto y=engine.process(mode,x,-x,c);
    CHECK(std::isfinite(y[0])&&std::isfinite(y[1])&&std::abs(y[0])<=1.f&&std::abs(y[1])<=1.f);
   }
  }
 }
 std::puts("Enhanced Motion finite output / spatial movement checks passed");
}
