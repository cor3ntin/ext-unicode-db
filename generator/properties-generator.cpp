#include "load.hpp"
#include "fmt/format.h"
#include "fmt/color.h"
#include <filesystem>
#include <ranges>
#include "generators.hpp"
#include "utils.hpp"

using namespace cedilla::tools;

namespace cedilla::tools {

void print_categories_data(FILE* out, const std::vector<codepoint> & all, const labels & l);
void print_scripts_data(FILE* out, const std::vector<codepoint> & all, const labels & l);

}

void usage() {
    fmt::print(fg(fmt::color::red),
               "Usage : unicode-properties-data-generator ucd.nounihan.flat.xml PropertyValueAliases.txt output.hpp\n");
    exit(1);
}

const char* useful_properties[] = {
    "ahex",
    "alpha",
    "dash",
    "hex",
    // "hyphen", deprecated
    "qmark",
    "term",
    "sterm",
    "di", // Default_Ignorable_Code_Point
    "dia",
    "ext",  // Extender,
    "sd",   // Soft_Dotted
    "math",
    "wspace",
    "ri",
    "nchar", //Noncharacter_Code_Point
    "vs",

    // emojis ?
    "emoji",
    "epres",
    "emod",
    "ebase",
    //"ecomp",
    //"expict",

    "pat_syn",
    "pat_ws",

    "xids", // XID_Start
    "xidc", // XID_Continue

    "ideo", // Ideographic

    "upper",
    "lower"
};


void  print_binary_properties(FILE* output, const std::vector<codepoint> & codepoints) {
    for(auto prop : useful_properties) {
        auto set = codepoints | std::views::filter ([prop](const codepoint & c) {
                   return c.has_binary_property(prop);
                   }) | std::views::transform(&codepoint::value) | ranges::to<std::vector>();
        print_binary_data_best(output, set, fmt::format("property_{}", to_lower(prop)));
    }
}

void print_binary_property_public_interface(FILE* output){
    fmt::print(output, "namespace cedilla {{");

    for(auto prop : useful_properties) {
        fmt::print(output, R"_(
inline constexpr bool is_{0}(char32_t c) noexcept {{
       return details::generated::property_{0}.lookup(c);
}}
)_", prop);
    }
    fmt::print(output, "}}\n");
}

void  print_casing_data(FILE* output, const std::vector<codepoint> & codepoints) {

    auto properties = std::array {
         std::pair{"lc", &codepoint::lowercase},
         std::pair{"uc", &codepoint::uppercase},
         std::pair{"tc", &codepoint::titlecase},
         std::pair{"cf", &codepoint::casefold}
    };
    for(auto [name, proj] : properties) {
        std::vector<std::size_t> starts {0};
        std::vector<std::string> values;

        for(std::size_t i = 0; ; i++) {
            auto set = codepoints | std::views::filter ([proj = proj, i](const codepoint & c) {
                           return std::invoke(proj, c).size() > i;
            }) | std::views::transform([proj = proj, i](const codepoint & c) {
                           const auto & values =  std::invoke(proj, c);
                           auto mapped = values[i];
                           if(i+ 1 < values.size())
                               mapped |= (1 << 31);
                           return std::tuple{c.value, mapped};
            }) | ranges::to<std::vector>();
            starts.push_back(starts.back() + set.size());
            if(set.size() == 0) {
                break;
            }
            starts.push_back(starts.back() + set.size());
            for(auto && [k, v] : set) {
                values.push_back(fmt::format("{:#04x}, {:#04x}", uint32_t(k), uint32_t(v)));
            }
        }
        fmt::print(output, "inline constexpr char32_t casing_data_{}[{}][2] = {{ {} }};", name, values.size(), fmt::join(values, ", "));
        fmt::print(output, "inline constexpr std::size_t casing_starts_{}[]  = {{ {} }};", name, fmt::join(starts, ", "));
        fmt::print(output, "inline constexpr std::size_t casing_max_len_{}  = {};", name, starts.size());
    }
}

void  print_grapheme_cluster_data(FILE* output, const std::vector<codepoint> & codepoints) {
    auto set = codepoints | std::views::filter ([](const codepoint & c) {
                   return !c.is_hangul() && c.gcb != grapheme_cluster_break::any;
    });
    std::vector<char32_t> keys = set
                                 | std::views::transform(&codepoint::value)
                                 | ranges::to<std::vector>;

    hash_data data = create_perfect_hash(keys);

    std::map<char32_t, uint8_t> gcb;
    std::ranges::transform(
        set,
        std::inserter(gcb, gcb.begin()), [](const codepoint & c) { return std::pair{c.value, uint8_t(c.gcb)};});

    std::vector<uint32_t> values = data.keys | std::views::transform([&](char32_t c) {
                                       return (uint32_t(c) << 11) | gcb[c];
                                   }) | ranges::to<std::vector>;

    print_hash_data(output, "lower11bits_codepoint_hash_map", "grapheme_break_data", data.salts, values);

}

void  print_word_break_data(FILE* output, const std::vector<codepoint> & codepoints) {
    auto set = codepoints | std::views::filter ([](const codepoint & c) {
                   // storing all alphabetics makes for an unecessary big table
                   // client should query the is_alpha property instead and deduce word_break::aletter
                   return c.wb != word_break::any && (c.wb != word_break::aletter || !c.has_binary_property("alpha"));
               });
    std::vector<char32_t> keys = set
                                 | std::views::transform(&codepoint::value)
                                 | ranges::to<std::vector>;

    hash_data data = create_perfect_hash(keys);

    std::map<char32_t, uint8_t> gcb;
    std::ranges::transform(
        set,
        std::inserter(gcb, gcb.begin()), [](const codepoint & c) { return std::pair{c.value, uint8_t(c.wb)};});

    std::vector<uint32_t> values = data.keys | std::views::transform([&](char32_t c) {
                                       return (uint32_t(c) << 11) | gcb[c];
                                   }) | ranges::to<std::vector>;

    print_hash_data(output, "lower11bits_codepoint_hash_map", "word_break_data", data.salts, values);
}


void print_header(FILE* out) {
    fmt::print(out, R"(
#pragma once
#include "cedilla/details/sets.hpp"
#include "cedilla/details/scripts.hpp"
namespace cedilla::details::generated {{
)");
}

void print_footer(FILE* out) {
    fmt::print(out, "}}\n");
}

int main(int argc, const char** argv) {
    if(argc != 4 || !std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        usage();
    }
    auto ucd_path = argv[1];
    auto property_aliases_path = argv[2];
    auto output_path = argv[3];

    auto codepoints = load_codepoints(ucd_path);
    fmt::print("loaded {}\n", ucd_path);

    auto property_file = fopen(property_aliases_path, "r");
    labels lbs = load_labels(property_aliases_path);
    fmt::print("loaded categories from {}\n", property_aliases_path);

    fflush(stdout);
    auto output = fopen(output_path, "w");
    if(!output) {
        die("can't open output file");
    }
    print_header(output);
    print_categories_data(output, codepoints, lbs);
    print_scripts_data(output, codepoints, lbs);
    print_binary_properties(output, codepoints);
    print_casing_data(output, codepoints);
    print_grapheme_cluster_data(output, codepoints);
    print_word_break_data(output, codepoints);
    print_footer(output);

    print_binary_property_public_interface(output);

    fclose(output);
}
