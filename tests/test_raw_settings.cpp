#include "../Source/RawFlangusSettings.h"
#include <cstdio>
#include <cstdlib>
namespace S=skellflang::rawsettings;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(false)
int main(){
 unsigned count=0;
 for(std::size_t i=0;i<8;++i)for(int v=S::minimum[i];v<=S::maximum[i];++v){
    auto s=S::defaults;s[i]=v;S::State out{};S::Record raw{};S::VersionedRecord versioned{};
    CHECK(S::encode(s,raw));CHECK(S::decode(raw.data(),raw.size(),out)&&out==s);
    CHECK(S::encodeVersioned(s,versioned));CHECK(S::decode(versioned.data(),versioned.size(),out)&&out==s);++count;
 }
 S::Record raw{};CHECK(S::encode(S::defaults,raw));
 const S::Record expected{{4,0,0,0,0,2,0,0,0,3,0,0,8,0,0,0,128,0,0,0,224,255,255,255,96,0,0,0,96,0,0,0}};
 CHECK(raw==expected);
 auto out=S::defaults;out[0]=9;const auto before=out;
 for(std::size_t n=0;n<32;++n){CHECK(!S::decode(raw.data(),n,out));CHECK(out==before);}
 CHECK(!S::decode(nullptr,32,out));
 for(std::size_t i=0;i<8;++i){auto bad=raw;bad[4*i+3]=127;auto result=S::decode(bad.data(),bad.size(),out);CHECK(!result&&result.parameter==int(i)&&out==before);}
 S::VersionedRecord record{};CHECK(S::encodeVersioned(S::defaults,record));record[4]=2;CHECK(!S::decode(record.data(),record.size(),out)&&out==before);record[4]=1;record[6]=31;CHECK(!S::decode(record.data(),record.size(),out)&&out==before);
 auto invalid=S::defaults;invalid[0]=0;CHECK(!S::encode(invalid,raw)&&raw==expected);
 std::printf("Raw settings PASS: %u exact values in both formats; signed bytes; invalid input and atomic rejection\n",count);
}
