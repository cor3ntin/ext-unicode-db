#include <cedilla/details/generated_props.hpp>
#include <cassert>
#include <fmt/format.h>
#include "fmt/color.h"
#include "load.hpp"

/*#include <cedilla/casing.hpp>

#include <string_view>
#include <iostream>
#include <locale>
#include <codecvt>


std::string toUtf8(std::u32string_view s) {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert;
    return convert.to_bytes(s.data(), s.data() + s.size());
}

template <std::ranges::input_range R>
requires std::same_as<std::ranges::range_value_t<R>, char32_t>
std::ostream& operator<<(std::ostream& os, const R& r) {
    std::u32string s(r.begin(), r.end());
    return os << toUtf8(s);
}
*/
int main(int, char**) {
    /*auto codepoints = cedilla::tools::load_codepoints(argv[1]);
    std::unordered_map<char32_t, cedilla::tools::codepoint> map;
    std::vector<bool> props(0x10FFFF, 0);
    for(const auto & c : codepoints) {
        map[c.value] = c;
        if(c.has_binary_property("alpha"))
            props[c.value] = 1;
    }*/

    char32_t x = 0;
    for(char32_t i = 0; i < 0x10FFFF; i++) {
        //auto e =  props[i];
        x+=  cedilla::details::generated::property_alpha.lookup(i);
        //if(e != r)
        //    fmt::print(fmt::fg(e == r? fmt::color::cyan : fmt::color::orange_red), "'U+{:04X}': {} <> {} \n", i, r, e);
    }
    return x;
}
