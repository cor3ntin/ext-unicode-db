
#pragma once
#include <iterator>
#include "cedilla/generated_props.hpp"

namespace uni {

constexpr category cp_category(char32_t cp) {
    if(cp > 0x10FFFF)
        return category::unassigned;
    return detail::tables::get_category(cp);
}

constexpr uni::version detail::age_from_string(std::string_view a) {
    for(std::size_t i = 0; i < std::size(detail::tables::age_strings); ++i) {
        const auto res = detail::propnamecomp(a, detail::tables::age_strings[i]);
        if(res == 0)
            return uni::version(i);
    }
    return uni::version::unassigned;
}

constexpr category detail::category_from_string(std::string_view s) {
    for(const auto& c : detail::tables::categories_names) {
        const auto res = detail::propnamecomp(s, c.name);
        if(res == 0)
            return category(c.value);
    }
    return category::unassigned;
}

constexpr block detail::block_from_string(std::string_view s) {
    for(const auto& c : detail::tables::blocks_names) {
        const auto res = detail::propnamecomp(s, c.name);
        if(res == 0)
            return block(c.value);
    }
    return block::no_block;
}

constexpr script detail::script_from_string(std::string_view s) {
    for(const auto& c : detail::tables::scripts_names) {
        const auto res = detail::propnamecomp(s, c.name);
        if(res == 0)
            return script(c.value);
    }
    return script::unknown;
}

constexpr bool detail::is_unassigned(category cat)
{
    return cat == category::unassigned;
}

constexpr bool detail::is_unknown(script s)
{
    return s == script::unknown;
}

constexpr bool detail::is_unknown(block b) {
    return b == block::no_block;
}

constexpr bool detail::is_unassigned(version v)
{
    return v == version::unassigned;
}

constexpr script cp_script(char32_t cp) {
    return detail::tables::cp_script<0>(cp);
}

constexpr script_extensions_view::script_extensions_view(char32_t c_) : c(c_){}

constexpr script_extensions_view::iterator::iterator(char32_t c_) : m_c(c_), m_script(detail::tables::get_cp_script(m_c, 1)) {
    if(m_script == script::unknown)
        m_script = detail::tables::cp_script<0>(m_c);
    }

constexpr script script_extensions_view::iterator::operator*() const {
    return m_script;
}

constexpr auto script_extensions_view::iterator::operator++(int) ->iterator & {
    idx++;
    m_script = detail::tables::get_cp_script(m_c, idx);
    return *this;
}

constexpr auto script_extensions_view::iterator::operator++()  -> iterator {
    auto c = *this;
    idx++;
    m_script = detail::tables::get_cp_script(m_c, idx);
    return c;
}

constexpr bool script_extensions_view::iterator::operator==(sentinel) const {
    return m_script == script::unknown;
}

constexpr bool script_extensions_view::iterator::operator!=(sentinel) const {
    return m_script != script::unknown;
}

/*constexpr bool script_extensions_view::iterator::operator==(iterator it) const {
    return m_script == it.m_script && m_c == it.m_c;
};
constexpr bool script_extensions_view::iterator::operator!=(iterator it) const {
    return !(*this == it);
};*/

constexpr script_extensions_view::iterator script_extensions_view::begin() const {
    return iterator{c};
}
constexpr script_extensions_view::sentinel script_extensions_view::end() const {
    return {};
}

constexpr script_extensions_view cp_script_extensions(char32_t cp) {
    return script_extensions_view(cp);
}


constexpr version cp_age(char32_t cp) {
    return static_cast<version>(detail::tables::age_data.value(cp, uint8_t(version::unassigned)));
}

constexpr block cp_block(char32_t cp) {
    const auto end = std::end(detail::tables::block_data._data);
    auto it = detail::upper_bound(std::begin(detail::tables::block_data._data), end, cp, [](char32_t cp_, uint32_t v) {
        char32_t c = (v >> 8);
        return cp_ < c;
    });
    if(it == end)
        return block::no_block;
    it--;
    auto offset = (*it) & 0xFF;
    if(offset == 0)
        return block::no_block;
    offset--;

    const auto d = std::distance(std::begin(detail::tables::block_data._data), it);
    return uni::block((d - offset) + 1);
}

template<>
constexpr bool cp_property_is<property::noncharacter_code_point>(char32_t cp) {
    return (char32_t(cp) & 0xfffe) == 0xfffe || (char32_t(cp) >= 0xfdd0 && char32_t(cp) <= 0xfdef);
}

// http://unicode.org/reports/tr44/#Lowercase
template<>
constexpr bool cp_property_is<property::lowercase>(char32_t cp) {
    return detail::tables::cat_ll.lookup(char32_t(cp)) || detail::tables::prop_olower_data.lookup(char32_t(cp));
}

// http://unicode.org/reports/tr44/#Uppercase
template<>
constexpr bool cp_property_is<property::uppercase>(char32_t cp) {
    return detail::tables::cat_lu.lookup(char32_t(cp)) || detail::tables::prop_oupper_data.lookup(char32_t(cp));
}

// http://unicode.org/reports/tr44/#Cased
template<>
constexpr bool cp_property_is<property::cased>(char32_t cp) {
    return cp_property_is<property::lower>(cp) || cp_property_is<property::upper>(cp) ||
           detail::tables::cat_lt.lookup(char32_t(cp));
}

// http://unicode.org/reports/tr44/#Math
template<>
constexpr bool cp_property_is<property::math>(char32_t cp) {
    return detail::tables::cat_sm.lookup(char32_t(cp)) || detail::tables::prop_omath_data.lookup(cp);
}

// http://unicode.org/reports/tr44/#Case_Ignorable
template<>
constexpr bool cp_property_is<property::case_ignorable>(char32_t) {
    return false;
}


// http://unicode.org/reports/tr44/#Grapheme_Extend
template<>
constexpr bool cp_property_is<property::grapheme_extend>(char32_t cp) {
    return detail::tables::cat_me.lookup(char32_t(cp)) || detail::tables::cat_mn.lookup(char32_t(cp)) ||
           detail::tables::prop_ogr_ext_data.lookup(cp);
}

constexpr bool cp_is_valid(char32_t cp) {
    return char32_t(cp) <= 0x10FFFF;
}
constexpr bool cp_is_assigned(char32_t cp) {
    return detail::tables::prop_assigned.lookup(char32_t(cp));
}

constexpr bool cp_is_ascii(char32_t cp) {
    return char32_t(cp) <= 0x7F;
}

template<>
constexpr bool cp_property_is<property::default_ignorable_code_point>(char32_t cp) {
    const auto c = char32_t(cp);
    const bool maybe = detail::tables::prop_odi_data.lookup(cp) || detail::tables::cat_cf.lookup(cp) ||
                       detail::tables::prop_vs_data.lookup(cp);
    if(!maybe)
        return false;
    // ignore (Interlinear annotation format characters
    if(c >= 0xFFF9 && c <= 0xFFFB) {
        return false;
    }
    // Ignore Egyptian hieroglyph format characters
    else if(c >= 0x13430 && c <= 0x13438) {
        return false;
    } else if(detail::tables::prop_wspace_data.lookup(cp))
        return false;
    else if(detail::tables::prop_pcm_data.lookup(cp))
        return false;
    return true;
}

// http://www.unicode.org/reports/tr31/#D1
template<>
constexpr bool cp_property_is<property::id_start>(char32_t cp) {
    const bool maybe =
        cp_category_is<category::letter>(cp) || detail::tables::cat_nl.lookup(cp) || detail::tables::prop_oids_data.lookup(cp);
    if(!maybe)
        return false;
    return !detail::tables::prop_pat_syn_data.lookup(cp) && !detail::tables::prop_pat_ws_data.lookup(cp);
}

template<>
constexpr bool cp_property_is<property::id_continue>(char32_t cp) {
    const bool maybe = cp_category_is<category::letter>(cp) || detail::tables::cat_nl.lookup(cp) ||
                       detail::tables::prop_oids_data.lookup(cp) || detail::tables::cat_mn.lookup(cp) || detail::tables::cat_mc.lookup(cp) ||
                       detail::tables::cat_nd.lookup(cp) || detail::tables::cat_pc.lookup(cp) || detail::tables::prop_oidc_data.lookup(cp);
    if(!maybe)
        return false;
    return !detail::tables::prop_pat_syn_data.lookup(cp) && !detail::tables::prop_pat_ws_data.lookup(cp);
}

namespace detail {

template<typename Array, typename Res = long long>
constexpr bool get_numeric_value(char32_t cp, const Array& array, Res& res) {
    auto it = detail::lower_bound(std::begin(array), std::end(array), cp,
                               [](const auto& d, char32_t cp_) { return d.first < cp_; });
    if(it == std::end(array) || it->first != cp)
        return false;
    res = it->second;
    return true;
}

}

constexpr numeric_value cp_numeric_value(char32_t cp) {
    long long res = 0;
    if(!(detail::get_numeric_value(cp, detail::tables::numeric_data64, res) ||
             detail::get_numeric_value(cp, detail::tables::numeric_data32, res) ||
             detail::get_numeric_value(cp, detail::tables::numeric_data16, res) || [&res, cp]() -> bool {
           res = detail::tables::numeric_data8.value(cp, 255);
           return res != 255;
       }())) {
        return {};
    }
    int16_t d = 1;
    detail::get_numeric_value(cp, detail::tables::numeric_data_d, d);
    return numeric_value(res, d);
}


}    // namespace uni

namespace std
{
    template<>
    struct iterator_traits<uni::script_extensions_view::iterator>
    {
        using difference_type = std::ptrdiff_t;
        using value_type = uni::script;
        using pointer =  uni::script *;
        using reference	=  uni::script;
        using iterator_category = std::forward_iterator_tag;
    };
}
