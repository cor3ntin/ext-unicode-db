#pragma once
#include <ranges>
#include <algorithm>

namespace cedilla::details {

template <std::size_t offsets_size,
          std::size_t short_offset_runs_size>
struct skiplist {
    uint8_t  offsets[offsets_size];
    alignas(8) uint32_t short_offset_runs[short_offset_runs_size];
    constexpr bool lookup(char32_t) const noexcept;
};

template <>
struct skiplist<0, 0> {
    constexpr bool lookup(char32_t) const noexcept {
        return false;
    }
};

template <std::size_t offsets_size,
         std::size_t short_offset_runs_size>
skiplist(const uint8_t (&)[offsets_size],
         const uint32_t (&)[short_offset_runs_size]) -> skiplist<offsets_size, short_offset_runs_size>;

skiplist() -> skiplist<0,0> ;


template <std::size_t offsets_size,
         std::size_t short_offset_runs_size>
constexpr
bool skiplist<offsets_size, short_offset_runs_size>::lookup(char32_t c) const noexcept {
    auto decode_len = [](uint32_t v) -> uint32_t { return v >> 21;};
    auto decode_prefix_sum = [](uint32_t v) ->uint32_t { return v  & ((1 << 21) - 1); };

    auto idx = std::ranges::lower_bound(std::views::all(short_offset_runs),
                                        c, {},[&](char32_t n) { return decode_prefix_sum(n); });
    if(idx == std::ranges::end(short_offset_runs))
        return false;
    idx = decode_prefix_sum(*idx) == c ? idx + 1 : idx;
    auto offset_idx = decode_len(*idx);

    auto len  = idx+1 != std::ranges::end(short_offset_runs) ?
                   decode_len(*(idx+1)) - offset_idx : offsets_size - offset_idx;
    auto prev = idx != std::ranges::begin(short_offset_runs) ?
                    decode_prefix_sum(*(idx-1)) : 0;

    uint32_t total = c - prev;
    uint32_t prefix_sum = 0;
    for(uint32_t i = 0; i < len -1 ; i++) {
        uint32_t offset = offsets[offset_idx];
        prefix_sum += offset;
        if(prefix_sum > total)
            break;
         offset_idx++;
    }
    return offset_idx % 2 == 1;
}

}

