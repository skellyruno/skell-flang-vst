#include "../SkellCore/include/SkellCore/World.h"
#include <array>
#include <cstdio>
#include <string_view>
using namespace skellcore::world;
static bool check(bool ok,const char* what) {
    if(!ok)std::fprintf(stderr,"SkellWorld protocol failure: %s\n",what);
    return ok;
}
int main() {
    Frame msg;
    if(!check(make(msg,Kind::Hello,"uuid_123","skell.flang","Skell-Flang"),
              "Cannot create HELLO")) return 1;
    std::array<char,maxLine> bytes{};
    std::size_t written=0;
    if(!check(encode(msg,bytes,written)&&written>10&&written<maxLine,
              "Cannot encode HELLO"))return 2;
    Frame reply;
    if(!check(decode({bytes.data(),written},reply) &&
              reply.kind==Kind::Hello &&
              text(reply.instance)=="uuid_123" &&
              text(reply.topic)=="skell.flang" &&
              text(reply.value)=="Skell-Flang","Round-trip mismatch"))return 3;
    if(!check(!decode("SW1|HELLO|bad|a|b|c\n",reply),
              "Unexpected extra field accepted"))return 4;
    if(!check(!decode("SW1|HELLO|u|x|embedded\nnewline",reply),
              "Embedded newline accepted"))return 5;
    if(!check(!decode("SW2|HELLO|u|x|v\n",reply),
              "Unknown protocol version accepted"))return 6;
    if(!check(!make(msg,Kind::Event,"skell.flang","preset","bad|message"),
              "Delimiter injected into value"))return 7;
    if(!check(!make(msg,Kind::Event,"bad instance","preset","ok"),
              "Non-ASCII identity field accepted"))return 8;
    if(!check(!make(msg,Kind::Event,"skell.flang","preset","hello\rworld"),
              "Control character accepted"))return 9;
    if(!check(make(msg,Kind::Peer,"uuid-2","skell.flang","online"),
              "Cannot create PEER"))return 10;
    Mailbox<4> mailbox;
    if(!check(mailbox.push(msg)&&mailbox.push(msg)&&mailbox.push(msg),
              "Mailbox cannot fill"))return 11;
    if(!check(!mailbox.push(msg),"Mailbox overflow not rejected"))return 12;
    Frame extracted;
    if(!check(mailbox.pop(extracted)&&mailbox.pop(extracted)&&mailbox.pop(extracted) &&
              !mailbox.pop(extracted),"Mailbox draining incorrect"))return 13;
    mailbox.resetWhenStopped();
    if(!check(mailbox.push(msg)&&mailbox.pop(extracted) &&
              extracted.kind==Kind::Peer,"Mailbox reset failed"))return 14;
    std::puts("SkellWorld v1 protocol, rejection and mailbox tests passed");
    return 0;
}
