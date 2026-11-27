#include "RateMatchedMotion.h"
#include <cstdio>
#include <cstdlib>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(false)
int main(){
 for(int rate:{32000,44100,48000,96000,192000}){
  skellflang::RateMatchedPole p;p.prepare(rate);
  double maxError=0.;
  for(int k=1;k<=900;++k){const float orig=float(k)/1000.f;const float coeff=p.coefficient(orig,1.f);CHECK(coeff>0&&coeff<1);CHECK(p.coefficient(orig,0.f)==orig);
   const double want=-48000.*std::log1p(-double(orig));const double actual=-rate*std::log1p(-double(coeff));maxError=std::max(maxError,std::abs(actual-want)/want);}
  const double dcErr=std::abs(rate*std::log(double(p.dc(1.f)))-48000.*std::log(.995))/(48000.*-std::log(.995));CHECK(maxError<.0001&&dcErr<.0001);
  for(int k=-100;k<=100;++k){const float x=float(k)/100.f;auto a=skellflang::movingPan(.2f,x*.3f,1.f);CHECK(std::abs(a[0]*a[0]+a[1]*a[1]-2.f)<1.e-6f);auto b=skellflang::movingPan(.2f,x*.3f,0.f);CHECK(b[0]==.8f&&b[1]==1.2f);}
  std::printf("%d Hz time constant max rel error %.9g; DC %.9g; constant power pan PASS\n",rate,maxError,dcErr);
 }
}
