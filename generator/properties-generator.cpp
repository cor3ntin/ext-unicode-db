#include "load.hpp"
#include "fmt/format.h"
#include "fmt/color.h"
#include <filesystem>
#include <ranges>
#include "bitset_generator.hpp"
#include "skiplist_generator.hpp"
#include "utils.hpp"

using namespace cedilla::tools;

void die(std::string_view msg) {
    fmt::print(fg(fmt::color::red),
               "{}\n", msg);
    exit(1);
}

void usage() {
    fmt::print(fg(fmt::color::red),
               "Usage : unicode-properties-data-generator ucd.nounihan.flat.xml\n");
    exit(1);
}

void dump(const std::vector<codepoint> & codepoints) {

    for(const codepoint & c : codepoints | std::views::filter([] (const codepoint & c) {
       return !c.reserved;
    })) {
        fmt::print("{}:{}\n", c.value, c.name);
    }
}

void print_binary_properties(FILE* output, const std::vector<codepoint> & codepoints) {
    fmt::print(output, R"(
#pragma once
#include "cedilla/details/bitset.hpp"
#include "cedilla/details/skiplist.hpp"

namespace cedilla::details::generated {{
)");

    for(auto prop : {"alpha"}) {
        auto set = codepoints | std::views::filter ([prop](const codepoint & c) {
                   return c.has_binary_property(prop);
                   }) | std::views::transform(&codepoint::value);
        skiplist_data skiplist = create_skiplist(set);
        print_skiplist_data(output, fmt::format("property_{}", to_lower(prop)), skiplist);

        //bitset_data bitset = create_bitset(codepoints, ;
        //print_bitset_data(output, fmt::format("property_{}", to_lower(prop)), bitset);


    }

    fmt::print(output, "}}\n");
}

int main(int argc, const char** argv) {
    if(argc != 3 || !std::filesystem::exists(argv[1])) {
        usage();
    }
    auto ucd_path = argv[1];
    auto output_path = argv[2];

    auto codepoints = load_codepoints(ucd_path);
    fmt::print("loaded {}\n", ucd_path);
    fflush(stdout);
    auto output = fopen(output_path, "w");
    if(!output) {
        die("can't open output file");
    }
    print_binary_properties(output, codepoints);
    fclose(output);
}
