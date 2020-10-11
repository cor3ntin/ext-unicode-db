#ifndef UNI_SINGLE_HEADER
#    pragma once
#    include "extra.h"
#endif

// More regex support for ctre

namespace uni {


constexpr __binary_prop __binary_prop_from_string(const std::string_view s) {
    for(const auto& c : __binary_prop_names) {
        const auto res = __pronamecomp(s, c.name);
        if(res == 0)
            return __binary_prop(c.value);
    }
    return __binary_prop::unknown;
}

template<>
constexpr bool __get_binary_prop<__binary_prop::ascii>(char32_t c) {
    return cp_is_ascii(c);
}

template<>
constexpr bool __get_binary_prop<__binary_prop::assigned>(char32_t c) {
    return cp_is_assigned(c);
}

template<>
constexpr bool __get_binary_prop<__binary_prop::any>(char32_t c) {
    return cp_is_valid(c);
}

}    // namespace uni