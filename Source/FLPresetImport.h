#pragma once
#include "ClassicChorusSettings.h"
#include "RawFlangusSettings.h"
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string_view>

namespace skellflang::flpreset {
// FL Studio 25.1 mixer and single-effect FST event envelopes. No plugin
// loading, arbitrary signature scanning, or state changes on parse failure.
inline constexpr std::size_t maximumBytes=16u*1024u*1024u;
enum class Kind { chorus, flangus, flanger, other };
enum class Error { none, malformed, unsupported, missing, ambiguous, invalidState };
inline const char* message(Error e) noexcept {
    switch(e){
        case Error::none:return "OK";
        case Error::malformed:return "Invalid or truncated FL preset.";
        case Error::unsupported:return "Unsupported FL preset format. Save an effect or mixer-track preset (.fst).";
        case Error::missing:return "This preset contains no matching effect.";
        case Error::ambiguous:return "More than one matching effect. Save the individual effect as an FST and import that instead.";
        case Error::invalidState:return "Matching effect has an unsupported version, size or parameter value.";
    }
    return "Unknown import error.";
}
struct View {const std::uint8_t* data=nullptr;std::size_t size=0;};
inline std::uint32_t le32(const std::uint8_t* p) noexcept {
    return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);
}
inline bool nameEquals(View v,std::string_view s) noexcept {
    if(v.size!=2*(s.size()+1))return false;
    for(std::size_t i=0;i<s.size();++i)if(v.data[2*i]!=static_cast<unsigned char>(s[i])||v.data[2*i+1]!=0)return false;
    return v.data[v.size-2]==0&&v.data[v.size-1]==0;
}
inline Kind identify(View v) noexcept {
    if(nameEquals(v,"Fruity Chorus"))return Kind::chorus;
    if(nameEquals(v,"Fruity Flangus"))return Kind::flangus;
    if(nameEquals(v,"Fruity Flanger"))return Kind::flanger;
    return Kind::other;
}
inline Error extract(const void* data,std::size_t size,Kind wanted,View& output) noexcept {
    if(!data||size<22||size>maximumBytes)return Error::malformed;
    const auto* b=static_cast<const std::uint8_t*>(data);
    if(std::memcmp(b,"FLhd",4)!=0||le32(b+4)!=6||std::memcmp(b+14,"FLdt",4)!=0)return Error::unsupported;
    if((b[8]!=0x30&&b[8]!=0x40)||b[9]!=0)return Error::unsupported;
    if(le32(b+18)!=size-22)return Error::malformed;
    std::size_t pos=22,matches=0;View match{};Kind pending=Kind::other;bool descriptor=false;
    while(pos<size){
        const auto id=b[pos++];std::size_t n=0;
        if(id<192)n=id<64?1u:id<128?2u:4u;
        else {
            // FL event lengths use unsigned base-128 varints. Five bytes max,
            // checked before reading payload or advancing the cursor.
            std::uint32_t len=0;bool ended=false;
            for(unsigned k=0;k<5;++k){
                if(pos>=size)return Error::malformed;
                const auto byte=b[pos++];
                if(k==4&&(byte&0xf0u))return Error::malformed;
                len|=std::uint32_t(byte&0x7fu)<<(7*k);
                if(!(byte&0x80u)){ended=true;break;}
            }
            if(!ended)return Error::malformed;
            n=len;
        }
        if(n>size-pos)return Error::malformed;
        const View v{b+pos,n};pos+=n;
        if(id==98){pending=Kind::other;descriptor=false;} // mixer slot boundary
        else if(id==201){pending=identify(v);descriptor=false;} // internal name, UTF16LE
        else if(id==212){descriptor=n==52;} // observed FL 25.1 plugin descriptor
        else if(id==213){ // native plugin data, paired with preceding name/descriptor
            if(pending==wanted&&wanted!=Kind::other){
                if(!descriptor)return Error::invalidState;
                if(++matches==1)match=v;
            }
            pending=Kind::other;descriptor=false;
        }
    }
    if(matches==0)return Error::missing;
    if(matches!=1)return Error::ambiguous;
    output=match;return Error::none;
}
inline Error decodeChorus(const void* data,std::size_t size,classicchorus::State& output) noexcept {
    View v;auto e=extract(data,size,Kind::chorus,v);if(e!=Error::none)return e;
    if(!classicchorus::decode(v.data,v.size,output))return Error::invalidState;
    return Error::none;
}
inline Error decodeFlangus(const void* data,std::size_t size,rawsettings::State& output) noexcept {
    View v;auto e=extract(data,size,Kind::flangus,v);if(e!=Error::none)return e;
    // FST carries the original 32-byte payload, never our SFLG wrapper.
    if(v.size!=32||!rawsettings::decode(v.data,v.size,output))return Error::invalidState;
    return Error::none;
}
}
