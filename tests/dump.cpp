#include <cedilla/details/generated_props.hpp>
#include <cassert>

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
int main() {
    assert(!cedilla::details::generated::property_upper.lookup(U'a'));
    assert(cedilla::details::generated::property_upper.lookup(U'A'));
    assert(!cedilla::details::generated::property_upper.lookup(U'z'));
    assert(cedilla::details::generated::property_upper.lookup(U'Z'));
}
