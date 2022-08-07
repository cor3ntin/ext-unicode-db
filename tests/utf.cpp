#include <cedilla/utf.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <range/v3/range/conversion.hpp>

TEST_CASE("decode utf8", "[utf]")
{
    using namespace std::literals;
    auto toU32 = [](std::u8string_view sv) {
        return cedilla::codepoint_view(std::move(sv)) | ranges::to<std::u32string>();
    };
    REQUIRE(toU32(u8"Test") == U"Test"sv);
    REQUIRE(toU32(u8"ΑβΓγΔ") == U"ΑβΓγΔ"sv);
    REQUIRE(toU32(u8"😀😀😀") == U"😀😀😀"sv);
}

TEST_CASE("decode utf16", "[utf]")
{
    using namespace std::literals;
    auto toU32 = [](std::u16string_view sv) {
        return cedilla::codepoint_view(std::move(sv)) | ranges::to<std::u32string>();
    };
    REQUIRE(toU32(u"Test") == U"Test"sv);
    REQUIRE(toU32(u"ΑβΓγΔ") == U"ΑβΓγΔ"sv);
    REQUIRE(toU32(u"😀😀😀") == U"😀😀😀"sv);
}

TEST_CASE("decode utf32", "[utf]")
{
    using namespace std::literals;
    auto toU32 = [](std::u32string_view sv) {
        return cedilla::codepoint_view(std::move(sv)) | ranges::to<std::u32string>();
    };
    REQUIRE(toU32(U"Test") == U"Test"sv);
    REQUIRE(toU32(U"ΑβΓγΔ") == U"ΑβΓγΔ"sv);
    REQUIRE(toU32(U"😀😀😀") == U"😀😀😀"sv);
}


TEST_CASE("reverse decode utf8", "[utf]")
{
    using namespace std::literals;
    auto toU32 = [](std::u8string_view sv) {
        return cedilla::codepoint_view(std::move(sv)) | std::views::reverse | ranges::to<std::u32string>();
    };
    REQUIRE(toU32(u8"Test") == U"tseT"sv);
    REQUIRE(toU32(u8"ΑβΓγΔ") == U"ΔγΓβΑ"sv);
    REQUIRE(toU32(u8"😀😀😀") == U"😀😀😀"sv);
}

TEST_CASE("reverse decode utf16", "[utf]")
{
    using namespace std::literals;
    auto toU32 = [](std::u16string_view sv) {
        return cedilla::codepoint_view(std::move(sv)) | std::views::reverse | ranges::to<std::u32string>();
    };
    REQUIRE(toU32(u"Test") == U"tseT"sv);
    REQUIRE(toU32(u"ΑβΓγΔ") == U"ΔγΓβΑ"sv);
    REQUIRE(toU32(u"😀😀😀") == U"😀😀😀"sv);
}


TEST_CASE("reverse decode utf32", "[utf]")
{
    using namespace std::literals;
    auto toU32 = [](std::u32string_view sv) {
        return cedilla::codepoint_view(std::move(sv)) | std::views::reverse | ranges::to<std::u32string>();
    };
    REQUIRE(toU32(U"Test") == U"tseT"sv);
    REQUIRE(toU32(U"ΑβΓγΔ") == U"ΔγΓβΑ"sv);
    REQUIRE(toU32(U"😀😀😀") == U"😀😀😀"sv);
}




