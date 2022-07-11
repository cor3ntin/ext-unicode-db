#include <catch2/catch_all.hpp>
#include <regex>
#include <fstream>
#include "utils.hpp"
#include <range/v3/range/conversion.hpp>
#include <cedilla/normalization.hpp>

using namespace cedilla::tools;

struct test {
    std::array<std::u32string, 5> data;
    std::string description;
    const auto & c(int i) const {
        return data[i-1];
    }

};

std::vector<test> nt;

std::vector<test> parse(std::string file) {
    std::regex re("([0-9A-F ]+);([0-9A-F ]+);([0-9A-F ]+);([0-9A-F ]+);([0-9A-F ]+);.*\\) (.+)",
                  std::regex::ECMAScript|std::regex::icase);
    std::ifstream infile(file);
    std::string line;
    std::vector<test> tests;
    tests.reserve(20000);
    while(std::getline(infile, line)) {
        std::smatch captures;
        if(line.starts_with("#"))
            continue;
        if(!std::regex_search(line, captures,  re))
            continue;
        test t;
        for (int i = 0; i <5; i++) {
            auto strs =  split(captures[i+1]);
            t.data[i] = strs | std::views::transform([](const std::string &s) {
                            return to_char32(s);
                        }) | ranges::to<std::u32string>();
        }
        t.description = captures[6];
        tests.push_back(std::move(t));
    }
    return tests;
}


int main(int argc, char** argv) {
    Catch::Session session;
    std::string normalization_tests_paths;
    using namespace Catch::Clara;
    auto cli
        = session.cli()
          | Opt(normalization_tests_paths, "NormalizationTest.txt path" )
                ["--path"];
    session.cli( cli );
    if(auto ret = session.applyCommandLine( argc, argv );  ret != 0) {
        return ret;
    }
    nt = parse(normalization_tests_paths);
    if(nt.size() < 15000)
        die("Couldn't load the test file");

    return session.run( argc, argv );
}

namespace Catch {
template<>
struct StringMaker<char32_t> {
    static std::string convert( char32_t c ) {
        return fmt::format("U+{:04X}", (uint32_t)c);
    }
};
}


TEST_CASE("NFD") {
    auto nfd = [](const std::u32string_view & v) {
        return cedilla::normalization_view<cedilla::normalization_form::nfd, std::u32string_view>(v)
               | ranges::to<std::u32string>();
    };

    for(const auto & test : nt) {
        INFO(test.description);
        CHECK(test.c(3) == nfd(test.c(1)));
        CHECK(test.c(3) == nfd(test.c(2)));
        CHECK(test.c(3) == nfd(test.c(3)));

        CHECK(test.c(5) == nfd(test.c(4)));
        CHECK(test.c(5) == nfd(test.c(5)));
    }
}

