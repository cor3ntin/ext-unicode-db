#include <variant>
#include <cedilla/details/generated_props.hpp>
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include "codepoint_data.hpp"
#include "load.hpp"


std::optional<std::vector<cedilla::tools::codepoint>> codepoints;


int main(int argc, char** argv) {
    Catch::Session session;
    std::string ucd_path;
    using namespace Catch::Clara;
    auto cli
        = session.cli()
          | Opt(ucd_path, "ucd path" )
                ["-p"]["--ucd-path"]
          ("ucd path file");
    session.cli( cli );
    if(auto ret = session.applyCommandLine( argc, argv );  ret != 0) {
        return ret;
    }
    codepoints = cedilla::tools::load_codepoints(ucd_path);
    return session.run( argc, argv );
}

struct property_map {
    std::string_view name;
    using F = bool(char32_t);
    F* function;
};


TEST_CASE("Exhaustive properties cross checking")
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
