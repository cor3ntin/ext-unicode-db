#include "common.h"
#include "pugixml.hpp"
#include <string>
#include <iostream>

std::unordered_map<char32_t, cp_test_data> load_test_data() {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(UCDXML_FILE);
    std::unordered_map<char32_t, cp_test_data> db;

    pugi::xml_node rep = doc.child("ucd").child("repertoire");
    for(pugi::xml_node cp : rep.children("char")) {
        try {
            auto code = char32_t(std::stoi(cp.attribute("cp").value(), 0, 16));
            auto age = uni::__age_from_string(cp.attribute("age").value());
            auto category = uni::__category_from__string(cp.attribute("gc").value());
            auto block = uni::__block_from_string(cp.attribute("blk").value());
            auto script = uni::__script_from_string(cp.attribute("sc").value());
            db[code] = {code, age, category, block, script};
        } catch(...) {    // stoi...
        }
    }
    return db;
}