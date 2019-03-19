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
            db[code] = {code, age, category, block, script, exts};
        } catch(...) {    // stoi...
        }
    }
    return db;
}