#include "fmt/core.h"
#include "load.hpp"
#include "codepoint_data.hpp"
#include "utils.hpp"
#include "generators.hpp"
#include <map>
#include <ranges>
#include <set>

namespace cedilla::tools {

void print_category_data(FILE* out, const std::vector<char32_t> & codepoints, std::string_view name) {
    print_binary_data_best(out, codepoints, fmt::format("gc_{}", name));
}

void print_categories_data(FILE* out, const std::vector<codepoint> & all, const labels & l) {
    // Write an enum for the category names
    constexpr auto tpl = R"(
namespace cedilla::details::generated {{
    enum class category {{
          {}
    }};
)";
    auto formatted_cats = l.categories | std::views::transform([](const auto & pair) {
        return fmt::format("{0},\n{1} = {0}", pair.first, pair.second);
    });
    fmt::print(out, tpl, fmt::join(formatted_cats, ",\n"));

    std::map<std::string_view, std::vector<std::string_view>> categories =
    {
        {"letter", {"lu", "ll", "lt", "lm", "lo"}},
        {"mark",   {"mn", "mc", "me"}},
        {"number", {"nd", "nl", "no"}},
        {"punctuation", {"pc", "pd", "ps", "pe", "pi", "pf", "po"}},
        {"punctuation", {"pc", "pd", "ps", "pe", "pi", "pf", "po"}},
        {"symbol", {"sm", "sc", "sk", "so"}},
        {"separator", {"zs", "zl", "zp"}},
        {"other",  {"cc", "cf", "cs", "co"}}
    };

    for(const auto & [meta, cats] : categories) {
        std::set<char32_t> aggregated;
        for(std::string_view cat : cats) {
            std::vector<char32_t> cps;
            auto filter = std::views::filter([cat](const codepoint & cp) {
                  return ci_equal(cp.general_category, cat);
                         }) | std::views::transform(&codepoint::value);
            for(char32_t c : all | filter) {
                aggregated.insert(c);
                cps.push_back(c);
            }
            print_category_data(out, cps, cat);

        }
        std::vector sorted(aggregated.begin(), aggregated.end());
        std::ranges::sort(sorted);
        print_category_data(out, sorted, meta);
    }

    fmt::print(out, "}}");


}


}
