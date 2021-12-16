#include <stdint.h>
#include <x86intrin.h>

int main()
{
    uint64_t res = _tzcnt_u64(1ULL);
}
