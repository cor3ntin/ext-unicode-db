#define CATCH_CONFIG_MAIN
#include "common.h"
#include <ext/unicode.hpp>
#include <catch2/catch.hpp>

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

    for(char32_t c = 907; c <= 0x10FFFF + 1; ++c) {
        auto expected = uni::block::no_block;
        if(auto it = codes.find(c); it != codes.end())
            expected = it->second.block;
        else
            continue;
        // std::cout << c << " " << int(uni::cp_block(c)) << " " << int(expected) << "\n";
        REQUIRE(uni::cp_block(c) == expected);
    }
}

TEST_CASE("Verify that all code point have the same category as in the DB") {

    for(char32_t c = 0x856; c <= 0x10FFFF + 1; ++c) {
        auto expected = uni::category::unassigned;
        if(auto it = codes.find(c); it != codes.end())
            expected = it->second.category;
        else
            continue;
        std::cout << "cat " << std::hex << c << std::dec << " " << int(uni::cp_category(c)) << " "
                  << int(expected) << "\n";
        REQUIRE(uni::cp_category(c) == expected);
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
        REQUIRE(uni::cp_script(c) == expected);
    }
}