#include "common.h"
#include "pugixml.hpp"
#include <string>
#include <iostream>
#include <charconv>

bool generated(char32_t c) {
    using P = std::pair<char32_t, char32_t>;
    const std::array ranges = {
        P{0xAC00, 0xD7A3},   P{0x3400, 0x4DB5},   P{0x4E00, 0x9FFA},
        P{0x20000, 0x2A6D6}, P{0x2A700, 0x2B734}, P{0x2B740, 0x2B81D},
        P{0x2B820, 0x2CEA1}, P{0x2CEB0, 0x2EBE0}, P{0x17000, 0x187EC},
        P{0x1B170, 0x1B2FB}, P{0x0F900, 0xFA6D},  P{0x0FA70, 0xD7F9},
        P{0x2F800, 0x2FA1D}};
    for(auto r : ranges) {
        if(c >= r.first && c <= r.second)
            return true;
    }
    return false;
}

std::string fixup_name(std::string n, std::string_view cp) {
    auto p = n.find("#");
    if(p == std::string::npos)
        return n;
    auto c = n;
    c.replace(p, 1, cp);
    return c;
}

std::unordered_map<char32_t, cp_test_data> load_test_data() {
    pugi::xml_document doc;
    doc.load_file(UCDXML_FILE);
    std::unordered_map<char32_t, cp_test_data> db;

    pugi::xml_node rep = doc.child("ucd").child("repertoire");
    for(pugi::xml_node cp : rep.children("char")) {
        try {
            uint32_t first;
            uint32_t last;

            if(!cp.attribute("cp").empty())
            {
                first = last = char32_t(std::stoi(cp.attribute("cp").value(), 0, 16));
            }
            else
            {
                first = char32_t(std::stoi(cp.attribute("first-cp").value(), 0, 16));
                last  = char32_t(std::stoi(cp.attribute("last-cp").value(), 0, 16));

            }
            auto name = fixup_name(cp.attribute("na").value(), cp.attribute("cp").value());
            auto age = uni::detail::age_from_string(cp.attribute("age").value());
            auto category = uni::detail::category_from_string(cp.attribute("gc").value());
            auto block = uni::detail::block_from_string(cp.attribute("blk").value());
            auto script = uni::detail::script_from_string(cp.attribute("sc").value());

            std::istringstream iss(cp.attribute("scx").value());
            std::vector<std::string> results((std::istream_iterator<std::string>(iss)),
                                             std::istream_iterator<std::string>());
            std::vector<uni::script> exts;
            for(auto&& x : results) {
                auto n = uni::detail::script_from_string(x);
                if(n == uni::script::unknown)
                    continue;
                exts.push_back(uni::detail::script_from_string(x));
            }

            std::string nv = cp.attribute("nv").value();
            int64_t d = 0;
            int64_t n = 0;
            if(nv != "NaN") {
                d = 1;
                auto idx = nv.find('/');
                std::from_chars(nv.data(), nv.data() + idx, n);
                if(idx != std::string::npos) {
                    std::from_chars(nv.data() + idx + 1, nv.data() + nv.size(), d);
                }
            }
            for(auto code = first; code <= last; code++)
                db[code] = {code, name, age, category, block, script, exts, n, d, generated(code)};
        } catch(...) {    // stoi...
        }
    }
    return db;
}
