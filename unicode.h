#pragma once
#include "props.h"
#include <cstring>
#include <string_view>
#include <optional>
#include <ratio>
#include <cmath>

namespace uni {

enum class property;


constexpr category cp_category(char32_t cp) {
    if(cp > 0x10FFFF)
        return category::unassigned;
    return __get_category(cp);
}

constexpr uni::version __age_from_string(std::string_view a) {
    for(auto i = 0; i < __age_strings.size(); ++i) {
        const auto res = __pronamecomp(a, __age_strings[i]);
        if(res == 0)
            return uni::version(i);
    }
    return uni::version::unassigned;
}

constexpr category __category_from_string(const std::string_view s) {
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

template<uni::version v = uni::version::standard_unicode_version>
constexpr script cp_script(char32_t cp) {
    static_assert(v >= uni::version::minimum_version,
                  "This version of the Unicode Database is not supported");
    return __cp_script<0, v>(cp);
}

template<uni::version v = uni::version::standard_unicode_version>
struct script_extensions_view {
    script_extensions_view(char32_t c) : c(c){};

    struct sentinel {};
    struct iterator {
        iterator(char32_t c) : m_c(c), m_script(__get_cp_script<v>(m_c, idx)) {
            if(m_script == script::unknown)
                m_script = __cp_script<0, v>(m_c);
        }
        script operator*() const {
            return m_script;
        };

        void operator++() {
            idx++;
            m_script = __get_cp_script<v>(m_c, idx);
        }

        bool operator==(sentinel s) const {
            return m_script == script::unknown;
        };
        bool operator!=(sentinel s) const {
            return m_script != script::unknown;
        };
        bool operator==(iterator it) const {
            return m_script == it.m_script && m_c == it.m_c;
        };
        bool operator!=(iterator it) const {
            return !(*this == it);
        };

    private:
        char32_t m_c;
        script m_script;
        int idx = 1;
    };

    iterator begin() const {
        return iterator{c};
    }

private:
    char32_t c;
};

template<uni::version v = uni::version::standard_unicode_version>
constexpr auto cp_script_extensions(char32_t cp) {
    static_assert(v >= uni::version::minimum_version,
                  "This version of the Unicode Database is not supported");
    if constexpr(v != uni::version::latest_version) {
        if(cp_age(cp) > v)
            return script::zzzz;
    }
    return script_extensions_view<v>(cp);
}


constexpr version cp_age(char32_t cp) {
    auto it = uni::upper_bound(__age_data.begin(), __age_data.end(), cp,
                               [](char32_t cp, const __age_data_t& a) { return cp < a.first; });
    if(it == __age_data.begin())
        return version::unassigned;
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

template<>
constexpr bool cp_is<property::noncharacter_code_point>(char32_t cp) {
    return (char32_t(cp) & 0xfffe) == 0xfffe || (char32_t(cp) >= 0xfdd0 && char32_t(cp) <= 0xfdef);
}

// http://unicode.org/reports/tr44/#Lowercase
template<>
constexpr bool cp_is<property::lowercase>(char32_t cp) {
    return uni::__cat_ll.lookup(char32_t(cp)) || uni::__prop_olower_data.lookup(char32_t(cp));
}

// http://unicode.org/reports/tr44/#Uppercase
template<>
constexpr bool cp_is<property::uppercase>(char32_t cp) {
    return uni::__cat_lu.lookup(char32_t(cp)) || uni::__prop_oupper_data.lookup(char32_t(cp));
}

// http://unicode.org/reports/tr44/#Cased
template<>
constexpr bool cp_is<property::cased>(char32_t cp) {
    return cp_is<property::lower>(cp) || cp_is<property::upper>(cp) ||
           uni::__cat_lt.lookup(char32_t(cp));
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

template<>
constexpr bool cp_is<property::default_ignorable_code_point>(char32_t cp) {
    const auto c = char32_t(cp);
    const bool maybe = uni::__prop_odi_data.lookup(cp) || uni::__cat_cf.lookup(cp) ||
                       uni::__prop_vs_data.lookup(cp);
    if(!maybe)
        return false;
    // ignore (Interlinear annotation format characters
    if(c >= 0xFFF9 && c <= 0xFFFB) {
        return false;
    }
    // Ignore Egyptian hieroglyph format characters
    else if(c >= 0x13430 && c <= 0x13438) {
        return false;
    } else if(uni::__prop_wspace_data.lookup(cp))
        return false;
    else if(uni::__prop_pcm_data.lookup(cp))
        return false;
    return true;
}

// http://www.unicode.org/reports/tr31/#D1
template<>
constexpr bool cp_is<property::id_start>(char32_t cp) {
    const bool maybe =
        cp_is<category::letter>(cp) || __cat_nl.lookup(cp) || __prop_oids_data.lookup(cp);
    if(!maybe)
        return false;
    return !__prop_pat_syn_data.lookup(cp) && !__prop_pat_ws_data.lookup(cp);
}

template<>
constexpr bool cp_is<property::id_continue>(char32_t cp) {
    const bool maybe = cp_is<category::letter>(cp) || __cat_nl.lookup(cp) ||
                       __prop_oids_data.lookup(cp) || __cat_mn.lookup(cp) || __cat_mc.lookup(cp) ||
                       __cat_nd.lookup(cp) || __cat_pc.lookup(cp) || __prop_oidc_data.lookup(cp);
    if(!maybe)
        return false;
    return !__prop_pat_syn_data.lookup(cp) && !__prop_pat_ws_data.lookup(cp);
}

template<typename Array, typename Res = long long>
constexpr bool _get_numeric_value(char32_t cp, const Array& array, Res& res) {
    auto it = uni::lower_bound(array.begin(), array.end(), cp,
                               [](const auto& d, char32_t cp) { return d.first < cp; });
    if(it == array.end() || it->first != cp)
        return false;
    res = it->second;
    return true;
}

constexpr numeric_value cp_numeric_value(char32_t cp) {
    long long res = 0;
    if(!(_get_numeric_value(cp, __numeric_data64, res) ||
         _get_numeric_value(cp, __numeric_data32, res) ||
         _get_numeric_value(cp, __numeric_data16, res) ||
         _get_numeric_value(cp, __numeric_data8, res))) {
        return {};
    }
    uint16_t d = 1;
    _get_numeric_value(cp, __numeric_data_d, d);
    return numeric_value(res, d);
}

}    // namespace uni