#pragma once
#include <string>
#include <vector>
#include <array>
#include <optional>
#include "codepoint_data.hpp"
#include "load.hpp"
#include <catch2/catch_tostring.hpp>
#include <fmt/core.h>


struct normalization_test {
    std::array<std::u32string, 5> data;
    std::string description;
    const auto & c(int i) const {
        return data[i-1];
    }
};

struct boundary_test {
    std::string description;
    std::vector<std::u32string> expected;
    std::u32string input;
};

inline std::vector<normalization_test> nt;
std::vector<normalization_test> parse_normalization_tests(std::string file);

inline std::optional<std::vector<cedilla::tools::codepoint>> codepoints;
inline std::optional<cedilla::tools::labels> labels;

std::vector<boundary_test> parse_boundaries_tests(std::string file);

inline std::vector<boundary_test> grapheme_tests;
inline std::vector<boundary_test> word_break_tests;


namespace Catch {
template<>
struct StringMaker<char32_t> {
    static std::string convert( char32_t c ) {
        return fmt::format("U+{:04X}", (uint32_t)c);
    }
};

}
