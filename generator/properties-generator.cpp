#include "load.hpp"
#include "fmt/format.h"
#include "fmt/color.h"
#include <filesystem>
#include <ranges>
#include "generators.hpp"
#include "utils.hpp"

using namespace cedilla::tools;

namespace cedilla::tools {

void print_categories_data(FILE* out, const std::vector<codepoint> & all, const categories & cats);

}
void die(std::string_view msg) {
    fmt::print(fg(fmt::color::red),
               "{}\n", msg);
    exit(1);
}

void usage() {
    fmt::print(fg(fmt::color::red),
               "Usage : unicode-properties-data-generator ucd.nounihan.flat.xml PropertyValueAliases.txt output.hpp\n");
    exit(1);
}

void dump(const std::vector<codepoint> & codepoints) {

    for(const codepoint & c : codepoints | std::views::filter([] (const codepoint & c) {
       return !c.reserved;
    })) {
        fmt::print("{}:{}\n", (uint32_t)c.value, c.name);
    }
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


void print_binary_properties(FILE* output, const std::vector<codepoint> & codepoints) {
    fmt::print(output, R"(
namespace cedilla::details::generated {{
)");

    for(auto prop : useful_properties) {
        auto set = codepoints | std::views::filter ([prop](const codepoint & c) {
                   return c.has_binary_property(prop);
                   }) | std::views::transform(&codepoint::value) | ranges::to<std::vector>();
        print_binary_data_best(output, set, fmt::format("property_{}", to_lower(prop)));
    }

    fmt::print(output, "}}\n");
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

void print_header(FILE* out) {
    fmt::print(out, R"(
#pragma once
#include "cedilla/details/sets.hpp"
)");
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
    categories cats = load_categories(property_aliases_path);
    fmt::print("loaded categories from {}\n", property_aliases_path);

    fflush(stdout);
    auto output = fopen(output_path, "w");
    if(!output) {
        die("can't open output file");
    }
    print_header(output);
    print_categories_data(output, codepoints, cats);
    print_binary_properties(output, codepoints);

    fclose(output);
}
