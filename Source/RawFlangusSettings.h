#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace skellflang::rawsettings {
using State = std::array<int, 8>;
using Record = std::array<std::uint8_t, 32>;
using VersionedRecord = std::array<std::uint8_t, 40>;
inline constexpr State minimum{{1,0,0,0,0,-128,-128,-128}};
inline constexpr State maximum{{9,1024,1024,128,128,128,128,128}};
inline constexpr State defaults{{4,512,768,8,128,-32,96,96}};
inline constexpr std::array<const char*,8> names{{"Order","Depth","Speed","Delay","Spread","Cross","Dry","Wet"}};
inline constexpr std::array<std::uint8_t,8> header{{'S','F','L','G',1,0,32,0}};
constexpr bool validContract() noexcept {
    for(std::size_t i=0;i<defaults.size();++i)
        if(defaults[i]<minimum[i]||defaults[i]>maximum[i])return false;
    return true;
}
static_assert(validContract(),"Every default must be representable");
struct Result { const char* error=nullptr; int parameter=-1; explicit operator bool() const noexcept {return error==nullptr;} };
inline Result validate(const State& s) noexcept {
    for(std::size_t i=0;i<s.size();++i)
        if(s[i]<minimum[i]||s[i]>maximum[i])return {"Raw value outside supported range",int(i)};
    return {};
}
inline Result decode(const void* data,std::size_t size,State& out) noexcept {
    if(!data)return {"Missing settings data"};
    const auto* p=static_cast<const std::uint8_t*>(data);
    if(size>=4&&p[0]=='S'&&p[1]=='F'&&p[2]=='L'&&p[3]=='G') {
        if(size<8)return {"Truncated SFLG header"};
        if(p[4]!=1||p[5]!=0)return {"Unsupported SFLG version"};
        if(p[6]!=32||p[7]!=0||size!=40)return {"Invalid SFLG payload size"};
        p+=8;size-=8;
    }
    if(size!=32)return {"Expected an exact 32-byte raw record or SFLG v1; full FST files are not supported"};
    State s{};
    for(std::size_t i=0;i<s.size();++i) {
        std::uint32_t u=0;for(unsigned j=0;j<4;++j)u|=std::uint32_t(p[4*i+j])<<(8*j);
        const auto v=std::int64_t(u)-(u>=0x80000000u?0x100000000LL:0LL);
        if(v<minimum[i]||v>maximum[i])return {"Raw value outside supported range",int(i)};
        s[i]=int(v);
    }
    out=s;return {};
}
inline Result encode(const State& s,Record& out) noexcept {
    if(const auto result=validate(s);!result)return result;
    for(std::size_t i=0;i<s.size();++i) {
        const auto u=std::uint32_t(s[i]);for(unsigned j=0;j<4;++j)out[4*i+j]=std::uint8_t(u>>(8*j));
    }
    return {};
}
inline Result encodeVersioned(const State& s,VersionedRecord& out) noexcept {
    Record payload{};if(const auto result=encode(s,payload);!result)return result;
    for(std::size_t i=0;i<header.size();++i)out[i]=header[i];
    for(std::size_t i=0;i<payload.size();++i)out[header.size()+i]=payload[i];
    return {};
}
}
