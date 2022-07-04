#include "bitset_generator.hpp"
#include "skiplist_generator.hpp"

namespace cedilla::tools {


inline void print_empty(FILE* output, std::string_view var_name) {

    fmt::print(output, R"(
       inline constexpr empty {};
)", var_name);

}

inline void print_ranges_data(FILE* output, std::string_view var_name,
                              const std::vector<std::tuple<char32_t, char32_t>> & data) {

    auto formatted_ranges = data | std::views::transform([](const std::tuple<char32_t, char32_t> & t) {
                                return fmt::format("{{ {:#04x}, {:#04x} }}", (uint32_t)std::get<0>(t), (uint32_t)std::get<1>(t));
    });

    fmt::print(output, R"(
       inline constexpr ranges<{}> {} {{{{
           {}
       }}}};
)", data.size(), var_name, fmt::join(formatted_ranges, ", "));

}


inline void print_small(FILE* output, std::string_view var_name, const range_of<char32_t> auto & data) {

    fmt::print(output, R"(
       inline constexpr small<{:#04x}> {};
)", fmt::join(data | std::views::transform([](char32_t c) {return uint32_t(c);}), ", "), var_name);

}


inline void print_binary_data_best(FILE* output, const range_of<char32_t> auto & data, std::string_view var_name) {
    if(data.empty()) {
        print_empty(output, var_name);
        return;
    }

    if(data.size() < 4) {
        print_small(output, var_name, data);
        return;
    }

    auto ranges = create_ranges<char32_t>(data);
    if(ranges.size() <= 2) {
        print_ranges_data(output, var_name, ranges);
        return;
    }

    skiplist_data skiplist = create_skiplist(data);
    print_skiplist_data(output, var_name, skiplist);
}

}
