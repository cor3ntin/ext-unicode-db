#include <catch2/catch_all.hpp>
#include <regex>
#include <fstream>
#include "utils.hpp"
#include <range/v3/range/conversion.hpp>
#include <cedilla/normalization.hpp>
#include "globals.hpp"

using namespace cedilla::tools;

std::vector<normalization_test> parse_normalization_tests(std::string file) {
    std::regex re("([0-9A-F ]+);([0-9A-F ]+);([0-9A-F ]+);([0-9A-F ]+);([0-9A-F ]+);.*\\) (.+)",
                  std::regex::ECMAScript|std::regex::icase);
    std::ifstream infile(file);
    std::string line;
    std::vector<normalization_test> tests;
    tests.reserve(20000);
    while(std::getline(infile, line)) {
        std::smatch captures;
        if(line.starts_with("#"))
            continue;
        if(!std::regex_search(line, captures,  re))
            continue;
        normalization_test t;
        for (int i = 0; i <5; i++) {
            auto strs =  split(captures[i+1]);
            t.data[i] = strs | std::views::transform([](const std::string &s) {
                            return to_char32(s);
                        }) | ranges::to<std::u32string>();
        }
        t.description = captures[6];
        tests.push_back(std::move(t));
    }
    return tests;
}


TEST_CASE("NFD", "[normalization]") {
    auto nfd = [](const std::u32string_view & v) {
        return cedilla::normalization_view<cedilla::normalization_form::nfd, std::u32string_view>(v)
               | ranges::to<std::u32string>();
    };

    for(const auto & test : nt) {
        INFO(test.description);
        CHECK(test.c(3) == nfd(test.c(1)));
        CHECK(test.c(3) == nfd(test.c(2)));
        CHECK(test.c(3) == nfd(test.c(3)));

        CHECK(test.c(5) == nfd(test.c(4)));
        CHECK(test.c(5) == nfd(test.c(5)));
    }
}

TEST_CASE("NFC", "[normalization]") {
    auto nfc = [](const std::u32string_view & v) {
        return cedilla::normalization_view<cedilla::normalization_form::nfc, std::u32string_view>(v)
               | ranges::to<std::u32string>();
    };

    for(const auto & test : nt) {
        INFO(test.description);
        CHECK(test.c(2) == nfc(test.c(1)));
        CHECK(test.c(2) == nfc(test.c(2)));
        CHECK(test.c(2) == nfc(test.c(3)));

        CHECK(test.c(4) == nfc(test.c(4)));
        CHECK(test.c(4) == nfc(test.c(5)));
    }
}

TEST_CASE("ccc", "[normalization]") {
    auto it = codepoints->begin();
    const auto end = codepoints->end();

    for(char32_t c = 0; c < 0x11FFFF; c++) {
        INFO("cp: U+" << std::hex << (uint32_t)c << " ");
        if(auto p = std::ranges::lower_bound(it, end, c, {},
                                             &cedilla::tools::codepoint::value); p != end && p->value == c) {
            CHECK(p->ccc == cedilla::details::generated::ccc_data.lookup(c));
            it = p+1;
        } else {
            CHECK(cedilla::details::generated::ccc_data.lookup(c) == 0);
        }
    }
}
