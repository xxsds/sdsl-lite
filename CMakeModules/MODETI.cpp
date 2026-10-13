using uint128_t [[gnu::mode(TI)]] = unsigned int;
static_assert(sizeof(uint128_t) == 16);

int main()
{
    uint128_t x = 5;
    (void)x;
    return 0;
}
