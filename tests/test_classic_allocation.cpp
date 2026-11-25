#include "NativeClassicChorusPath.h"
#include <cstdio>
#include <cstdlib>
extern unsigned countAllocations();
int main(){skellflang::NativeClassicChorusPath p;auto s=skellflang::classicchorus::defaults;if(!p.prepare(192000,s,true))return 1;std::array<float,512> l{},r{};auto count=countAllocations();for(int b=0;b<512;++b){s[0]=b%2?0:1024;s[1]=b%2?0:5000;s[9]=b%2?0:100;p.beginBlock(b%32<16,s);for(int i=0;i<512;++i){l[i]=.2f*float(std::sin((b*512+i)*.1));r[i]=-l[i];}p.renderChunk(l.data(),r.data(),512);for(int i=0;i<512;++i){auto y=p.blend(i,l[i],r[i],l[i],r[i],l[i],r[i],1,1,1);if(!std::isfinite(y[0])||!std::isfinite(y[1]))return 2;}if(b%31==0)p.reset();}const unsigned delta=countAllocations()-count;std::printf("192 kHz 262144 samples, automation/reset/bypass, C++ new/new[] calls=%u\n",delta);return delta?3:0;}
