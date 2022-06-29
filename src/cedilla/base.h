#pragma once
#include <cstdint>
#include <algorithm>
#include <string_view>
#include "cedilla/synopsis.hpp"

namespace uni::detail {

template<class ForwardIt, class T, class Compare>
constexpr ForwardIt upper_bound(ForwardIt first, ForwardIt last, const T& value, Compare comp) {
    ForwardIt it = first;
    typename std::iterator_traits<ForwardIt>::difference_type count = std::distance(first, last);
    typename std::iterator_traits<ForwardIt>::difference_type step = count / 2;

    while(count > 0) {
        it = first;
        step = count / 2;
        std::advance(it, step);
        if(!comp(value, *it)) {
            first = ++it;
            count -= step + 1;
        } else
            count = step;
    }
    return first;
}

template<class ForwardIt, class T, class Compare>
constexpr ForwardIt lower_bound(ForwardIt first, ForwardIt last, const T& value, Compare comp) {
    ForwardIt it = first;
    typename std::iterator_traits<ForwardIt>::difference_type count = std::distance(first, last);
    typename std::iterator_traits<ForwardIt>::difference_type step = count / 2;

    while(count > 0) {
        it = first;
        step = count / 2;
        std::advance(it, step);
        if(comp(*it, value)) {
            first = ++it;
            count -= step + 1;
        } else
            count = step;
    }
    return first;
}

template<class ForwardIt, class T>
constexpr ForwardIt lower_bound(ForwardIt first, ForwardIt last, const T& value) {
    ForwardIt it = first;
    typename std::iterator_traits<ForwardIt>::difference_type count = std::distance(first, last);
    typename std::iterator_traits<ForwardIt>::difference_type step = count / 2;

    while(count > 0) {
        it = first;
        step = count / 2;
        std::advance(it, step);
        if(*it < value) {
            first = ++it;
            count -= step + 1;
        } else
            count = step;
    }
    return first;
}

template<class ForwardIt, class T>
constexpr bool binary_search(ForwardIt first, ForwardIt last, const T& value) {
    first = detail::lower_bound(first, last, value);
    return (!(first == last) && !(value < *first));
}
template<typename T, auto N>
struct compact_range {
    std::uint32_t _data[N];
    constexpr T value(char32_t cp, T default_value) const {
        const auto end = std::end(_data);
        auto it = detail::upper_bound(std::begin(_data), end, cp, [](char32_t local_cp, uint32_t v) {
            char32_t c = (v >> 8);
            return local_cp < c;
        });
        if(it == end)
            return default_value;
        it--;
        return *(it)&0xFF;
    }
};
template<class T, class... U>
compact_range(T, U...) -> compact_range<T, sizeof...(U) + 1>;


template<typename T, auto N>
struct compact_list {
    std::uint32_t _data[N];
    constexpr T value(char32_t cp, T default_value) const {
        const auto end = std::end(_data);
        auto it = detail::lower_bound(std::begin(_data), end, cp, [](uint32_t v, char32_t local_cp) {
            char32_t c = (v >> 8);
            return c < local_cp;
        });
        if(it == end || ((*it) >> 8) != cp)
            return default_value;
        return *(it)&0xFF;
    }
};
template<class T, class... U>
compact_list(T, U...)-> compact_list<T, sizeof...(U) + 1>;


template <typename T, std::size_t N>
struct array {
    using type = T[N];
};

template <typename T>
struct array<T, 0> {
    using type = T*;
};

template <typename T, std::size_t N>
using array_t = typename array<T, N>::type;



template<std::size_t ValueSize, std::size_t r1_s, std::size_t r2_s, int16_t r2_t_f, int16_t r2_t_b, std::size_t r3_s,
         std::size_t r4_s, int16_t r4_t_f, int16_t r4_t_b, std::size_t r5_s, int16_t r5_t_f,
         int16_t r5_t_b, std::size_t r6_s>
struct basic_trie {

    static_assert(ValueSize == 1 || ValueSize == 2);
    using ValueType = std::conditional_t<ValueSize == 2,
                      unsigned __int128, std::uint64_t>;

    // not tries, just bitmaps for all code points 0..0x7FF (UTF-8 1- and 2-byte sequences)
    ValueType r1[32];

    // trie for code points 0x800..0xFFFF (UTF-8 3-byte sequences, aka rest of BMP)

    uint8_t r2[r2_s];
    array_t<ValueType, r3_s> r3;    // leaves can be shared, so size isn't fixed

    // trie for 0x10000..0x10FFFF (UTF-8 4-byte sequences, aka non-BMP code points)
    array_t<std::uint8_t, r4_s> r4;
    array_t<std::uint8_t, r5_s> r5;   // two level to exploit sparseness of non-BMP
    array_t<ValueType, r6_s> r6;      // again, leaves are shared

    constexpr int lookup(char32_t u) const {
        std::uint32_t c = u;
        if(c < 0x800) {
            if constexpr(r1_s == 0){
                return 0;
            }
            else {
                return trie_range_leaf(c, r1[std::size_t(c >> 6)]);
            }
        } else if(c < 0x10000) {
            if constexpr(r3_s == 0) {
                return 0;
            }
            else {
                std::size_t i = (std::size_t(c >> 6) - 0x20);
                auto child = 0;
                if(i >= r2_t_f && i < r2_t_f + r2_s)
                    child = r2[i - r2_t_f];
                return trie_range_leaf(c, r3[child]);
            }
        } else {
            if constexpr(r6_s == 0)
                return 0;
            std::size_t i4 = (c >> 12) - 0x10;
            auto child = 0;
            if constexpr(r4_s > 0) {
                if(i4 >= r4_t_f && i4 < r4_t_f + r4_s)
                    child = r4[i4 - r4_t_f];
            }

            std::size_t i5 = static_cast<std::size_t>(std::size_t(child << 6) + (std::size_t(c >> 6) & std::size_t(0x3f)));
            auto leaf = 0;
            if constexpr(r5_s != 0) {
                if(i5 >= std::size_t(r5_t_f) && i5 < std::size_t(r5_t_f) + r5_s)
                    leaf = r5[i5 - std::size_t(r5_t_f)];
            }
            return trie_range_leaf(c, r6[leaf]);
        }
    }

    constexpr int trie_range_leaf(std::uint32_t c, std::uint64_t chunk) const {
        if constexpr(ValueSize ==1) {
            return (chunk >> (c & 0b111111)) & 0b1;
        }
        else {
            return  (((chunk  >> (c & 0b111111)) & 0b1) << 1 )
                  | (((chunk+1) >> (c & 0b111111)) & 0b1);
        }
    }
};

template<std::size_t size>
struct flat_array {
    char32_t data[size];
    constexpr bool lookup(char32_t u) const {
        if constexpr(size < 20) {
            for(auto it = std::begin(data); it != std::end(data); ++it) {
                if(*it == u)
                    return true;
                if(it == std::end(data))
                    return false;
            }
            return false;
        } else {
            return detail::binary_search(std::begin(data), std::end(data), u);
        }
    }
};


template<auto N>
struct range_array {
    std::uint32_t _data[N];
    constexpr bool lookup(char32_t cp) const {
        const auto end = std::end(_data);
        auto it = detail::upper_bound(std::begin(_data), end, cp, [](char32_t local_cp, uint32_t v) {
            char32_t c = (v >> 8);
            return local_cp < c;
        });
        if(it == end)
            return false;
        it--;
        return (*it) & 0xFF;
    }
};

template<class... U>
range_array(U...)->range_array<sizeof...(U)>;


constexpr char propcharnorm(char a) {
    if(a >= 'A' && a <= 'Z')
        return static_cast<char>(a + char(32));
    if(a == ' ' || a == '-')
        return '_';
    return a;
}

constexpr int propcharcomp(char a, char b) {
    a = propcharnorm(a);
    b = propcharnorm(b);
    if(a == b)
        return 0;
    if(a < b)
        return -1;
    return 1;
}

constexpr int propnamecomp(std::string_view sa, std::string_view sb) {
    // workaround, iterators in std::string_view are not constexpr in libc++ (for now)
    const char* a = sa.data();
    const char* b = sb.data();

    const char* ae = sa.data() + sa.size();
    const char* be = sb.data() + sb.size();

    for(; a != ae && b != be; a++, b++) {
        auto res = propcharcomp(*a, *b);
        if(res != 0)
            return res;
    }
    if(sa.size() < sb.size())
        return -1;
    else if(sb.size() < sa.size())
        return 1;
    return 0;
}

template <typename A, typename B>
struct pair
{
    A first;
    B second;
};

template <typename A, typename B>
pair(A, B) -> pair<A, B>;

struct string_with_idx { const char* name; uint32_t value; };


}    // namespace uni::detail

namespace uni {

constexpr double numeric_value::value() const {
    return static_cast<double>(numerator()) / static_cast<double>(_d);
}

constexpr long long numeric_value::numerator() const {
    return _n;
}

constexpr int numeric_value::denominator() const {
    return _d;
}

constexpr bool numeric_value::is_valid() const {
    return _d != 0;
}

constexpr numeric_value::numeric_value(long long n, int16_t d) : _n(n), _d(d) {}


}    // namespace uni
