#pragma once
#include <bit>
#include <ranges>
#include <algorithm>
#include <span>
#include "cedilla/details/skiplist.hpp"

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

struct always_true_fn {
    bool operator()(std::uint32_t) const{
        return true;
    }
};

template <std::size_t N, typename ValueType,
         typename KeyFunction=std::identity,
         typename ValueFunction=always_true_fn>
struct perfect_hash_map {
    std::uint16_t salts[N];
    ValueType     values[N];
    constexpr inline std::uint32_t hash(std::uint32_t c, std::uint32_t salt, std::uint64_t n) const {
        std::uint32_t y = (c + salt) * 2654435769;
        y ^= c * 0x31415926;
        return (std::uint64_t(y) * std::uint64_t(n)) >> 32;
    }
    const std::invoke_result_t<ValueFunction, ValueType>
    lookup(char32_t c) const noexcept {
        const KeyFunction kf;
        const ValueFunction vf;
        auto salt = salts[hash(c, 0, N)];
        const auto & v = values[hash(c, salt, N)];
        if(c == kf(v)) {
            return vf(v);
        };
        return {};
    }
};

struct extract_higer_codepoint_fn {
    char32_t operator()(std::uint32_t c) const{
        return c >> 11;
    }
};

struct extract_lower11bits_fn {
    std::uint16_t operator()(std::uint32_t c) const{
         return c & 0x7FF;
    }
};

template <std::size_t N>
using lower11bits_codepoint_hash_map = perfect_hash_map<N, std::uint32_t,
                                                        extract_higer_codepoint_fn, extract_lower11bits_fn>;
}
