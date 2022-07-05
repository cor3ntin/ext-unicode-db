#include <algorithm>
#include <cstdint>
#include <span>
#include <bit>
#include <vector>
#include <random>
#include <immintrin.h>
#include <benchmark/benchmark.h>
#include <cedilla/properties.hpp>
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











