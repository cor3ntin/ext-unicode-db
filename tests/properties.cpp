#include <variant>
#include <set>
#include <ranges>
#include <cedilla/properties.hpp>
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include "codepoint_data.hpp"
#include "load.hpp"
#include <fmt/core.h>
#include <fmt/ranges.h>


std::optional<std::vector<cedilla::tools::codepoint>> codepoints;
std::optional<cedilla::tools::labels> labels;


int main(int argc, char** argv) {
    Catch::Session session;
    std::string ucd_path;
    std::string aliases_path;
    using namespace Catch::Clara;
    auto cli
        = session.cli()
          | Opt(ucd_path, "ucd path" )
                ["--ucd-path"]
          | Opt(aliases_path, "PropertyValueAliases.txt path" )
                ["--aliases-path"];
    session.cli( cli );
    if(auto ret = session.applyCommandLine( argc, argv );  ret != 0) {
        return ret;
    }
    codepoints = cedilla::tools::load_codepoints(ucd_path);
    labels = cedilla::tools::load_labels(aliases_path);
    return session.run( argc, argv );
}

struct property_map {
    std::string_view name;
    using F = bool(char32_t);
    F* function;
};


TEST_CASE("Exhaustive properties checking")
{
    #define P(X) property_map{#X, &cedilla::is_##X}
    auto prop = GENERATE(
        P(alpha),
        P(dash),
        P(hex),
        P(qmark),
        P(term),
        P(sterm),
        P(di),
        P(dia),
        P(ext),
        P(sd),
        P(math),
        P(wspace),
        P(ri),
        P(nchar),
        P(vs),
        P(emoji),
        P(epres),
        P(emod),
        P(ebase),
        P(pat_ws),
        P(pat_syn),
        P(xids),
        P(xidc),
        P(ideo),
        P(upper),
        P(lower)
     );
    #undef P

    INFO("property: " << prop.name);
    std::vector<bool> props(0x10FFFF, 0);
    for(const auto & c : *codepoints) {
        INFO("cp: U+" << std::hex << (uint32_t)c.value << " " << c.name);
        CHECK(c.has_binary_property(prop.name) == prop.function(c.value));
    }
}

TEST_CASE("cp_script") {
    std::set<std::string> scripts_set;
    scripts_set.insert("zzzz");
    for(const auto & [k, _] : labels->scripts) {
        scripts_set.insert(k);
    }

    std::vector scripts(scripts_set.begin(), scripts_set.end());
    std::ranges::sort(scripts);
    auto unknown = std::ranges::find(scripts, "zzzz");
    if(unknown != scripts.end()) {
        scripts.erase(unknown);
    }
    scripts.insert(scripts.begin(), "zzzz");

    auto it = codepoints->begin();
    const auto end = codepoints->end();

    for(char32_t c = 0; c < 0x10FFFF; c++) {
        if(auto p = std::ranges::lower_bound(it, end, c, {}, &cedilla::tools::codepoint::value); p != end && p->value == c && ! p->reserved) {
            REQUIRE((uint32_t)cedilla::cp_script(c)  == std::distance(scripts.begin(), std::ranges::find(scripts, p->script)));
            it = p+1;
        }
        else {
            CHECK(cedilla::cp_script(c)  == cedilla::script::unknown);
        }
    }
}
