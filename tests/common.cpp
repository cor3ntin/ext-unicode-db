#include "common.h"
#include "pugixml.hpp"
#include <string>
#include <iostream>
#include <charconv>

bool generated(char32_t c) {
    const std::array ranges = {
        std::pair{0xAC00, 0xD7A3},   std::pair{0x3400, 0x4DB5},   std::pair{0x4E00, 0x9FFA},
        std::pair{0x20000, 0x2A6D6}, std::pair{0x2A700, 0x2B734}, std::pair{0x2B740, 0x2B81D},
        std::pair{0x2B820, 0x2CEA1}, std::pair{0x2CEB0, 0x2EBE0}, std::pair{0x17000, 0x187EC},
        std::pair{0x1B170, 0x1B2FB}, std::pair{0x0F900, 0xFA6D},  std::pair{0x0FA70, 0xD7F9},
        std::pair{0x2F800, 0x2FA1D}};
    for(auto r : ranges) {
        if(c >= r.first && c <= r.second)
            return true;
    }
    return false;
}

std::unordered_map<char32_t, cp_test_data> load_test_data() {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(UCDXML_FILE);
    std::unordered_map<char32_t, cp_test_data> db;

    pugi::xml_node rep = doc.child("ucd").child("repertoire");
    for(pugi::xml_node cp : rep.children("char")) {
        try {
            auto code = char32_t(std::stoi(cp.attribute("cp").value(), 0, 16));
            auto name = generated(code) ? "" : cp.attribute("na").value();
            auto age = uni::__age_from_string(cp.attribute("age").value());
            auto category = uni::__category_from_string(cp.attribute("gc").value());
            auto block = uni::__block_from_string(cp.attribute("blk").value());
            auto script = uni::__script_from_string(cp.attribute("sc").value());

            std::istringstream iss(cp.attribute("scx").value());
            std::vector<std::string> results((std::istream_iterator<std::string>(iss)),
                                             std::istream_iterator<std::string>());
            std::vector<uni::script> exts;
            for(auto&& x : results) {
                exts.push_back(uni::__script_from_string(x));
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

            db[code] = {code, name, age, category, block, script, exts, n, d};
        } catch(...) {    // stoi...
        }
    }
    return db;
}