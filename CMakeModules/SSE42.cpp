#include <stdint.h>
#include <xmmintrin.h>

int main()
{
    uint64_t x = 1;
    uint64_t res = __builtin_popcountll(x);
}
