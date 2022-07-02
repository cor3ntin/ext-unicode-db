#pragma once
#include <ranges>

namespace cedilla::details {

template <std::size_t block_size,
          std::size_t words_count,
          std::size_t block_count,
          std::size_t index_size>
struct bitset {
    uint64_t blocks[words_count];
    uint8_t  block_index[block_count][block_size];
    uint8_t  indexes[index_size];
    constexpr bool lookup(char32_t) const noexcept;
};

template <>
struct bitset<0, 0, 0, 0> {
    constexpr bool lookup(char32_t) const noexcept {
        return false;
    }
};

template <std::size_t block_size, std::size_t words_count, std::size_t block_count, std::size_t index_size>
bitset(const uint64_t (&blocks)[words_count],
       const uint8_t (&block_index)[block_count][block_size],
       const uint8_t (&indexes)[index_size]) -> bitset<block_size, words_count, block_count, index_size>;

bitset() -> bitset<0, 0, 0, 0> ;


template <std::size_t block_size,
         std::size_t words_count,
         std::size_t block_count,
         std::size_t index_size>
constexpr
bool bitset<block_size, words_count, block_count, index_size>::lookup(char32_t c) const noexcept {
    [[maybe_unused]] std::size_t bs = block_size;
    std::size_t bucket_idx  = c / 64;
    std::size_t block_idx   = bucket_idx / block_size;
    if(block_idx >= std::ranges::size(indexes))
        return false;
    const auto index_in_block_index = indexes[block_idx];
    const auto & block   = block_index[index_in_block_index];
    std::size_t word_idx = block[bucket_idx % block_size];
    uint64_t chunk = blocks[word_idx];
    return (chunk & (uint64_t(1) << uint64_t(c  % 64))) != 0;
}

}


