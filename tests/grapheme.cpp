#include "globals.hpp"
#include <fstream>
#include <regex>
#include <range/v3/view/split.hpp>
#include <range/v3/range/conversion.hpp>
#include <catch2/catch_all.hpp>
#include "utils.hpp"
#include <cedilla/grapheme_cluster.hpp>

std::string trim(std::string s) {
    s = std::regex_replace(s, std::regex("^\\s+"), std::string(""));
    s = std::regex_replace(s, std::regex("\\s+$"), std::string(""));
    return s;
}

std::vector<grapheme_break_test> parse_grapheme_break_tests(std::string file) {
    std::regex re("^(.+)\\s*#(.+)$",
                  std::regex::ECMAScript|std::regex::icase);
    std::ifstream infile(file);
    std::string line;
    std::vector<grapheme_break_test> tests;
    while(std::getline(infile, line)) {
        if(line.starts_with("#"))
            continue;
        std::smatch captures;
        if(!std::regex_search(line, captures,  re))
            continue;
        grapheme_break_test test;
        test.description = trim(captures[2]);
        line = captures[1];
        std::u32string current;
        for(auto && r : line | ranges::views::split(' ')) {
            std::string s = trim(ranges::to<std::string>(r));
            if(s.empty())
                continue;
            if(s == "÷") {
                if(current.empty())
                    continue;
                test.expected.push_back(current);
                current.clear();
                continue;
            }
            if(s == "×")
                continue;
            char32_t c = cedilla::tools::to_char32(trim(s));
            if(c == 0)
                continue;
            current.push_back(c);
            test.input.push_back(c);
        }
        if(!current.empty())
            test.expected.push_back(current);
        tests.push_back(std::move(test));
    }
    return tests;
}


TEST_CASE("Grapheme breaks", "[clusterization]") {
    auto graphemes = [](const std::u32string_view & v) {
        std::vector<std::u32string> graphemes;
        for(auto && s : cedilla::grapheme_clusters_view<std::u32string_view>(v)) {
            graphemes.push_back(s | ranges::to<std::u32string>());
        }
        return graphemes;
    };
    for(const auto & test : grapheme_tests) {
        //if(test.description.find("÷ [0.2] WATCH (ExtPict) × [9.0] COMBINING DIAERESIS (Extend_ExtCccZwj) ÷ [999.0] SPACE (Other) ÷ [0.3]") == std::string::npos)
        //     continue;

        INFO(test.description);
        CHECK(graphemes(test.input) == test.expected);
    }
}
