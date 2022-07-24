#include "fmt/core.h"
#include "load.hpp"
#include "codepoint_data.hpp"
#include "utils.hpp"
#include "generators.hpp"
#include <map>
#include <ranges>
#include <set>
#include <range/v3/view/unique.hpp>

namespace cedilla::tools {

using indexed_scripts = std::map<std::string, std::pair<uint16_t, std::string>>;

std::map<std::string, std::pair<uint16_t, std::string>> sorted_scripts(const labels& l) {
    uint16_t idx = 1;
    std::vector<std::pair<std::string, std::string>> scripts{l.scripts.begin(), l.scripts.end()};
    std::ranges::sort(scripts, {}, &std::pair<std::string, std::string>::first);
    std::map<std::string, std::pair<uint16_t, std::string>> res;
    for(const auto & [k, v] : scripts) {
        auto i = k == "zzzz"? 0 : idx++;
        res[k] = {i, {}};
        if(k != v)
            res[v] = {i, k};
    }
    return res;
}


void print_scripts_enum(FILE* out, const indexed_scripts & scripts) {
    constexpr auto tpl = R"(
    enum class script {{
          {}
    }};
)";
    std::vector<std::pair<std::string, std::pair<uint16_t, std::string>>> sorted =
        {scripts.begin(), scripts.end()};
    std::ranges::sort(sorted, [](const auto & a, const auto & b) {
        if(a.second.first < b.second.first)
            return true;
        if(a.first == b.second.second)
            return true;
        return false;
    });


    auto formatted = sorted | std::views::transform([](const auto & pair) {
                         auto alias = pair.second.second;
                         auto value = pair.second.first;
                         return fmt::format("{0} = {1}", pair.first, !alias.empty()
                                                        ? alias : fmt::format("{}", value));
    });
    fmt::print(out, tpl, fmt::join(formatted, ",\n"));
}

void print_script_extension_data(FILE* out, std::size_t index,
                                 const std::vector<std::pair<char32_t, uint16_t>>& codepoints) {
    fmt::print(out,
        "template<> inline constexpr script_data<{}> script_properties<size_t({})>{{{{ {} }}}};\n", codepoints.size(), index,
        fmt::join(codepoints | std::views::transform([](const std::pair<char32_t, uint16_t>& rng) {
                             return fmt::format("({:#08x} << 11) | {}", std::uint32_t(rng.first), rng.second);

                                // uint32_t((rng.first << 11) | rng.second);
                  }),
                  ", "));
}

void print_script_switch(FILE* output, std::size_t max)
{
    constexpr const char* tpl =
R"(constexpr
script get_script_ext(char32_t c, std::size_t index) noexcept {{
        constexpr auto d = static_cast<uint16_t>(script::unknown);
        switch(index) {{
               {}
        }};
        return script::unknown;
}})";

    fmt::print(output, tpl, fmt::join(std::views::iota(std::size_t(0), max) | std::views::transform([](std::size_t s) {

         return fmt::format("case {0}: return static_cast<script>(script_properties<{0}>.lookup(c, d));", s);
    }), "\n"));
}

void print_scripts_data(FILE* out, const std::vector<codepoint>& all, const labels& l) {
    indexed_scripts scripts = sorted_scripts(l);

    print_scripts_enum(out, scripts);

    std::size_t script_index = 0;
    for(;;) {
        std::vector<std::pair<char32_t, uint16_t>> scripts_data;
        auto for_level = all | std::views::filter([&](const codepoint& c) {
                             return script_index < c.script_extensions.size();
                         }) |
                         ranges::to<std::vector>;
        if(for_level.empty()) {
            break;
        }
        auto it = for_level.begin();

        std::string prev_script = "unknown";
        for(std::size_t c = 0; c <= 0x10ffff; c++) {
            std::string script = "unknown";
            auto probe = std::ranges::lower_bound(it, for_level.end(), c, {}, &codepoint::value);
            if(probe != for_level.end() && probe->value == c) {
                if(script_index < probe->script_extensions.size()) {
                    it = probe;
                    script = to_lower(probe->script_extensions[script_index]);
                }
            }
            if(script != prev_script) {
                scripts_data.emplace_back(c, scripts[script].first);
                prev_script = script;
            }
        }
        print_script_extension_data(out, script_index, scripts_data);
        script_index++;
    }
    print_script_switch(out, 1);
}


}    // namespace cedilla::tools
