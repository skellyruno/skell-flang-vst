#include "FLPresetImport.h"
#include <vector>
#include <fstream>
#include <iterator>
#include <cstdio>
#include <cstdlib>
using namespace skellflang;
using namespace flpreset;
using Bytes=std::vector<std::uint8_t>;
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(false)
static void event(Bytes& b,unsigned id,const Bytes& payload){
    b.push_back(std::uint8_t(id));
    if(id>=192){auto n=payload.size();while(n>=128){b.push_back(std::uint8_t((n&127)|128));n>>=7;}b.push_back(std::uint8_t(n));}
    b.insert(b.end(),payload.begin(),payload.end());
}
static void plugin(Bytes& b,const char* name,const Bytes& payload){
    Bytes n;for(;*name;++name){n.push_back(std::uint8_t(*name));n.push_back(0);}n.insert(n.end(),{0,0});
    event(b,98,{0,0});event(b,201,n);event(b,212,Bytes(52));event(b,213,payload);
}
static Bytes envelope(const Bytes& events,unsigned kind=0x30){
    Bytes b{'F','L','h','d',6,0,0,0,std::uint8_t(kind),0,21,0,96,0,'F','L','d','t',0,0,0,0};
    for(unsigned k=0;k<4;++k)b[18+k]=std::uint8_t(events.size()>>(8*k));
    b.insert(b.end(),events.begin(),events.end());return b;
}
int main(int argc,char** argv){
    if(argc>1){ // private validation, not bundled with public tests
        for(int i=1;i<argc;++i){std::ifstream in(argv[i],std::ios::binary);CHECK(bool(in));
            Bytes bytes((std::istreambuf_iterator<char>(in)),{});
            classicchorus::State c{};rawsettings::State f{};View flanger;
            auto ce=decodeChorus(bytes.data(),bytes.size(),c),fe=decodeFlangus(bytes.data(),bytes.size(),f);
            auto le=extract(bytes.data(),bytes.size(),Kind::flanger,flanger);
            std::printf("%s chorus=%s flangus=%s flanger=%s\n",argv[i],message(ce),message(fe),message(le));
            if(ce==Error::none){for(int x:c)std::printf("%d ",x);std::puts("");}
            if(fe==Error::none){for(int x:f)std::printf("%d ",x);std::puts("");}
            if(le==Error::none)std::printf("Flanger evidence length=%zu\n",flanger.size);
        }
        return 0;
    }
    classicchorus::State cs=classicchorus::defaults;cs[0]=1024;cs[11]=1024;
    rawsettings::State fs=rawsettings::defaults;fs[5]=-128;fs[6]=-127;
    classicchorus::Record cr{};rawsettings::Record fr{};CHECK(classicchorus::encode(cs,cr));CHECK(rawsettings::encode(fs,fr));
    Bytes events;plugin(events,"Fruity Chorus",Bytes(cr.begin(),cr.end()));
    auto b=envelope(events);classicchorus::State c{};rawsettings::State f{};
    CHECK(decodeChorus(b.data(),b.size(),c)==Error::none&&c==cs);
    CHECK(decodeFlangus(b.data(),b.size(),f)==Error::missing);
    plugin(events,"Fruity Wrapper",Bytes(300,0xff));plugin(events,"Fruity Flangus",Bytes(fr.begin(),fr.end()));
    b=envelope(events,0x40);CHECK(decodeChorus(b.data(),b.size(),c)==Error::none&&c==cs);
    CHECK(decodeFlangus(b.data(),b.size(),f)==Error::none&&f==fs);
    for(std::size_t n=0;n<b.size();++n){auto before=c;CHECK(decodeChorus(b.data(),n,c)!=Error::none&&c==before);}
    auto bad=b;bad.push_back(0);CHECK(decodeChorus(bad.data(),bad.size(),c)==Error::malformed);
    auto wrong=events;plugin(wrong,"Fruity Chorus",Bytes(cr.begin(),cr.end()));bad=envelope(wrong,0x40);
    CHECK(decodeChorus(bad.data(),bad.size(),c)==Error::ambiguous&&c==cs);
    Bytes foreign;plugin(foreign,"Fruity Flanger",Bytes(cr.begin(),cr.end()));bad=envelope(foreign);
    CHECK(decodeChorus(bad.data(),bad.size(),c)==Error::missing&&c==cs);
    View v;CHECK(extract(bad.data(),bad.size(),Kind::flanger,v)==Error::none&&v.size==52);
    Bytes invalid(cr.begin(),cr.end());invalid[0]=9;foreign.clear();plugin(foreign,"Fruity Chorus",invalid);bad=envelope(foreign);
    CHECK(decodeChorus(bad.data(),bad.size(),c)==Error::invalidState&&c==cs);
    invalid.assign(fr.begin(),fr.end());invalid[0]=0;foreign.clear();plugin(foreign,"Fruity Flangus",invalid);bad=envelope(foreign);
    CHECK(decodeFlangus(bad.data(),bad.size(),f)==Error::invalidState&&f==fs);
    bad=envelope({213,255,255,255,255,255,1});CHECK(decodeChorus(bad.data(),bad.size(),c)==Error::malformed);
    bad=envelope({213,128});CHECK(decodeChorus(bad.data(),bad.size(),c)==Error::malformed);
    bad=envelope(events,0);CHECK(decodeChorus(bad.data(),bad.size(),c)==Error::unsupported);
    // A new slot invalidates the prior named plugin, even with a valid payload.
    foreign.clear();Bytes name;for(char ch:std::string_view("Fruity Chorus")){name.push_back(std::uint8_t(ch));name.push_back(0);}name.insert(name.end(),{0,0});
    event(foreign,201,name);event(foreign,212,Bytes(52));event(foreign,98,{1,0});event(foreign,213,Bytes(cr.begin(),cr.end()));bad=envelope(foreign,0x40);
    CHECK(decodeChorus(bad.data(),bad.size(),c)==Error::missing);
    // Deterministic fuzz: mutate every byte, plus arbitrary short buffers.
    for(std::size_t i=0;i<b.size();++i){bad=b;bad[i]^=0xff;auto prior=c;auto err=decodeChorus(bad.data(),bad.size(),c);if(err!=Error::none)CHECK(c==prior);else CHECK(classicchorus::valid(c));}
    std::uint32_t seed=0xfeedabcd;for(int trial=0;trial<20000;++trial){Bytes random(std::size_t(trial%256));for(auto& x:random){seed=1664525u*seed+1013904223u;x=std::uint8_t(seed>>24);}auto prior=c;if(decodeChorus(random.data(),random.size(),c)!=Error::none)CHECK(c==prior);}
    std::puts("FST envelopes, range validation, malformed/duplicate rejection and fuzz passed");
}
