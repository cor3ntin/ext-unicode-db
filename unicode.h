#pragma once
#include "props.h"
#include <cstring>
#include <string_view>
#include <optional>
#include <ratio>
#include <cmath>

namespace uni {
constexpr category cp_category(char32_t cp) {
    if(cp > 0x10FFFF)
        return category::unassigned;
    return __get_category(cp);
}

constexpr age __age_from_string(std::string_view a) {
    for(auto i = 0; i < __age_strings.size(); ++i) {
        const auto res = __pronamecomp(a, __age_strings[i]);
        if(res == 0)
            return age(i);
    }
    return age::unassigned;
}

constexpr category __category_from__string(const std::string_view s) {
    for(auto i = 0; i < __categories_names.size(); ++i) {
        const auto& c = __categories_names[i];
        const auto res = __pronamecomp(s, c.name);
        if(res == 0)
            return category(c.value);
    }
    return category::unassigned;
}

constexpr block __block_from_string(const std::string_view s) {
    for(auto i = 0; i < __blocks_names.size(); ++i) {
        const auto& c = __blocks_names[i];
        const auto res = __pronamecomp(s, c.name);
        if(res == 0)
            return block(c.value);
    }
    return block::no_block;
}

constexpr script __script_from_string(const std::string_view s) {
    for(auto i = 0; i < __scripts_names.size(); ++i) {
        const auto& c = __scripts_names[i];
        const auto res = __pronamecomp(s, c.name);
        if(res == 0)
            return script(c.value);
    }
    return script::unknown;
}


constexpr age cp_age(char32_t cp) {
    auto it = uni::upper_bound(__age_data.begin(), __age_data.end(), cp,
                               [](char32_t cp, const __age_data_t& a) { return cp < a.first; });
    if(it == __age_data.begin())
        return age::unassigned;
    it--;
    return it->a;
}

constexpr block cp_block(char32_t cp) {
    if(cp > 0x10FFFF)
        return block::no_block;
    auto it = uni::upper_bound(__block_data.begin(), __block_data.end(), cp,
                               [](char32_t cp, const __block_data_t& b) { return cp < b.first; });
    if(it == __block_data.begin())
        return block::no_block;
    it--;
    return it->b;
}

constexpr script cp_script(char32_t cp) {
    if(cp > 0x10FFFF)
        return script::unknown;

    constexpr const auto end = __scripts_data.begin() + __scripts_data_indexes[1];
    auto it = uni::upper_bound(__scripts_data.begin(), end, cp,
                               [](char32_t cp, const __script_data_t& s) { return cp < s.first; });
    if(it == end)
        return script::unknown;
    it--;
    return it->s;
}

constexpr bool cp_is_non_character(char32_t cp) {
    return (char32_t(cp) & 0xfffe) == 0xfffe || (char32_t(cp) >= 0xfdd0 && char32_t(cp) <= 0xfdef);
}

constexpr bool cp_is_lowercase(char32_t cp) {
    return uni::__cat_ll.lookup(char32_t(cp)) || uni::__prop_other_lower_data.lookup(char32_t(cp));
}

constexpr bool cp_is_uppercase(char32_t cp) {
    return uni::__cat_lu.lookup(char32_t(cp)) || uni::__prop_other_upper_data.lookup(char32_t(cp));
}

constexpr bool cp_is_cased(char32_t cp) {
    return cp_is_lowercase(cp) || cp_is_uppercase(cp) || uni::__cat_lt.lookup(char32_t(cp));
}


constexpr bool cp_is_alphabetic(char32_t cp) {
    return uni::__prop_alpha_data.lookup(char32_t(cp));
}

constexpr bool cp_is_whitespace(char32_t cp) {
    return uni::__prop_ws_data.lookup(char32_t(cp));
}

constexpr bool cp_is_valid(char32_t cp) {
    return char32_t(cp) <= 0x10FFFF;
}
constexpr bool cp_is_assigned(char32_t cp) {
    return uni::__prop_assigned.lookup(char32_t(cp));
}

constexpr bool cp_is_ascii(char32_t cp) {
    return char32_t(cp) <= 0x7F;
}

constexpr bool cp_is_ignorable(char32_t cp) {
    const auto c = char32_t(cp);
    const bool maybe = uni::__prop_other_ignorable_data.lookup(cp) || uni::__cat_cf.lookup(cp) ||
                       uni::__prop_variation_selector_data.lookup(cp);
    if(!maybe)
        return false;
    // ignore (Interlinear annotation format characters
    if(c >= 0xFFF9 && c <= 0xFFFB) {
        return false;
    }
    // Ignore Egyptian hieroglyph format characters
    else if(c >= 0x13430 && c <= 0x13438) {
        return false;
    } else if(uni::__prop_ws_data.lookup(cp))
        return false;
    else if(uni::__prop_prepend_concatenation_mark_data.lookup(cp))
        return false;
    return true;
}

// http://www.unicode.org/reports/tr31/#D1
constexpr bool cp_is_idstart(char32_t cp) {
    const bool maybe = __cp_is_letter(cp) || __cat_nl.lookup(cp) || __prop_oidstart_data.lookup(cp);
    if(!maybe)
        return false;
    return !__prop_pattern_syntax_data.lookup(cp) && !__prop_pattern_ws_data.lookup(cp);
}
constexpr bool cp_is_idcontinue(char32_t cp) {
    const bool maybe = __cp_is_letter(cp) || __cat_nl.lookup(cp) ||
                       __prop_oidstart_data.lookup(cp) || __cat_mn.lookup(cp) ||
                       __cat_mc.lookup(cp) || __cat_nd.lookup(cp) || __cat_pc.lookup(cp) ||
                       __prop_oidcontinue_data.lookup(cp);
    if(!maybe)
        return false;
    return !__prop_pattern_syntax_data.lookup(cp) && !__prop_pattern_ws_data.lookup(cp);
}


constexpr bool cp_is_xidstart(char32_t cp) {
    return __prop_xidstart_data.lookup(char32_t(cp));
}
constexpr bool cp_is_xidcontinue(char32_t cp) {
    return __prop_xidcontinue_data.lookup(char32_t(cp));
}

struct numeric_value {

    constexpr double value() const {
        return numerator() / double(_d);
    }

    constexpr intmax_t numerator() const {
        return _n * pow(10, _p);
    }

    constexpr intmax_t denominator() const {
        return _d;
    }

    constexpr bool is_valid() const {
        return _d != 0;
    }

protected:
    constexpr numeric_value() = default;
    constexpr numeric_value(uint8_t p, int16_t n, int16_t d) : _p(p), _n(n), _d(d) {}

    uint8_t _p = 0;
    int16_t _n = 0;
    int16_t _d = 0;
    friend constexpr numeric_value cp_numeric_value(char32_t cp);
};

constexpr numeric_value cp_numeric_value(char32_t cp) {
    auto it = lower_bound(__numeric_data.begin(), __numeric_data.end(), cp,
                          [](const __numeric_data_t& d, char32_t cp) { return d.c < cp; });
    if(it == __numeric_data.end() || it->c != cp)
        return {};
    return numeric_value{it->p, it->n, it->d};
}


}    // namespace uni