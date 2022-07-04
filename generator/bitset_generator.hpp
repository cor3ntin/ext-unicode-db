#pragma once
#include <cassert>
#include <cstdint>
#include <execution>
#include <map>
#include <vector>
#include <ranges>
#include <algorithm>
#include <limits>
#include <set>
#include "fmt/ranges.h"
#include <range/v3/view/chunk.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/range/conversion.hpp>

#include "codepoint_data.hpp"
#include "utils.hpp"

namespace cedilla::tools {

constexpr std::size_t BITSET_BUCKET_SIZE = 64;

struct block_data {
    std::vector<uint8_t> indexes;
    std::vector<std::vector<uint8_t>> unique;
    std::size_t size = 0 ;
    std::size_t block_len = 0;
};

struct bitset_data {
    std::vector<std::uint64_t> unique_chunks;
    std::vector<uint8_t>       all_indexes;
    block_data blocks;


    std::size_t size() const {
        return unique_chunks.size() * 8 + blocks.size;
    }
};

inline void print_bitset_data(FILE* output, std::string_view var_name, const bitset_data & data) {

    auto all_zero = std::ranges::find_if(data.blocks.unique, [](const auto & rng) {
        return std::ranges::all_of(rng, [](std::uint8_t n) {return n == 0 ;});
    });

    auto indexes = data.blocks.indexes
                   | std::views::reverse
                   | std::views::drop_while([&all_zero, &data](std::uint8_t n) {
                         if(all_zero == std::ranges::end(data.blocks.unique))
                             return false;
                         return std::distance(ranges::begin(data.blocks.unique), all_zero) == n;
                  })
                   | std::views::reverse;

    auto formatted_blocks = data.blocks.unique | std::views::transform([] (const auto & rng) {
                                return fmt::format("{{ {} }}", fmt::join(rng, ", "));
                            });

    if(ranges::empty(indexes)) {
        fmt::print(output, "inline constexpr bitset {};", var_name);
        return;
    }

    fmt::print(output, R"(
       inline constexpr bitset {} {{
           .blocks = {{ {:#016X} }},
           .block_index = {{ {} }},
           .indexes = {{ {} }}
       }};
)",
               var_name,
               fmt::join(data.unique_chunks, ", "),
               fmt::join(formatted_blocks, ", "),
               fmt::join(indexes, ", "));
}

inline block_data compute_block_data(const std::ranges::input_range auto & r, std::size_t len) {
    block_data data;
    data.block_len = len;
    auto it = r.begin();
    const auto end = std::ranges::end(r);
    std::vector<uint8_t> chunk(len, 0);
    do {
        std::size_t s =  std::min(len, std::size_t(std::ranges::distance(it, end)));
        auto chunk_end = it + s;
        if(chunk_end == end)
            std::ranges::fill(chunk, 0);
        std::ranges::copy(ranges::subrange(it, chunk_end), chunk.begin());
        auto probe = std::ranges::find(data.unique, chunk);
        if(probe == std::ranges::end(data.unique)) {
            probe = data.unique.insert(data.unique.end(), chunk);
        };
        data.indexes.push_back(std::distance(data.unique.begin(), probe));
        it = chunk_end;
    } while(it != end);
    data.size = data.indexes.size() + data.unique.size() * len;
    return data;
}

inline block_data ideal_block(const std::ranges::input_range auto & r) {
    auto v = std::views::iota(1uz, std::size_t(std::min(r.size(), 64uz)))
             | std::views::transform([&r](auto len) {
                 return compute_block_data(r, len);
    });
    auto d = *std::min_element(std::execution::par, v.begin(), v.end(),
                               [](const block_data & a, const block_data & b) {
        return a.size < b.size;
    });
    return d;
}

template <range_of<char32_t> R>
std::optional<bitset_data> create_bitset(const R & input) {
    constexpr std::size_t max = 0x10FFFF;
    std::vector<std::uint64_t> bits(max/BITSET_BUCKET_SIZE + 10, 0);
    std::for_each(std::execution::par_unseq, input.begin(), input.end(), [&bits](const char32_t & c) {
        std::size_t bucket = c /  BITSET_BUCKET_SIZE;
        uint64_t bit = c %  BITSET_BUCKET_SIZE;
        bits[bucket] |= (uint64_t(1) << bit);
    });
    std::set<uint64_t> unique_words {bits.begin(), bits.end()};
    unique_words.insert(0);
    if(unique_words.size() >= std::numeric_limits<uint8_t>::max())
        return {};
    bitset_data data;
    data.unique_chunks = {unique_words.begin(), unique_words.end()};
    data.all_indexes = bits | std::views::transform ( [&](uint64_t word) {
        return uint8_t(std::ranges::distance(data.unique_chunks.begin(),
                                             std::ranges::find(data.unique_chunks, word)));
    }) | ranges::to<std::vector>();
    data.blocks = ideal_block(data.all_indexes);
    return data;
}

}
