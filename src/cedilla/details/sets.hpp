#pragma once
#include <ranges>
#include <algorithm>

namespace cedilla::details {

struct empty {
    static constexpr bool lookup(char32_t) noexcept {
        return false;
    }
};

template <char32_t... V>
struct small {
    static constexpr bool lookup(char32_t c) noexcept {
        return ((c == V) || ...) ;
    }
};

template <std::size_t N>
struct ranges {
    char32_t data[N][2];
    constexpr bool lookup(char32_t c) const noexcept {
        for(const auto & r : data) {
            if(c < r[0])
                return false;
            if(c < r[1])
                return true;
        }
        return false;
    }
};

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

template <std::size_t block_size, std::size_t words_count, std::size_t block_count, std::size_t index_size>
bitset(const uint64_t (&blocks)[words_count],
       const uint8_t (&block_index)[block_count][block_size],
       const uint8_t (&indexes)[index_size]) -> bitset<block_size, words_count, block_count, index_size>;


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

template <std::size_t offsets_size,
         std::size_t short_offset_runs_size>
struct skiplist {
    uint8_t  offsets[offsets_size];
    alignas(8) uint32_t short_offset_runs[short_offset_runs_size];
    constexpr bool lookup(char32_t) const noexcept;
};

template <std::size_t offsets_size,
         std::size_t short_offset_runs_size>
skiplist(const uint8_t (&)[offsets_size],
         const uint32_t (&)[short_offset_runs_size]) -> skiplist<offsets_size, short_offset_runs_size>;


template <std::size_t offsets_size,
         std::size_t short_offset_runs_size>
constexpr
    bool skiplist<offsets_size, short_offset_runs_size>::lookup(char32_t c) const noexcept {
    auto decode_len = [](uint32_t v) -> uint32_t { return v >> 21;};
    auto decode_prefix_sum = [](uint32_t v) ->uint32_t { return v  & ((1 << 21) - 1); };

    auto idx = std::ranges::upper_bound(std::views::all(short_offset_runs),
                                        c, {},[&](char32_t n) { return decode_prefix_sum(n); });
    if(idx == std::ranges::end(short_offset_runs))
        return false;

    auto offset_idx = decode_len(*idx);

    auto len  = idx+1 != std::ranges::end(short_offset_runs) ?
                   decode_len(*(idx+1)) - offset_idx : offsets_size - offset_idx;
    auto prev = idx != std::ranges::begin(short_offset_runs) ?
                    decode_prefix_sum(*(idx-1)) : 0;

    uint64_t total = c - prev;
    uint64_t prefix_sum = 0;
    for(uint32_t i = 0; i < len - 1; i++) {
        uint32_t offset = offsets[offset_idx];
        prefix_sum += offset;
        if(prefix_sum > total)
            break;
        offset_idx++;
    }
    return offset_idx % 2 == 1;
}

}
