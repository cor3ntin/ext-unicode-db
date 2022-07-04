#pragma  once
#include <tuple>
#include <fmt/format.h>

#include "utils.hpp"

namespace cedilla::tools {

inline std::vector<std::tuple<char32_t, char32_t>> create_ranges(range_of<char32_t> auto && codepoints) {
    std::optional<char32_t> start;
    std::optional<char32_t> end;
    std::vector<std::tuple<char32_t, char32_t>> ranges;
    for(char32_t c : codepoints) {
        if(!start) {
            end = start = c;
            continue;
        }
        if(c != *end + 1) {
            ranges.emplace_back(*start, *end + 1);
            end = start = c;
            continue;
        }
        end = *end + 1;
    }
    if(start) {
        ranges.emplace_back(*start, *end + 1);
    }
    return ranges;
}

struct skiplist_data {
    std::vector<std::uint8_t> coded_offsets;
    std::vector<std::uint32_t> short_offset_runs;

    std::size_t size() const {
        return coded_offsets.size() + short_offset_runs.size() * 4;
    }
};

constexpr inline auto flatten = std::views::transform([](auto && tuple)  {
                    constexpr auto S = std::tuple_size_v<std::remove_cvref_t<decltype(tuple)>>;
                    return [&]<std::size_t...I>(std::index_sequence<I...>) {
                        return std::array{std::get<I>(tuple)...};
                    }(std::make_index_sequence<S>{});
}) | std::ranges::views::join;


inline skiplist_data create_skiplist(range_of<char32_t> auto && codepoints) {
    auto ranges = create_ranges(codepoints);
    std::vector<char32_t> offsets;
    {
        char32_t offset = 0;
        for(char32_t point : ranges | flatten) {
            auto delta = point - offset;
            offsets.push_back(delta);
            offset = point;
        }
    }
    offsets.push_back(std::numeric_limits<uint8_t>::max() + 1);
    std::uint32_t prefix_sum = 0;

    skiplist_data d;
    std::uint32_t start = 0;
    for(auto offset : offsets ) {
        prefix_sum  += offset;
        if(offset <= std::numeric_limits<uint8_t>::max()) {
            d.coded_offsets.push_back(offset);
        }
        else {
            std::uint32_t short_offset = (start << std::uint32_t(21)) | prefix_sum;
            d.short_offset_runs.push_back(short_offset);
            d.coded_offsets.push_back(0);
            start = d.coded_offsets.size();
        }
    }
    return d;
}

inline void print_skiplist_data(FILE* output, std::string_view var_name, const skiplist_data & data) {
    constexpr const char* tpl = R"(
       inline constexpr skiplist<{}, {}> {} {{
           .offsets = {{ {} }},
           .short_offset_runs = {{ {} }}
       }};
    )";
    fmt::print(output, tpl,
               data.coded_offsets.size(),
               data.short_offset_runs.size(),
               var_name,
               fmt::join(data.coded_offsets, ", "),
               fmt::join(data.short_offset_runs, ", ")
    );
}

}
