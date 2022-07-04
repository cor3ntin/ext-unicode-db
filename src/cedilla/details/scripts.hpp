#pragma once
#include <cstdint>
#include <algorithm>
#include <ranges>

namespace cedilla::details {

template<std::size_t Size>
struct script_data {
    std::uint32_t data[Size];
    constexpr uint16_t lookup(char32_t cp, uint16_t default_value) const noexcept{
        auto it = std::ranges::upper_bound(std::views::all(data), cp, [](char32_t needle, uint32_t v) {
            char32_t c = (v >> 11);
            return needle < c;
        });
        if(it == std::ranges::end(data))
            return default_value;
        it--;
        return (*it)&(0x7FF);
    }
};

namespace generated {

struct undefined { undefined(); };

template<std::size_t index>
undefined script_properties;
}

}
