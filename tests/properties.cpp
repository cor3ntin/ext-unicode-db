#include <variant>
#include <set>
#include <ranges>
#include <cedilla/properties.hpp>
#include <cedilla/normalization.hpp>
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include "codepoint_data.hpp"
#include "load.hpp"
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <random>
#include "globals.hpp"


struct property_map {
    std::string_view name;
    using F = bool(char32_t);
    F* function;
};


TEST_CASE("Exhaustive properties checking", "[props]")
{
    #define P(X) property_map{#X, &cedilla::is_##X}
    auto prop = GENERATE(
        P(alpha),
        P(dash),
        P(hex),
        P(qmark),
        P(term),
        P(sterm),
        P(di),
        P(dia),
        P(ext),
        P(sd),
        P(math),
        P(wspace),
        P(ri),
        P(nchar),
        P(vs),
        P(emoji),
        P(epres),
        P(emod),
        P(ebase),
        P(pat_ws),
        P(pat_syn),
        P(xids),
        P(xidc),
        P(ideo),
        P(upper),
        P(lower)
     );
    #undef P

    auto it = codepoints->begin();
    const auto end = codepoints->end();

    for(char32_t c = 0; c < 0x10FFFF; c++) {
        INFO("cp: U+" << std::hex << (uint32_t)c << " " << prop.name);
        if(auto p = std::ranges::lower_bound(it, end, c, {}, &cedilla::tools::codepoint::value); p != end && p->value == c) {
            CHECK(p->has_binary_property(prop.name) == prop.function(c));
            it = p+1;
        } else {
            CHECK(!prop.function(c));
        }
    }
}

TEST_CASE("cp_script", "[props]") {
    std::set<std::string> scripts_set;
    scripts_set.insert("zzzz");
    for(const auto & [k, _] : labels->scripts) {
        scripts_set.insert(k);
    }

    std::vector scripts(scripts_set.begin(), scripts_set.end());
    std::ranges::sort(scripts);
    auto unknown = std::ranges::find(scripts, "zzzz");
    if(unknown != scripts.end()) {
        scripts.erase(unknown);
    }
    scripts.insert(scripts.begin(), "zzzz");

    auto it = codepoints->begin();
    const auto end = codepoints->end();

    for(char32_t c = 0; c < 0x10FFFF; c++) {
        if(auto p = std::ranges::lower_bound(it, end, c, {}, &cedilla::tools::codepoint::value); p != end && p->value == c && ! p->reserved) {
            CHECK((uint32_t)cedilla::cp_script(c)  == std::distance(scripts.begin(), std::ranges::find(scripts, p->script)));
            it = p+1;
        }
        else {
            CHECK(cedilla::cp_script(c)  == cedilla::script::unknown);
        }
    }
}


/*TEST_CASE("benchmarks") {
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    std::uniform_int_distribution<char32_t> distr(0, 0x10ffff); // define the range
    std::vector<char32_t> vec;
    vec.reserve(10000000);
    for(std::size_t i = 0 ; i < 10000000; i++)
        vec.push_back(distr(gen));

    BENCHMARK("binary_search") {
        auto count = 0;
        for(std::size_t i = 0; i != vec.size() ; i++)
            count += cedilla::details::skiplist_search(vec[i],
                                                       cedilla::details::generated::gc_letter.short_offset_runs,
                                                       cedilla::details::generated::gc_letter.offsets);
        return count;
    };

    BENCHMARK("linear_simd_256") {
        auto count = 0;
        for(std::size_t i = 0; i != vec.size() ; i++)
            count += cedilla::details::skiplist_search_simd(vec[i],
                                                        cedilla::details::generated::gc_letter.short_offset_runs,
                                                        cedilla::details::generated::gc_letter.offsets);
        return count;
    };

    BENCHMARK("linear_simd_256_2") {
        auto count = 0;
        for(std::size_t i = 0; i != vec.size() ; i++)
            count += cedilla::details::skiplist_search_simd_256_2(vec[i],
                                                            cedilla::details::generated::gc_letter.short_offset_runs,
                                                            cedilla::details::generated::gc_letter.offsets);
        return count;
    };


    BENCHMARK("linear_simd_128") {
        auto count = 0;
        for(std::size_t i = 0; i != vec.size() ; i++)
                count += cedilla::details::skiplist_search_simd_128(vec[i],
                                                                cedilla::details::generated::gc_letter.short_offset_runs,
                                                                cedilla::details::generated::gc_letter.offsets);
        return count;
    };

    BENCHMARK("linear_simd_128_2") {
        auto count = 0;
        for(std::size_t i = 0; i != vec.size() ; i++)
            count += cedilla::details::skiplist_search_simd_128_2(vec[i],
                                                                cedilla::details::generated::gc_letter.short_offset_runs,
                                                                cedilla::details::generated::gc_letter.offsets);
        return count;
    };
}
*/

