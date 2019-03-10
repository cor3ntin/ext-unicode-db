#pragma once
#include "props.h"
#include <cstring>
#include <string_view>

namespace uni {
enum class codepoint : char32_t {};
constexpr category cp_category(codepoint cp) {
    if((char32_t)cp > 0x10FFFF)
        return category::unassigned;
    return __get_category((char32_t)cp);
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


constexpr age cp_age(codepoint cp) {
    auto it = uni::upper_bound(__age_data.begin(), __age_data.end(), (char32_t)cp,
                               [](char32_t cp, const __age_data_t& a) { return cp < a.first; });
    if(it == __age_data.begin())
        return age::unassigned;
    it--;
    return it->a;
}

constexpr block cp_block(codepoint cp) {
    if((char32_t)cp > 0x10FFFF)
        return block::no_block;
    auto it = uni::upper_bound(__block_data.begin(), __block_data.end(), (char32_t)cp,
                               [](char32_t cp, const __block_data_t& b) { return cp < b.first; });
    if(it == __block_data.begin())
        return block::no_block;
    it--;
    return it->b;
}

constexpr script cp_script(codepoint cp) {
    if((char32_t)cp > 0x10FFFF)
        return script::unknown;

    constexpr const auto end = __scripts_data.begin() + __scripts_data_indexes[1];
    auto it = uni::upper_bound(__scripts_data.begin(), end, (char32_t)cp,
                               [](char32_t cp, const __script_data_t& s) { return cp < s.first; });
    if(it == end)
        return script::unknown;
    it--;
    return it->s;
}

constexpr bool cp_is_non_character(codepoint cp) {
    return (char32_t(cp) & 0xfffe) == 0xfffe || (char32_t(cp) >= 0xfdd0 && char32_t(cp) <= 0xfdef);
}

constexpr bool cp_is_lowercase(codepoint cp) {
    return uni::__cat_ll.lookup(char32_t(cp)) || uni::__prop_other_lower_data.lookup(char32_t(cp));
}

constexpr bool cp_is_uppercase(codepoint cp) {
    return uni::__cat_lu.lookup(char32_t(cp)) || uni::__prop_other_upper_data.lookup(char32_t(cp));
}

constexpr bool cp_is_cased(codepoint cp) {
    return cp_is_lowercase(cp) || cp_is_uppercase(cp) || uni::__cat_lt.lookup(char32_t(cp));
}


constexpr bool cp_is_alphabetic(codepoint cp) {
    return uni::__prop_alpha_data.lookup(char32_t(cp));
}

constexpr bool cp_is_whitespace(codepoint cp) {
    return uni::__prop_ws_data.lookup(char32_t(cp));
}

constexpr bool cp_is_valid(codepoint cp) {
    return char32_t(cp) <= 0x10FFFF;
}
constexpr bool cp_is_assigned(codepoint cp) {
    return uni::__prop_assigned.lookup(char32_t(cp));
}

constexpr bool cp_is_ascii(codepoint cp) {
    return char32_t(cp) <= 0x7F;
}

constexpr bool cp_is_ignorable(codepoint cp) {
    const auto c = char32_t(cp);
    const bool maybe = uni::__prop_other_ignorable_data.lookup(c) || uni::__cat_cf.lookup(c) ||
                       uni::__prop_variation_selector_data.lookup(c);
    if(!maybe)
        return false;
    // ignore (Interlinear annotation format characters
    if(c >= 0xFFF9 && c <= 0xFFFB) {
        return false;
    }
    // Ignore Egyptian hieroglyph format characters
    else if(c >= 0x13430 && c <= 0x13438) {
        return false;
    } else if(uni::__prop_ws_data.lookup(c))
        return false;
    else if(uni::__prop_prepend_concatenation_mark_data.lookup(c))
        return false;
    return true;
}

// http://www.unicode.org/reports/tr31/#D1
constexpr bool cp_is_idstart(codepoint cp) {
    const auto c = char32_t(cp);
    const bool maybe = __cp_is_letter(c) || __cat_nl.lookup(c) || __prop_oidstart_data.lookup(c);
    if(!maybe)
        return false;
    return !__prop_pattern_syntax_data.lookup(c) && !__prop_pattern_ws_data.lookup(c);
}
constexpr bool cp_is_idcontinue(codepoint cp) {
    const auto c = char32_t(cp);
    const bool maybe = __cp_is_letter(c) || __cat_nl.lookup(c) || __prop_oidstart_data.lookup(c) ||
                       __cat_mn.lookup(c) || __cat_mc.lookup(c) || __cat_nd.lookup(c) ||
                       __cat_pc.lookup(c) || __prop_oidcontinue_data.lookup(c);
    if(!maybe)
        return false;
    return !__prop_pattern_syntax_data.lookup(c) && !__prop_pattern_ws_data.lookup(c);
}


constexpr bool cp_is_xidstart(codepoint cp) {
    return __prop_xidstart_data.lookup(char32_t(cp));
}
constexpr bool cp_is_xidcontinue(codepoint cp) {
    return __prop_xidcontinue_data.lookup(char32_t(cp));
}


}    // namespace uni