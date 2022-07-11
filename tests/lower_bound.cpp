#include <cedilla/properties.hpp>
#include <cedilla/normalization.hpp>
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

TEST_CASE("Test lower bound", "[sets]")
{
    const auto & a = cedilla::details::generated::canonical_decomposition_mapping_small_keys;
    for(int i = 0; i < 0xffff; i++) {

        REQUIRE(cedilla::details::branchless_lower_bound(a, i) == std::ranges::lower_bound(a, i));
    }
}
