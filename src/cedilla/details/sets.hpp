#pragma once
#include <bit>
#include <ranges>
#include <algorithm>
#include <span>
#include <immintrin.h>

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
    alignas(32) uint8_t  offsets[offsets_size];
    alignas(32) uint32_t short_offset_runs[short_offset_runs_size];
    constexpr bool lookup(char32_t) const noexcept;
};

template <std::size_t offsets_size,
         std::size_t short_offset_runs_size>
skiplist(const uint8_t (&)[offsets_size],
         const uint32_t (&)[short_offset_runs_size]) -> skiplist<offsets_size, short_offset_runs_size>;


constexpr bool skiplist_search(char32_t c, std::span<const uint32_t> short_offset_runs, std::span<const uint8_t> offsets) noexcept {
    auto decode_len = [](uint32_t v) -> uint32_t { return v >> 21;};
    auto decode_prefix_sum = [](uint32_t v) ->uint32_t { return v  & ((1 << 21) - 1); };

    auto idx = std::ranges::upper_bound(short_offset_runs.begin() + 1, short_offset_runs.end() - 1,
                                        c, {},[&](char32_t n) { return decode_prefix_sum(n); });
    if(idx == std::ranges::end(short_offset_runs) - 1)
        return false;

   [[maybe_unused]] auto n = std::distance(std::ranges::begin(short_offset_runs), idx);

    auto offset_idx = decode_len(*idx);

   auto len  = decode_len(*(idx+1)) - offset_idx;
   auto prev = decode_prefix_sum(*(idx-1));

    //auto len  = idx+1 != std::ranges::end(short_offset_runs) ?
    //               decode_len(*(idx+1)) - offset_idx : short_offset_runs.size() - offset_idx;
    //auto prev = idx != std::ranges::begin(short_offset_runs) ?
    //                decode_prefix_sum(*(idx-1)) : 0;

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

bool skiplist_search_simd(char32_t c, std::span<const uint32_t> short_offset_runs,
                                      std::span<const uint8_t> offsets) noexcept {


    constexpr uint32_t M = ((1 << 21) - 1);

    auto decode_len = [](uint32_t v) -> uint32_t { return v >> 21;};
    //auto decode_prefix_sum = [](uint32_t v) ->uint32_t { return v  & M; };

    __m256i mask  = _mm256_set_epi32(M, M, M, M, M, M, M, M);
    __m256i value = _mm256_set_epi32(c, c, c, c, c, c, c, c);

    std::size_t idx;
    for(std::size_t i = 0; i < short_offset_runs.size() - 8; i+=8) {
        const __m256i line    = _mm256_load_si256((const __m256i*) &short_offset_runs[i]);
        const __m256i masked  = _mm256_and_si256(mask, line);
        const __m256i cmp     = _mm256_cmpgt_epi32(masked, value);
        const __m256 m256     = _mm256_castsi256_ps(cmp);
        unsigned int m        = _mm256_movemask_ps(m256);
        auto n = std::countr_zero(m);
        if(n != 32) {
            idx = i + n;
            goto found;
        }
    }
    return false;
found:
    auto offset_idx = decode_len(short_offset_runs[idx]) ;
    auto len  = decode_len(short_offset_runs[idx+1]) - offset_idx;
    auto prev = short_offset_runs[idx-1] & M;

    uint64_t total = c - prev;
    uint64_t prefix_sum = 0;
    for(uint32_t i = 0; i < len - 1; i++, offset_idx++) {
        prefix_sum += offsets[offset_idx];
        if(prefix_sum > total)
            break;
    }
    return offset_idx % 2 == 1;
}


bool skiplist_search_simd_256_2(char32_t c, std::span<const uint32_t> short_offset_runs,
                          std::span<const uint8_t> offsets) noexcept {


    constexpr uint32_t M = ((1 << 21) - 1);

    auto decode_len = [](uint32_t v) -> uint32_t { return v >> 21;};
    //auto decode_prefix_sum = [](uint32_t v) ->uint32_t { return v  & M; };

    __m256i mask  = _mm256_set_epi32(M, M, M, M, M, M, M, M);
    __m256i value = _mm256_set_epi32(c, c, c, c, c, c, c, c);

    std::size_t idx;
    for(std::size_t i = 0; i < short_offset_runs.size() - 8; i+=8) {
        const __m256i line    = _mm256_load_si256((const __m256i*) &short_offset_runs[i]);
        const __m256i masked  = _mm256_and_si256(mask, line);
        const __m256i cmp     = _mm256_cmpgt_epi32(masked, value);
        const __m256 m256     = _mm256_castsi256_ps(cmp);
        unsigned int m        = _mm256_movemask_ps(m256);
        auto n = std::countr_zero(m);
        if(n != 32) {
            idx = i + n;
            goto found;
        }
    }
    return false;
found:
    auto prev = short_offset_runs[idx-1] & M;
    auto offset_idx = decode_len(short_offset_runs[idx]) ;
    auto len  = decode_len(short_offset_runs[idx+1]) - offset_idx;

    uint32_t total = c - prev;
    int i = 0;
    const __m256i totalx = _mm256_set_epi32(total, total, total, total, total, total, total, total);
    constexpr int N = 16;
    std::size_t prefix_sum = 0;
    alignas(32) std::array<uint16_t, N> a;
    for(; i < int(len) - (N + 1); i+=N, offset_idx += N) {

        a[0] = prefix_sum + offsets[offset_idx];

        [&]<std::size_t...I>(std::index_sequence<I...>) {
            ((a[I+1] = a[I] + offsets[offset_idx + I + 1]), ...);
        }(std::make_index_sequence<a.size() -1>{});

        const __m256i prefixes = _mm256_load_si256((const __m256i*) a.data());
        const __m256i cmp      = _mm256_cmpgt_epi32(prefixes, totalx);
        unsigned int m         = _mm256_movemask_ps(_mm256_castsi256_ps(cmp));
        auto n = std::countr_zero(m);
        if(n != 32) {
            offset_idx += n;
            return offset_idx % 2 == 1;
        }
        prefix_sum = a[a.size() -1];
    }
    for(; i < int(len) -1; i++) {
        uint32_t offset = offsets[offset_idx];
        prefix_sum += offset;
        if(prefix_sum > total)
            break;
        offset_idx++;

    }
    return offset_idx % 2 == 1;
}


bool skiplist_search_simd_128(char32_t c, std::span<const uint32_t> short_offset_runs,
                          std::span<const uint8_t> offsets) noexcept {


    constexpr uint32_t M = ((1 << 21) - 1);

    auto decode_len = [](uint32_t v) -> uint32_t { return v >> 21;};

    __m128i mask  = _mm_set_epi32(M, M, M, M);
    __m128i value = _mm_set_epi32(c, c, c, c);

    std::size_t idx;
    for(std::size_t i = 0; i < short_offset_runs.size() - 4; i+=4) {
        const __m128i line    = _mm_load_si128((const __m128i*) &short_offset_runs[i]);
        const __m128i masked  = _mm_and_si128(mask, line);
        const __m128i cmp     = _mm_cmpgt_epi32(masked, value);
        unsigned int m        = _mm_movemask_ps(_mm_castsi128_ps(cmp));
        auto n = std::countr_zero(m);
        if(n != 32) {
            idx = i + n;
            goto found;
        }
    }
    return false;
found:
    auto offset_idx = decode_len(short_offset_runs[idx]) ;
    auto len  = decode_len(short_offset_runs[idx+1]) - offset_idx;
    auto prev = short_offset_runs[idx-1] & M;

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

bool skiplist_search_simd_128_2(char32_t c, std::span<const uint32_t> short_offset_runs,
                                std::span<const uint8_t> offsets) noexcept {


    constexpr uint32_t M = ((1 << 21) - 1);

    auto decode_len = [](uint32_t v) -> uint32_t { return v >> 21;};

    __m128i mask  = _mm_set_epi32(M, M, M, M);
    __m128i value = _mm_set_epi32(c, c, c, c);

    std::size_t idx;
    for(int i = 0; i < int(short_offset_runs.size()) - 4; i+=4) {
        const __m128i line    = _mm_load_si128((const __m128i*) &short_offset_runs[i]);
        const __m128i masked  = _mm_and_si128(mask, line);
        const __m128i cmp     = _mm_cmpgt_epi32(masked, value);
        unsigned int m        = _mm_movemask_ps(_mm_castsi128_ps(cmp));
        auto n = std::countr_zero(m);
        if(n != 32) {
            idx = i + n;
            goto found;
        }
    }
    return false;
found:
    auto offset_idx = decode_len(short_offset_runs[idx]) ;
    auto len  = decode_len(short_offset_runs[idx+1]) - offset_idx;
    auto prev = short_offset_runs[idx-1] & M;

    uint32_t total = c - prev;
    int i = 0;
    const __m128i totalx = _mm_set_epi32(total, total, total, total);
    std::size_t prefix_sum = 0;
    alignas(32) std::array<uint16_t, 8> a;
    for(; i < int(len) - 9; i+=8, offset_idx += 8) {

        a[0] = prefix_sum + offsets[offset_idx];
        [&]<std::size_t...I>(std::index_sequence<I...>) {
            ((a[I+1] = a[I] + offsets[offset_idx + I + 1]), ...);
        }(std::make_index_sequence<a.size() -1>{});

        const __m128i prefixes = _mm_load_si128 ((const __m128i*) a.data());
        const __m128i cmp      = _mm_cmpgt_epi16(prefixes, totalx);
        unsigned int m         = _mm_movemask_epi8(cmp);
        auto n = std::countl_zero(m);
        if(n != 32) {
            break;

            offset_idx += n;
            return offset_idx % 2 == 1;
        }
        prefix_sum = a[a.size() -1];
    }
    for(; i < int(len) -1; i++) {
        uint32_t offset = offsets[offset_idx];
        prefix_sum += offset;
        if(prefix_sum > total)
            break;
        offset_idx++;

    }
    return offset_idx % 2 == 1;
}


template <std::size_t offsets_size, std::size_t short_offset_runs_size>
inline constexpr
    bool skiplist<offsets_size, short_offset_runs_size>::lookup(char32_t c) const noexcept {
  //  if (std::is_constant_evaluated()) {
    //        return skiplist_search(c, short_offset_runs, offsets);
  //  }
  //  else {
  //return false;
  return skiplist_search_simd_128(c, short_offset_runs, offsets);
  //  }
}
}
