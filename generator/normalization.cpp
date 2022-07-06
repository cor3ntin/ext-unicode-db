#include "fmt/core.h"
#include "fmt/ranges.h"
#include "load.hpp"
#include "codepoint_data.hpp"
#include "utils.hpp"
#include "generators.hpp"
#include <map>
#include <ranges>
#include <set>
#include <bit>
#include <range/v3/view/unique.hpp>
#include <filesystem>
#include <cassert>

using namespace cedilla::tools;

namespace cedilla::tools {


void dump_canonical_mappings(FILE* output, const std::vector<codepoint> & codepoints)
{
    std::vector<std::uint16_t> small;
    std::vector<std::uint32_t> large;
    std::vector<std::tuple<char32_t, std::size_t, bool>> compositions;

    auto is_primary_composite = [&](const char32_t c) {
        auto it = std::ranges::find(codepoints, c, &codepoint::value);
        assert(it != std::ranges::end(codepoints));
        return !it->has_binary_property("comp_ex");
    };

    for(const auto & c : codepoints) {
        if(c.decomposition.empty() || !c.canonical_decomposition)
            continue;
        if(c.decomposition.size()>2)
            die(fmt::format("{:0x} ({}) decomposes canonically to more than 2 codepoints", (uint32_t)c.value, c.name));

        bool fits_in6_bytes =
            c.value <= 0xffff && std::ranges::all_of(c.decomposition, [](char32_t c) { return c <= 0xffff;});

        if(c.decomposition.size() >  1 && is_primary_composite(c.value)) {
            compositions.emplace_back(c.decomposition[1], fits_in6_bytes?small.size() : large.size(), fits_in6_bytes);
        }

        if(fits_in6_bytes) {
            small.push_back(c.value);
            small.push_back(c.decomposition[0]);
            small.push_back(c.decomposition.size() > 1 ? c.decomposition[1] : 0);
        }
        else {
            auto v = uint64_t(c.value) << 42;
            v |= uint64_t(c.decomposition[0]) << 21;
            v |= c.decomposition.size() > 1 ? uint64_t(c.decomposition[0]) : 0;
            large.push_back(v);
        }
    }

    auto K = [](auto && e) {return std::get<0>(std::forward<decltype(e)>(e));};
    std::ranges::sort(compositions, {}, K );

    uint32_t max_e = K(std::ranges::max(compositions, {} , K));
    if(std::bit_width(max_e) > 17)
        die("unexpectedly high number of bits in composition mapping");


    std::vector<uint32_t> prepared_compositions = compositions | std::ranges::views::transform([](const auto & c) {
                                   auto s = std::get<1>(c);
                                   if(std::get<2>(c))
                                       s /= 3;
                                   return (uint32_t(std::get<0>(c)) << 17) | uint32_t(s);
                               }) | ranges::to<std::vector>;

    constexpr auto tpl = R"(
namespace cedilla::generated {{
    std::uint16_t canonical_decomposition_mapping_small[{}] = {{ {:#04X} }};
    std::uint32_t canonical_decomposition_mapping_large[{}] = {{ {:#06x} }};
}})";
    // std::uint32_t canonical_composition_mapping[{}] = {{ {:#04x} }};
    fmt::print(output, tpl,
               small.size(), fmt::join(small, ", "),
               large.size(), fmt::join(large, ", "));


    ///fmt::print("\n----- {}: {} \n", compositions.size(), std::bit_width(max_e));

}

void  dump_quick_checks(FILE* output, const std::vector<codepoint> & codepoints) {
    fmt::print(output, R"(
namespace cedilla::details::generated {{
)");

    for(auto prop : {"nfd_qc"}) {
        auto set = codepoints | std::views::filter ([prop](const codepoint & c) {
                       return c.has_binary_property(prop);
                   }) | std::views::transform(&codepoint::value) | ranges::to<std::vector>();

        print_binary_data_best(output, set, fmt::format("property_{}", to_lower(prop)));
    }

    fmt::print(output, "}}\n");

}


void dump_combining_classes(FILE* output, const std::vector<codepoint> & codepoints)
{
    std::vector<std::pair<uint32_t, int>> combining_ranges;
    std::map<std::uint8_t, uint32_t> classes;
    for(const codepoint & c : codepoints) {
        if(!combining_ranges.empty() && c.ccc == combining_ranges.back().second)
            continue;
        combining_ranges.push_back({c.value, c.ccc});
    }
    combining_ranges.push_back({combining_ranges.back().first, 0});

    std::vector<uint32_t> ranges = combining_ranges | std::ranges::views::transform([](const auto & c) {
                                                      return (c.first << 21) | c.second;
                                                  }) | ranges::to<std::vector>;


    constexpr auto tpl = R"(
namespace cedilla::generated {{
    std::uint32_t combining_classes[{}] = {{ {:#04x} }};

}})";
    fmt::print(output, tpl, ranges.size(), fmt::join(ranges, ", "));

}



void dump_normalization_statistics(const std::vector<codepoint> & codepoints) {
    std::map<int, int> canonical_count;
    std::map<int, int> legacy_count;
    std::size_t count =  0, can_count = 0;
    char32_t max_can = 0, max = 0;
    char32_t max_can_mapping = 0;
    int has_cp_with_mapping_greater_than_16_bits = 0;
    int count_16 = 0;
    std::map<std::uint8_t, uint32_t> classes;
    for(const auto & c : codepoints) {
         classes[c.ccc] ++;
        if(c.decomposition.empty())
            continue;
        if(c.ccc != 0 && c.has_binary_property("nfd_qc")) {
            fmt::print("{:0x} wtf", (uint32_t)c.value);
        }

        max = std::max(max, c.value);
        count++;
        if(c.canonical_decomposition) {
            if(c.value <= 0xffff)
                count_16++;
            canonical_count[c.decomposition.size()]++;
            max_can = std::max(max_can, c.value);
            can_count++;
            for (auto m : c.decomposition) {
                max_can_mapping = std::max(max_can_mapping, m);
                if(m > 0xffff && c.value < 0xffff) {
                    fmt::print("{} :  {:0x} {:0x}\n", c.name, (std::uint32_t)c.value, (std::uint32_t)m);
                    has_cp_with_mapping_greater_than_16_bits++;
                }
            }
        }
        else {
            legacy_count[c.decomposition.size()]++;
        }
    }

    fmt::print(R"(
total: {}
canonical: {}
legacy: {}
max char: {}
max char canonical: {} ({})
canonical rules size: {}
legacy rules size: {}
largest char in composition mapping: {}
16 bits: {} / more : {}
has_cp_with_mapping_greater_than_16_bits: {}
classes: {},  {}
)", count, can_count, count - can_count, (std::uint32_t) max,  (std::uint32_t) max_can, std::bit_width((std::uint32_t)max_can),
    canonical_count, legacy_count,
    std::bit_width((std::uint32_t)max_can_mapping), count_16, can_count - count_16, has_cp_with_mapping_greater_than_16_bits,
    classes.size(), classes);
}

void print_header(FILE* out) {
    fmt::print(out, R"(
#pragma once
#include "cedilla/details/sets.hpp"

)");
}

void usage() {
    fmt::print(fg(fmt::color::red),
               "Usage : unicode-normalization-data-generator ucd.nounihan.flat.xml output.hpp\n");
    exit(1);
}



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
    print_header(output);
    dump_normalization_statistics(codepoints);
    dump_quick_checks(output, codepoints);
    dump_combining_classes(output, codepoints);
    dump_canonical_mappings(output, codepoints);
    fclose(output);
    return 0;
}
