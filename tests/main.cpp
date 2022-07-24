#include <variant>
#include <set>
#include <ranges>
#include <cedilla/properties.hpp>
#include <cedilla/normalization.hpp>
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include "codepoint_data.hpp"
#include "load.hpp"
#include <fmt/core.h>
#include <fmt/ranges.h>
#include "globals.hpp"
#include "utils.hpp"


using namespace cedilla::tools;

int main(int argc, char** argv) {
    Catch::Session session;
    std::string ucd_path;
    std::string aliases_path;
    std::string normalization_tests_paths;
    std::string grapheme_tests_paths;
    using namespace Catch::Clara;
    auto cli
        = session.cli()
          | Opt(ucd_path, "ucd path" )
                ["--ucd-path"]
          | Opt(aliases_path, "PropertyValueAliases.txt path" )
                ["--aliases-path"]
          | Opt(normalization_tests_paths, "NormalizationTest.txt path" )
                ["--normalization-tests"]
          | Opt(grapheme_tests_paths, "GraphemeBreakTest.txt path" )
                ["--graphemes-tests"];

    session.cli( cli );
    if(auto ret = session.applyCommandLine( argc, argv );  ret != 0) {
        return ret;
    }
    ::codepoints = cedilla::tools::load_codepoints(ucd_path);
    ::labels = cedilla::tools::load_labels(aliases_path);
    nt = parse_normalization_tests(normalization_tests_paths);
    if(nt.size() < 15000)
        die("Couldn't load the test file");
    grapheme_tests = parse_grapheme_break_tests(grapheme_tests_paths);
    if(grapheme_tests.size() < 500)
        die("Couldn't load the test file");

    return session.run( argc, argv );
}
