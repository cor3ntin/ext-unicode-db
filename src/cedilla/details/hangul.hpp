#pragma once
#include <cedilla/details/generated_normalization.hpp>

namespace cedilla::details {


enum class hangul_syllable_kind {
    invalid,
    leading   = 0x01,
    vowel     = 0x02,
    trailing  = 0x04
};

constexpr inline char32_t hangul_lbase = 0x1100;    // First leading Jamo
static const char32_t hangul_lcount = 19;
static const char32_t hangul_vbase = 0x1161;    // First vowel
static const char32_t hangul_vcount = 21;
static const char32_t hangul_tbase = 0x11A7;    // First trailing
static const char32_t hangul_tcount = 28;
static const char32_t hangul_sbase = 0xAC00;    // First composed
static const char32_t hangul_ncount = 588;
static const char32_t hangul_scount = 11172;

constexpr inline bool is_decomposable_hangul(char32_t codepoint) {
    return codepoint >= hangul_sbase && codepoint <= 0xD7A3;
}

constexpr inline char32_t hangul_syllable_index(char32_t codepoint) {
    return codepoint - hangul_sbase;
}

constexpr inline bool is_hangul_lpart(char32_t codepoint) {
    return codepoint >= 0x1100 && codepoint <= 0x1112;
}

constexpr inline bool is_hangul_vpart(char32_t codepoint) {
    return codepoint >= 0x1160 && codepoint <= 0x1175;
}

constexpr inline bool is_hangul_tpart(char32_t codepoint) {
    return codepoint >= 0x11A7 && codepoint <= 0x11C2;
}

constexpr inline bool is_hangul(char32_t codepoint) {
    return (codepoint >= 0x1100 && codepoint < 0x11C2) || is_decomposable_hangul(codepoint);
}

inline hangul_syllable_kind hangul_syllable_type(char32_t codepoint) {
    if(is_hangul_lpart(codepoint))
        return hangul_syllable_kind::leading;
    if(is_hangul_vpart(codepoint))
        return hangul_syllable_kind::vowel;
    if(is_hangul_tpart(codepoint))
        return hangul_syllable_kind::trailing;

    if(!is_decomposable_hangul(codepoint))
        return hangul_syllable_kind::invalid;

    const auto & table = details::generated::hangul_syllables;
    if(codepoint < table->cp)
        return hangul_syllable_kind::invalid;
    auto it = details::branchless_lower_bound(table, codepoint,
                                              [] (details::generated::hangul_syllable s, char32_t needle) {
                                                  return s.cp < needle;
                                              });

    if(it == std::ranges::end(table))
        return hangul_syllable_kind::invalid;

    return hangul_syllable_kind(it->kind);
}

}
