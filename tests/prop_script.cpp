#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

/*#include "common.h"
#include <cedilla/properties.hpp>

//#include "names.hpp"

const auto codes = load_test_data();

TEST_CASE("Verify that all code point have the same age as in the DB") {

    for(char32_t c = 0; c <= 0x10FFFF + 1; ++c) {
        auto expected = uni::version::unassigned;
        if(auto it = codes.find(c); it != codes.end())
            expected = it->second.age;
        else
            continue;
        REQUIRE(uni::cp_age(c) == expected);
    }
}

TEST_CASE("Verify that all code point have the block as in the DB") {

    for(char32_t c = 0; c <= 0x10FFFF + 1; ++c) {
        auto expected = uni::block::no_block;
        if(auto it = codes.find(c); it != codes.end())
            expected = it->second.block;
        else
            continue;
        CHECK(uni::cp_block(c) == expected);
    }
}

TEST_CASE("Verify that all code point have the same category as in the DB") {

    for(char32_t c = 0; c <= 0x10FFFF + 1; ++c) {
        auto expected = uni::category::unassigned;
        if(auto it = codes.find(c); it != codes.end())
            expected = it->second.category;
        else
            continue;
        // std::cout << "cat " << std::hex << c << std::dec << " " << int(uni::cp_category(c)) << "
        // "
        //          << int(expected) << "\n";
        CHECK(uni::cp_category(c) == expected);
    }
}


TEST_CASE("Verify that all code point have the script as in the DB") {

    REQUIRE(uni::cp_script(0x1312D) == uni::script::egyp);
    for(char32_t c = 0; c <= 0x10FFFF + 1; ++c) {
        auto expected = uni::script::unknown;
        if(auto it = codes.find(c); it != codes.end())
            expected = it->second.script;
        else
            continue;
        CHECK(uni::cp_script(c) == expected);
        //std::cout << "script " << std::hex << c << std::dec << " " << int(uni::cp_script(c)) << " "  << int(expected) << "\n";
    }
}

TEST_CASE("Verify that all code point have the script extensions as in the DB") {

    for(char32_t c = 0; c <= 0x10FFFF + 1; ++c) {
        auto it = codes.find(c);
        if(it == codes.end())
            continue;
        auto expected = it->second.extensions;
        std::vector<uni::script> extensions;
        auto v = uni::cp_script_extensions(c);
        for(auto s : v) {
            extensions.push_back(s);
        }
        using namespace Catch::Matchers;
        //std::cout << "script " << std::hex << c << std::dec << " " << int(uni::cp_script(c)) << "\n";

        CHECK_THAT(expected, UnorderedEquals(extensions));
    }
}


TEST_CASE("Verify that all code point have the numeric value as in the DB") {

    for(char32_t c = 0xbc; c <= 0x10FFFF + 1; ++c) {
        auto it = codes.find(c);
        if(it == codes.end())
            continue;
        auto d = it->second.d;
        auto n = it->second.n;
        auto nv = uni::cp_numeric_value(c);
        REQUIRE((d != 0 || !nv.is_valid()));
        REQUIRE(d == nv.denominator());
        REQUIRE(n == nv.numerator());
    }
}


TEST_CASE("Verify that all code point have the name as in the db") {

    for(char32_t c = 0x0; c <= 0x10FFFF + 1; ++c) {
        auto it = codes.find(c);
        if(it == codes.end())
            continue;
        auto name = it->second.name;
        if(name.empty())
            continue;

        std::string str;
        auto n = uni::cp_name(c);
        for(auto l : n) {
            str.push_back(l);
        }
        std::cout << "name " << std::hex << c << std::dec << " " << name << str << " \n";
        CHECK(str == name);
    }
}*/
