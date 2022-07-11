/*#include <algorithm>
#include <cstdint>
#include <span>
#include <bit>
#include <vector>
#include <random>
#include <immintrin.h>
#include <benchmark/benchmark.h>
#include <cedilla/properties.hpp>
#include <cedilla/normalization.hpp>
#include <unicode/uchar.h>
using namespace cedilla::details;
using namespace cedilla::details::generated;

const std::vector<char32_t> elems = [] {
    int N = 30000000;

    std::vector<char32_t> v;
    std::mt19937 gen{};
    gen.seed(132352542);
    std::uniform_int_distribution<char32_t> c{ 0, 0x10ffff };
    v.reserve(N);
    for (auto i = 0; i < N; i++)
        v.push_back(c(gen));
    return v;
}();

const std::vector<uint16_t> elems16 = [] {
    int N = 30000000;

    std::vector<uint16_t> v;
    std::mt19937 gen{};
    gen.seed(132352542);
    std::uniform_int_distribution<char32_t> c{ 0, 0xffff };
    v.reserve(N);
    for (auto i = 0; i < N; i++)
        v.push_back(c(gen));
    return v;
}();



static void UpperBound(benchmark::State& state, const std::vector<char32_t> &chars) {
    using namespace cedilla::details;
    using namespace cedilla::details::generated;

    for (auto _ : state) {
        for(char32_t c : chars) {
            benchmark::DoNotOptimize(skiplist_search(c, property_lower.short_offset_runs, property_lower.offsets));
        }
    }
    state.SetItemsProcessed(chars.size());
}

BENCHMARK_CAPTURE(UpperBound, "upper bound", elems)->Iterations(50);;

static void SIMD(benchmark::State& state, const std::vector<char32_t> &chars) {
    using namespace cedilla::details;
    using namespace cedilla::details::generated;

    for (auto _ : state) {
        for(char32_t c : chars) {
            benchmark::DoNotOptimize(skiplist_search_simd_128(c, property_lower.short_offset_runs, property_lower.offsets));
        }
    }
    state.SetItemsProcessed(chars.size());
}

BENCHMARK_CAPTURE(SIMD, "simd", elems)->Iterations(50);

static void SIMD2(benchmark::State& state, const std::vector<char32_t> &chars) {
    using namespace cedilla::details;
    using namespace cedilla::details::generated;

    for (auto _ : state) {
        for(char32_t c : chars) {
            benchmark::DoNotOptimize(skiplist_search_simd_128_2(c, property_lower.short_offset_runs, property_lower.offsets));
        }
    }
    state.SetItemsProcessed(chars.size());
}

BENCHMARK_CAPTURE(SIMD2, "failed experiment", elems)->Iterations(50);



static void SIMD256(benchmark::State& state, const std::vector<char32_t> &chars) {
    using namespace cedilla::details;
    using namespace cedilla::details::generated;

    for (auto _ : state) {
        for(char32_t c : chars) {
            benchmark::DoNotOptimize(skiplist_search_simd(c, property_lower.short_offset_runs, property_lower.offsets));
        }
    }
    state.SetItemsProcessed(chars.size());
}

BENCHMARK_CAPTURE(SIMD256, "SIMD256", elems)->Iterations(50);


static void SIMD256_2(benchmark::State& state, const std::vector<char32_t> &chars) {
    using namespace cedilla::details;
    using namespace cedilla::details::generated;

    for (auto _ : state) {
        for(char32_t c : chars) {
            benchmark::DoNotOptimize(skiplist_search_simd_256_2(c, property_lower.short_offset_runs, property_lower.offsets));
        }
    }
    state.SetItemsProcessed(chars.size());
}

BENCHMARK_CAPTURE(SIMD256_2, "SIMD256 2", elems)->Iterations(50);

static void ICU(benchmark::State& state, const std::vector<char32_t> &chars) {
    using namespace cedilla::details;
    using namespace cedilla::details::generated;

    for (auto _ : state) {
        for(char32_t c : chars) {
            benchmark::DoNotOptimize(u_isULowercase(c));
        }
    }
    state.SetItemsProcessed(chars.size());
}

BENCHMARK_CAPTURE(ICU, "icu", elems)->Iterations(50);


static void FindCPStdSearch(benchmark::State& state, const std::vector<uint16_t> &chars) {
    using namespace cedilla::details;
    using namespace cedilla::details::generated;

    for (auto _ : state) {
        for(uint16_t c : chars) {
            benchmark::DoNotOptimize(std::ranges::upper_bound(cedilla::details::generated::canonical_decomposition_mapping_small_keys, c));
        }
    }
    state.SetItemsProcessed(chars.size());
}

BENCHMARK_CAPTURE(FindCPStdSearch, "standard_binary_search", elems16)->Iterations(50);


static void FindBranchLess(benchmark::State& state, const std::vector<uint16_t> &chars) {
    using namespace cedilla::details;
    using namespace cedilla::details::generated;

    for (auto _ : state) {
        for(uint16_t c : chars) {
            benchmark::DoNotOptimize(branchless_lower_bound(
                cedilla::details::generated::canonical_decomposition_mapping_small_keys, uint16_t(c)));
        }
    }
    state.SetItemsProcessed(chars.size());
}

BENCHMARK_CAPTURE(FindBranchLess, "branchless search", elems16)->Iterations(50);
*/

#include <algorithm>
#include <random>
#include <vector>
#include <ranges>
#include <benchmark/benchmark.h>


inline auto branchless_lower_bound(const std::ranges::random_access_range auto & r, const auto & value) {
    auto it = std::ranges::begin(r);
    std::size_t N = std::ranges::size(r);
    while(N > 1) {
        const std::size_t half = N / 2;
        it = (it[half] < value) ? it + half : it;
        N -= half;
    }
    return *it >= value ? it : it+1;
}

inline auto linear_search(const std::ranges::random_access_range auto & r, const auto & value) {
    return std::ranges::find(r, value, [&] (auto e) {
        return e >= value;
    });
}


template <std::size_t N>
const std::vector<uint16_t> array = [] {
    std::vector<uint16_t> v;
    std::mt19937 gen{};
    gen.seed(132352542);
    std::uniform_int_distribution<uint16_t> c{ 0, 0xffff };
    v.reserve(N);
    for (std::size_t i = 0; i < N; i++)
        v.push_back(c(gen));
    std::ranges::sort(v);
    return v;
}();



const std::vector<uint16_t> needles = [] {
    int N = 3000000;

    std::vector<uint16_t> v;
    std::mt19937 gen{};
    gen.seed(132352542);
    std::uniform_int_distribution<char32_t> c{ 0, 0xffff };
    v.reserve(N);
    for (auto i = 0; i < N; i++)
        v.push_back(c(gen));
    return v;
}();


static void StdLB(benchmark::State& state, const std::vector<uint16_t> &data,
                       const std::vector<uint16_t> &needles) {
    for (auto _ : state) {
        for(uint16_t c : needles) {
            benchmark::DoNotOptimize(std::ranges::lower_bound(data, c));
        }
    }
    state.SetItemsProcessed(data.size());
}

BENCHMARK_CAPTURE(StdLB, StdLB 10, array<10>, needles);
BENCHMARK_CAPTURE(StdLB, StdLB 100, array<100>, needles);
BENCHMARK_CAPTURE(StdLB, StdLB 1000, array<1000>, needles);
BENCHMARK_CAPTURE(StdLB, StdLB 10000, array<10000>, needles);
BENCHMARK_CAPTURE(StdLB, StdLB 100000, array<100000>, needles);


static void Branchless(benchmark::State& state,  const std::vector<uint16_t> &data,
                       const std::vector<uint16_t> &needles) {
    for (auto _ : state) {
        for(uint16_t c : needles) {
            benchmark::DoNotOptimize(branchless_lower_bound(data, c));
        }
    }
    state.SetItemsProcessed(data.size());
}

BENCHMARK_CAPTURE(Branchless, Branchless 10, array<10>, needles);
BENCHMARK_CAPTURE(Branchless, Branchless 100, array<100>, needles);
BENCHMARK_CAPTURE(Branchless, Branchless 1000, array<1000>, needles);
BENCHMARK_CAPTURE(Branchless, Branchless 10000, array<10000>, needles);
BENCHMARK_CAPTURE(Branchless, Branchless 100000, array<100000>, needles);

static void LinearSearch(benchmark::State& state,  const std::vector<uint16_t> &data,
                       const std::vector<uint16_t> &needles) {
    for (auto _ : state) {
        for(uint16_t c : needles) {
            benchmark::DoNotOptimize(linear_search(data, c));
        }
    }
    state.SetItemsProcessed(data.size());
}

BENCHMARK_CAPTURE(LinearSearch, LinearSearch 10, array<10>, needles);
BENCHMARK_CAPTURE(LinearSearch, LinearSearch 100, array<100>, needles);
BENCHMARK_CAPTURE(LinearSearch, LinearSearch 1000, array<1000>, needles);
BENCHMARK_CAPTURE(LinearSearch, LinearSearch 10000, array<10000>, needles);
BENCHMARK_CAPTURE(LinearSearch, LinearSearch 100000, array<100000>, needles);








