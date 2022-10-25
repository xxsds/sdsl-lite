// https://github.com/xxsds/sdsl-lite/issues/43#issuecomment-1089948786
// g++-11 -std=c++20 -O3 -march=native -DNDEBUG -falign-loops=64 -I/.../sdsl-lite/include/
// /.../sdsl-lite/test/performance/int_vector_end.cpp

#include <chrono>
#include <iostream>
#include <random>
#include <vector>

#include <sdsl/bit_vectors.hpp>

static constexpr size_t num_values{1ULL << 26};

void run_benchmark(std::vector<uint64_t> const & random_values)
{
    sdsl::int_vector<64> values;
    // values.growth_factor = 2;
    // std::vector<uint64_t> values;
    values.reserve(num_values);

    std::chrono::steady_clock::time_point const start{std::chrono::steady_clock::now()};

    for (size_t const elem : random_values)
        values.push_back(elem);

    std::chrono::steady_clock::time_point const end{std::chrono::steady_clock::now()};
    double const mean =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / (double)values.size();
    std::cout << "On average, it took " << mean << " nanoseconds to push_back one value.\n";
}

int main()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> distrib{};

    std::vector<uint64_t> random_values(num_values);

    for (size_t i = 1; i < 10; ++i)
    {
        std::generate(random_values.begin(),
                      random_values.end(),
                      [&]()
                      {
                          return distrib(gen);
                      });
        std::cout << "Run " << i << ": ";
        run_benchmark(random_values);
    }

    return 0;
}
