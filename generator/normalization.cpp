﻿#include "fmt/core.h"
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
    struct small_decomposition {
        uint16_t key;
        uint16_t r1;
        uint16_t r2 = 0;
    };

    struct composition {
        uint32_t leading;
        std::size_t index;
        bool in4bytes = false;
    };

    std::vector<small_decomposition> small;
    std::vector<std::uint64_t> large;


    std::vector<composition> compositions;

    auto is_primary_composite = [&](const char32_t c) {
        auto it = std::ranges::find(codepoints, c, &codepoint::value);
        assert(it != std::ranges::end(codepoints));
        return !it->has_binary_property("comp_ex");
    };

    for(const auto & c : codepoints) {
        if(c.is_hangul())
            continue;
        if(c.decomposition.empty() || !c.canonical_decomposition)
            continue;
        if(c.decomposition.size()>2)
            die(fmt::format("{:0x} ({}) decomposes canonically to more than 2 codepoints", (uint32_t)c.value, c.name));

        bool fits_in4_bytes =
            c.value <= 0xffff && std::ranges::all_of(c.decomposition, [](char32_t c) { return c <= 0xffff;});

        if(c.decomposition.size() >  1 && is_primary_composite(c.value)) {
            compositions.emplace_back(c.decomposition[0], fits_in4_bytes?
                                                          small.size() : large.size(), fits_in4_bytes);
        }

        if(fits_in4_bytes) {
            small.emplace_back(c.value, c.decomposition[0], c.decomposition.size() > 1 ? c.decomposition[1] : 0);
        }
        else {
            auto v = uint64_t(c.value) << 42;
            v |= uint64_t(c.decomposition[0]) << 21;
            v |= c.decomposition.size() > 1 ? uint64_t(c.decomposition[1]) : 0;
            large.push_back(v);
        }
    }

    uint32_t max_e = std::ranges::max(compositions, {} , &composition::leading).leading;
    if(std::bit_width(max_e) > 17)
        die("unexpectedly high number of bits in composition mapping");


    std::ranges::sort(compositions, {}, [](const composition & c) {
          return c.leading << 15;
    });

    std::vector<std::string> prepared_compositions = compositions | std::ranges::views::transform([](const composition & c) {
                                   auto s = c.index;
                                   uint32_t packed = (uint32_t(c.leading) << 15) | uint32_t(s);
                                   return fmt::format("{:#04x} /*{:#04x} | {} */",packed , uint32_t(c.leading), uint32_t(s));



                               }) | ranges::to<std::vector>;

    constexpr auto tpl = R"(
    struct small_decomposition {{
        uint16_t key;
        uint16_t r1;
        uint16_t r2 = 0;
    }};

    // clang-format off
    constexpr inline small_decomposition canonical_decomposition_mapping_small[{}] = {{ {} }};
    // clang-format on
    constexpr inline std::uint64_t canonical_decomposition_mapping_large[{}] = {{ {:#06x} }};
    constexpr inline std::uint32_t canonical_composition_mapping[{}] = {{
        {}
    }};
)";

    auto transform_small_decomposition = [](const small_decomposition & decomp) {
        return fmt::format("{{ {:#04x}, {:#04x}, {:#04x}, }}", decomp.key, decomp.r1, decomp.r2);
    };

    //
    fmt::print(output, tpl,
               //small_keys.size(), fmt::join(small_keys, ", "),
               small.size(), fmt::join(small | std::views::transform(transform_small_decomposition), ", \n"),
               large.size(), fmt::join(large, ", "),
               prepared_compositions.size(), fmt::join(prepared_compositions, ",\n"));


    ///fmt::print("\n----- {}: {} \n", compositions.size(), std::bit_width(max_e));

}

void  dump_quick_checks(FILE* output, const std::vector<codepoint> & codepoints) {
    auto nfd_qc_no = codepoints | std::views::filter ([](const codepoint & c) {
                   return !c.NFD_QC;
               }) | std::views::transform(&codepoint::value) | ranges::to<std::vector>();
    auto nfc_qc_no = codepoints | std::views::filter ([](const codepoint & c) {
                         return !c.NFC_QC;
                     }) | std::views::transform(&codepoint::value) | ranges::to<std::vector>();


    print_binary_data_best(output, nfd_qc_no, "nfd_qc_no");
    print_binary_data_best(output, nfc_qc_no, "nfc_qc_no");
}


void dump_combining_classes(FILE* output, const std::vector<codepoint> & codepoints) {
    auto interesting = codepoints
                       | std::views::filter([](const codepoint & c) { return c.ccc != 0 ;});

    std::vector<char32_t> keys = interesting
        | std::views::transform(&codepoint::value)
        | ranges::to<std::vector>;

    hash_data data = create_perfect_hash(keys);

    std::map<char32_t, uint8_t> ccc;
    std::ranges::transform(
        interesting,
        std::inserter(ccc, ccc.begin()), [](const codepoint & c) { return std::pair{c.value, c.ccc};});

    std::vector<uint32_t> values = data.keys | std::views::transform([&](char32_t c) {
                                       return (uint32_t(c) << 11) | ccc[c];
    }) | ranges::to<std::vector>;
    print_hash_data(output, "lower11bits_codepoint_hash_map", "ccc_data", data.salts, values);
}


void dump_hangul_syllables(FILE* output, const std::vector<codepoint> & codepoints) {

    struct syllable {
        uint32_t cp   : 21;
        uint32_t kind : 11;
    };

    std::vector<syllable> syllables;
    uint8_t type = uint8_t(hangul_syllable_kind::invalid);
    auto hangul_cps = codepoints
                       | std::views::filter([](const codepoint & c) { return c.is_hangul() ;})
                  | ranges::to<std::vector>;

    for(const codepoint & cp : hangul_cps) {
        if(cp.hangul_syllable_kind == type)
            continue;
        syllables.emplace_back(cp.value, cp.hangul_syllable_kind);
        type = cp.hangul_syllable_kind;
    }
    syllables.emplace_back(hangul_cps.back().value+1, uint8_t(hangul_syllable_kind::invalid));


    constexpr auto tpl = R"(
    struct hangul_syllable {{
        uint32_t cp: 21;
        uint32_t kind: 11;
    }};
    constexpr inline hangul_syllable hangul_syllables[{}] = {{
        {}
    }};
)";

    auto transform_hangul_syllables = [](const syllable & s) {
        return fmt::format("{{ {:#04x}, {} }}", s.cp, s.kind);
    };

    fmt::print(output, tpl,
               syllables.size(), fmt::join(syllables | std::views::transform(transform_hangul_syllables), ", \n"));
}



size_t find_recursion_size(const std::vector<codepoint> & codepoints, char32_t needle) {
    auto it = std::ranges::lower_bound(codepoints, needle, {}, &codepoint::value);
    if(it == codepoints.end() || it->value != needle || !it->canonical_decomposition)
        return 0;
    size_t size = 0;
    for(const auto & c : it->decomposition) {
        size += find_recursion_size(codepoints, c);
    }
    if(size == 0)
        return 1;
    return size;
}

size_t find_recursion_size(const std::vector<codepoint> & codepoints) {
    size_t size = 0;
    for(const auto & c : codepoints) {
        size = std::max(find_recursion_size(codepoints, c.value), size);
    }
    return size;
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


void dump_recursive(const std::vector<codepoint> & codepoints) {
    std::map<char32_t, std::vector<char32_t>> with_mapping;
    std::set<uint32_t> recursive;
    for(auto cp : codepoints) {
        if(!cp.decomposition.empty() && cp.canonical_decomposition) {
            with_mapping[cp.value]  = cp.decomposition;
        }
    }
    for (const auto & [c, mapping] : with_mapping) {
        for(auto m : mapping) {
            if(with_mapping.contains(m))
                recursive.insert(c);

        }
    }
    fmt::print("recursive: {}\n", recursive.size());
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

    fmt::print(output, "namespace cedilla::details::generated {{\n");

    dump_normalization_statistics(codepoints);

    fmt::print("MAX RECURSION : {} !!!!!\n", find_recursion_size(codepoints));

    dump_recursive(codepoints);
    dump_quick_checks(output, codepoints);
    dump_combining_classes(output, codepoints);
    dump_canonical_mappings(output, codepoints);
    dump_hangul_syllables(output, codepoints);

    fmt::print(output, "}}\n");

    fclose(output);
    return 0;
}
