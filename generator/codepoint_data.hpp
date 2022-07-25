#pragma once
#include <string>
#include <unordered_set>
#include <vector>

namespace cedilla::tools {

int binary_property_index(std::string_view);

enum class hangul_syllable_kind {
    invalid,
    leading   = 0x01,
    vowel     = 0x02,
    trailing  = 0x04
};


enum class grapheme_cluster_break : uint8_t{
    any,

    hangul_l   = uint8_t(hangul_syllable_kind::leading),
    hangul_v   = uint8_t(hangul_syllable_kind::vowel),
    hangul_t   = uint8_t(hangul_syllable_kind::trailing),
    hangul_lv  = hangul_l | hangul_v,
    hangul_lvt = hangul_lv | hangul_t,

    control    = 0x10,
    prepend    = 0x20,
    extend     = 0x30,
    regional_indicator = 0x40,
    extended_pictographic = 0x50,
    spacing_mark = 0x60,
    cr  = 0x70,
    lf  = 0x80,
    zwj = 0x90
};

enum class word_break : uint8_t{
    any,
    cr,
    double_quote,
    extended_num_let,
    extend,
    format,
    hebrew_letter,
    katakana,
    aletter,
    lf,
    midnumlet,
    midletter,
    midnum,
    newline,
    numeric,
    regional_indicator,
    single_quote,
    wseg_space,
    extended_pictographic,
    zwj
};

struct codepoint {
    char32_t    value = 0;
    std::string name;
    std::unordered_set<std::int8_t> binary_properties;
    std::string age;
    bool reserved = false;
    std::string general_category;
    std::string script;
    std::vector<std::string> script_extensions;
    std::string numeric_value_str;
    std::int64_t numeric_value_numerator = 0 , numeric_value_denominator = 0;
    std::vector<char32_t> uppercase, lowercase, titlecase, casefold;
    std::vector<char32_t> decomposition;
    bool canonical_decomposition = true;
    std::vector<std::string> aliases;
    int ccc = 0;
    bool NFD_QC = true;
    // Assume Maybe == No
    bool NFC_QC = true;
    grapheme_cluster_break gcb = grapheme_cluster_break::any;
    word_break wb = word_break::any;

    uint8_t hangul_syllable_kind = uint8_t(hangul_syllable_kind::invalid);

    bool is_hangul() const {
        return hangul_syllable_kind != uint8_t(hangul_syllable_kind::invalid);
    }

    bool has_binary_property(std::string_view prop) const {
        return binary_properties.contains(binary_property_index(prop));
    }
};


}
