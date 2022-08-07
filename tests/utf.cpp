#include <cedilla/utf.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <range/v3/range/conversion.hpp>
#include "globals.hpp"


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


TEST_CASE("encode utf8", "[utf]")
{
    using namespace std::literals;
    auto toU8 = [](std::u32string_view sv) {
        return sv | cedilla::to_utf8 | ranges::to<std::u8string>();
    };
    REQUIRE(toU8(U"Test") == u8"Test"sv);
    REQUIRE(toU8(U"ΑβΓγΔ") == u8"ΑβΓγΔ"sv);
    REQUIRE(toU8(U"😀😀😀") == u8"😀😀😀"sv);
}

TEST_CASE("encode utf16", "[utf]")
{
    using namespace std::literals;
    auto toU16 = [](std::u32string_view sv) {
        return sv | cedilla::to_utf16 | ranges::to<std::u16string>();
    };
    REQUIRE(toU16(U"Test") == u"Test"sv);
    REQUIRE(toU16(U"ΑβΓγΔ") == u"ΑβΓγΔ"sv);
    REQUIRE(toU16(U"😀😀😀") == u"😀😀😀"sv);
}
