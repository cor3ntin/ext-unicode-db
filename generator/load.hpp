#pragma once
#include "codepoint_data.hpp"
#include <unordered_map>

namespace cedilla::tools {

struct categories {
    std::unordered_multimap<std::string, std::string> names;
};

std::vector<codepoint> load_codepoints(std::string db);
categories load_categories(const std::string & prop_alias_file);

}
