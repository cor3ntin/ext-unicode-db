#define CATCH_CONFIG_MAIN
#include <cedilla/name_to_cp.hpp>
#include <cedilla/cp_to_name.hpp>
#include <iostream>
#include <fmt/format.h>
#include <set>
#include "common.h"
#include <catch2/catch.hpp>

const auto codes = load_test_data();

TEST_CASE("Verify that all names have the same codepoint as in the DB") {

    for(char32_t c = 0; c <= 0x10FFFF + 1; ++c) {
        if(auto it = codes.find(c); it == codes.end()) {
            continue;
        }
        else {
            const auto & name = it->second.name;
            if(name.empty())
                continue;
            const auto res = uni::cp_from_name(name) ;
            //fmt::print("{} :  expected {:0x} found {:0x}\n", name, uint32_t(c), uint32_t(res) );
            CHECK(res == c);
        }
    }
}

TEST_CASE("Verify that all code point have the same name as in the DB") {
    for(char32_t c = 0; c <= 0x10FFFF + 1; ++c) {
        if(auto it = codes.find(c); it == codes.end()) {
            continue;
        }
        else {
            if(it->second.generated)
                continue;
            const auto & name = it->second.name;
            const auto res = uni::cp_name(c).to_string();
            fmt::print("{:0x} :  expected {} found {}\n", uint32_t(c)
            , name, res );
            CHECK(res == name);
        }
    }
}