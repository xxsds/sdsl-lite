// Run the uint256_t tests with the uint128_t fallback class, as uint128_t_test does for uint128_t.
#include <cstdint>
#include <type_traits>

#include <sdsl/bits.hpp>

#include <gtest/gtest.h>

#pragma push_macro("__GNUC__")
#undef __GNUC__
#include <sdsl/uint256_t.hpp>
#pragma pop_macro("__GNUC__")

#include "uint256_t_test.cpp"
