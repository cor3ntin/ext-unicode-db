#include <cedilla/details/generated_props.hpp>
#include <cassert>
#include <fmt/format.h>
#include "fmt/color.h"
#include "load.hpp"
#include <fmt/format.h>

#include <cedilla/properties.hpp>
#include <cedilla/normalization.hpp>
#include <cedilla/casing.hpp>
#include <cedilla/grapheme_cluster.hpp>
#include <cedilla/word_break.hpp>
#include <cedilla/utf.hpp>
#include <cedilla/details/to.hpp>
#include <string_view>
#include <string>


void print(std::u8string_view u8) {
    fmt::print("{}\n", std::string_view(reinterpret_cast<const char*>(u8.data()), u8.size()));
}

void f() {
    const char8_t* algo = u8R"(
toTitlecase(X): Find the word boundaries in X according to Unicode Standard
Annex #29, “Unicode Text Segmentation.” For each word boundary, find the first
cased character F following the word boundary. If F exists, map F to Titlecase_Mapping(F);
then map all characters C between F and the following word boundary to Lowercase_Mapping(C).
)";
    print(std::u8string_view(algo)
          | cedilla::title
          | cor3ntin::rangesnext::to<std::u8string>());

/*
   Totitlecase(X): Find The Word Boundaries In X According To Unicode Standard
   Annex #29, “Unicode Text Segmentation.” For Each Word Boundary, Find The First
   Cased Character F Following The Word Boundary. If F Exists, Map F To Titlecase_mapping(F);
   Then Map All Characters C Between F And The Following Word Boundary To Lowercase_mapping(C).
*/

}


int main(int, char**) {
    f();

    /*auto codepoints = cedilla::tools::load_codepoints(argv[1]);
    std::unordered_map<char32_t, cedilla::tools::codepoint> map;
    std::vector<bool> props(0x10FFFF, 0);
    for(const auto & c : codepoints) {
        map[c.value] = c;
        if(c.has_binary_property("alpha"))
            props[c.value] = 1;
    }*/

    //char32_t x = 0;
    //for(char32_t i = 0; i < 0x10FFFF; i++) {
        //auto e =  props[i];
     //   x+=  cedilla::details::generated::property_alpha.lookup(i);
        //if(e != r)
        //    fmt::print(fmt::fg(e == r? fmt::color::cyan : fmt::color::orange_red), "'U+{:04X}': {} <> {} \n", i, r, e);
    //}
    //return x;
}
