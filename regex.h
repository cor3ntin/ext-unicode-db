#ifndef UNI_SINGLE_HEADER
#    pragma once
#    include "extra.h"
#endif

// More regex support for ctre

namespace uni::detail {


constexpr binary_prop binary_prop_from_string(std::string_view s) {
    for(const auto& c : tables::binary_prop_names) {
        const auto res = propnamecomp(s, c.name);
        if(res == 0)
            return binary_prop(c.value);
    }
    return binary_prop::unknown;
}

template<>
constexpr bool get_binary_prop<binary_prop::ascii>(char32_t c) {
    return cp_is_ascii(c);
}

template<>
constexpr bool get_binary_prop<binary_prop::assigned>(char32_t c) {
    return cp_is_assigned(c);
}

template<>
constexpr bool get_binary_prop<binary_prop::any>(char32_t c) {
    return cp_is_valid(c);
}

constexpr bool is_unknown(binary_prop s)
{
    return s == binary_prop::unknown;
}

}    // namespace uni::detail