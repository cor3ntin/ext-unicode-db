#pragma once
#include <string>
#include <unordered_set>
#include <vector>

namespace cedilla::tools {

int binary_property_index(std::string_view);

struct codepoint {
    char32_t    value = 0;
    std::string name;
    std::unordered_set<std::int8_t> binary_properties;
    std::string age;
    bool reserved = false;
    std::string general_category;
    std::string script;
    std::vector<std::string> script_extensions;
    std::string numeric_value_str;
    std::int64_t numeric_value_numerator = 0 , numeric_value_denominator = 0;
    std::vector<char32_t> uppercase, lowercase, titlecase, casefold;
    std::vector<std::string> aliases;

    bool has_binary_property(std::string_view prop) const {
        return binary_properties.contains(binary_property_index(prop));
    }
};


}
