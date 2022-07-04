#pragma once
#include "codepoint_data.hpp"
#include <unordered_map>

namespace cedilla::tools {

struct labels {
    std::unordered_multimap<std::string, std::string> categories;
    std::unordered_multimap<std::string, std::string> scripts;
};

std::vector<codepoint> load_codepoints(std::string db);
labels load_labels(const std::string & prop_alias_file);

}
