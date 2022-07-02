#pragma once
#include <string>
#include <algorithm>
#include <sstream>
#include <ranges>
#include <charconv>
#include <fmt/format.h>

namespace cedilla::tools {

inline std::string to_lower(std::string s) {
    std::ranges::transform(s, s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

inline bool is_in(std::string_view value, std::initializer_list<const char*> lst) {
    return std::ranges::find(lst, value) != std::ranges::end(lst);
}

inline bool ci_equal(std::string_view a, std::string_view b) {
    auto lower = std::ranges::views::transform([](char c) ->char { return std::tolower(c); });
    return std::ranges::equal(a | lower, b | lower);
}

inline std::vector<std::string> split(const std::string & str) {
    std::istringstream iss(str);
    std::vector<std::string> results((std::istream_iterator<std::string>(iss)),
                                     std::istream_iterator<std::string>());
    return results;
}

inline char32_t to_char32(std::string_view v) {
    std::uint32_t out;
    std::from_chars(v.data(), v.data()+v.size(), out, 16);
    return static_cast<char32_t>(out);
}

inline std::string to_hex(char32_t v) {
    return fmt::format("{:04X}", v);
}

template <typename R, typename Type>
concept range_of = std::ranges::input_range<R> &&
                   std::same_as<std::ranges::range_value_t<std::remove_cvref_t<R>>, Type>;

}
