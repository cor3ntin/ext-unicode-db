#include "load.hpp"
#include "pugixml.hpp"
#include "utils.hpp"
#include <charconv>
#include <algorithm>
#include <ranges>
#include <string>

namespace cedilla::tools {

static std::string fixup_name(std::string n, char32_t cp) {
    auto p = n.find("#");
    if(p == std::string::npos)
        return n;
    n.replace(p, 1, to_hex(cp));
    return n;
}

const char* binary_props[] = {
    "AHex",
    "Alpha",
    "Bidi_C",
    "Bidi_M",
    "Cased",
    "CE",
    "CI",
    "Comp_Ex",
    "Dash",
    "Dep",
    "DI",
    "Dia",
    "Ext",
    "Gr_Base",
    "Gr_Ext",
    "Hex",
    //          "Hyphen",
    "IDC",
    "Ideo",
    "IDS",
    "IDSB",
    "IDST",
    "Join_C",
    "LOE",
    "Lower",
    "Math",
    "NChar",
    "OAlpha",
    "ODI",
    "OGr_Ext",
    "OIDC",
    "OIDS",
    "OLower",
    "OMath",
    "OUpper",
    "Pat_Syn",
    "Pat_WS",
    "PCM",
    "QMark",
    "Radical",
    "RI",
    "SD",
    "STerm",
    "Term",
    "UIdeo",
    "Upper",
    "VS",
    "WSpace",
    "XIDC",
    "XIDS",

    "Emoji",
    "EPres",
    "EMod",
    "EBase",
    "EComp",
    "ExtPict"

    "CWL",
    "CWT",
    "CWU",
    "CWCF"
};


/*
 * struct codepoint {
    char32_t    value = 0;
    std::string name;
    std::set<std::string> binary_properties;
    std::string age = 0;
    bool reserved = false;
    std::string general_category;
    std::string script;
    std::string script_extensions;
    std::string numeric_value;
    std::vector<char32_t> uppercase, lowercase, titlecase, casefold;
    std::vector<std::string> aliases;
};
*/

static void parse_full_case_mapping(const pugi::xml_node & n, const char* property, std::vector<char32_t> & out) {
    using namespace std::literals;
    if(n.attribute(property).value() == "#"sv)
        return;
    auto lst = split(n.attribute(property).value());
    std::ranges::transform(lst, std::back_inserter(out), &to_char32);
}

static codepoint load_one(const pugi::xml_node & n) {
    using namespace std::literals;

    codepoint c;
    c.value = to_char32(n.attribute("cp").value());
    c.name  = fixup_name(n.attribute("na").value(), c.value);
    c.age   = n.attribute("age").value();
    c.general_category = to_lower(n.attribute("gc").value());
    c.reserved = is_in(c.general_category, {"co", "cn", "cs"});
    if(c.reserved)
        return c;
    c.script = to_lower(n.attribute("sc").value());
    c.script_extensions = split(n.attribute("scx").value());
    std::ranges::transform(c.script_extensions, c.script_extensions.begin(), &to_lower);


    // numeric value, if any
    std::string nv =  n.attribute("nv").value();
    if(nv != "NaN") {
        c.numeric_value_str = nv;
        c.numeric_value_denominator = 1;
        auto idx = nv.find('/');
        std::from_chars(nv.data(), nv.data() + idx, c.numeric_value_numerator);
        if(idx != std::string::npos) {
            std::from_chars(nv.data() + idx + 1, nv.data() + nv.size(), c.numeric_value_denominator);
        }
    }
    // aliases
    for(pugi::xml_node an : n.children("name-alias")) {
        auto alias = std::string(an.attribute("alias").value());
        c.aliases.emplace_back(fixup_name(std::move(alias), c.value));
    }

    // binary props
    for (int i = 0; i < std::ranges::size(binary_props); i++) {
        if(n.attribute(binary_props[i]).value() == "Y"sv)
            c.binary_properties.insert(i);
    }

    parse_full_case_mapping(n, "uc", c.uppercase);
    parse_full_case_mapping(n, "lc", c.lowercase);
    parse_full_case_mapping(n, "tc", c.titlecase);
    parse_full_case_mapping(n, "cf", c.casefold);

    return c;
}

std::vector<codepoint> load_codepoints(std::string db) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(db.c_str());
    std::vector<codepoint> codepoints;
    codepoints.reserve(0x110000);

    pugi::xml_node rep = doc.child("ucd").child("repertoire");
    for(pugi::xml_node node : rep.children("char")) {
        uint32_t first;
        uint32_t last;
        if(!node.attribute("cp").empty()) {
            first = last = to_char32(node.attribute("cp").value());
            codepoints.push_back(load_one(node));
        }
        else {
            first = to_char32(node.attribute("first-cp").value());
            last  = to_char32(node.attribute("last-cp").value());
            auto tpl = load_one(node);
            for(auto code = first; code <= last; code++) {
                tpl.value = code;
                tpl.name  = fixup_name(node.attribute("na").value(), code);
                codepoints.push_back(tpl);
            }
        }
    }

    return codepoints;
}

int binary_property_index(std::string_view needle) {
    auto it = std::ranges::find_if(binary_props, [&](const auto & elem) {
        return ci_equal(elem, needle);
    });
    if(it == std::ranges::end(binary_props))
        return -1;
    return std::distance(std::ranges::begin(binary_props), it);
}

}
