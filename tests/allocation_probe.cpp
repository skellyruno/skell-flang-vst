#include <atomic>
#include <cstdlib>
#include <new>
static std::atomic<unsigned> calls{0};
void* operator new(std::size_t n){if(void* p=std::malloc(n?n:1)){++calls;return p;}throw std::bad_alloc();}
void* operator new[](std::size_t n){if(void* p=std::malloc(n?n:1)){++calls;return p;}throw std::bad_alloc();}
void operator delete(void* p) noexcept{std::free(p);} void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);} void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
unsigned countAllocations(){return calls.load();}
