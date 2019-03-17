#pragma once

#include "unicode.h"
#include <catch2/catch.hpp>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>

template<typename I>
std::string n2hexstr(I w, size_t hex_len = sizeof(I) << 1) {
    static const char* digits = "0123456789ABCDEF";
    std::string rc(hex_len, '0');
    for(size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
        rc[i] = digits[(w >> j) & 0x0f];
    return rc;
}

inline std::ostream& operator<<(std::ostream& os, std::u32string const& value) {
    for(auto codepoint : value) {
        os << std::hex << "U+" << n2hexstr(codepoint, 8) << " ";
    }
    return os;
}

struct cp_test_data {
    char32_t cp;
    uni::version age;
    uni::category category;
    uni::block block;
    uni::script script;
};

std::unordered_map<char32_t, cp_test_data> load_test_data();