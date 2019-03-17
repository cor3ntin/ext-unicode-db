#pragma once
#include <cstdint>
#include <array>
#include <algorithm>
namespace uni {

template<class ForwardIt, class T, class Compare>
constexpr ForwardIt upper_bound(ForwardIt first, ForwardIt last, const T& value, Compare comp) {
    ForwardIt it;
    typename std::iterator_traits<ForwardIt>::difference_type count, step;
    count = std::distance(first, last);

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
    ForwardIt it;
    typename std::iterator_traits<ForwardIt>::difference_type count, step;
    count = std::distance(first, last);

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

template<std::size_t r1_s, std::size_t r2_s, std::size_t r3_s, std::size_t r4_s, std::size_t r5_s,
         std::size_t r6_s>
struct __bool_trie {

    // not tries, just bitmaps for all code points 0..0x7FF (UTF-8 1- and 2-byte sequences)
    std::array<std::uint64_t, 32> r1;

    // trie for code points 0x800..0xFFFF (UTF-8 3-byte sequences, aka rest of BMP)
    std::array<std::uint8_t, r2_s> r2;
    std::array<std::uint64_t, r3_s> r3;    // leaves can be shared, so size isn't fixed

    // trie for 0x10000..0x10FFFF (UTF-8 4-byte sequences, aka non-BMP code points)
    std::array<std::uint8_t, r4_s> r4;
    std::array<std::uint8_t, r5_s> r5;     // two level to exploit sparseness of non-BMP
    std::array<std::uint64_t, r6_s> r6;    // again, leaves are shared

    constexpr bool lookup(char32_t u) const {
        std::uint32_t c = u;
        if(c < 0x800) {
            if constexpr(r1_s == 0)
                return false;
            return trie_range_leaf(c, r1[c >> 6]);
        } else if(c < 0x10000) {
            if constexpr(r3_s == 0)
                return false;
            auto child = r2[(c >> 6) - 0x20];
            return trie_range_leaf(c, r3.begin()[child]);
        } else {
            if constexpr(r6_s == 0)
                return false;
            auto child = r4[(c >> 12) - 0x10];
            auto leaf = r5.begin()[(child << 6) + ((c >> 6) & 0x3f)];
            return trie_range_leaf(c, r6.begin()[leaf]);
        }
    }

    constexpr bool trie_range_leaf(std::uint32_t c, std::uint64_t chunk) const {
        return (chunk >> (c & 0b111111)) & 0b1;
    }
};

template<std::size_t size>
struct flat_array {
    std::array<char32_t, size> data;
    constexpr bool lookup(char32_t u) const {
        if constexpr(size < 20) {
            for(auto it = data.begin(); it != data.end(); ++it) {
                if(*it == u)
                    return true;
                if(it == data.end())
                    return false;
            }
        } else {
            return std::binary_search(data.begin(), data.end(), u);
        }
        return false;
    }
};


struct __range_array_elem {
    char32_t c : 24;
    bool b = 1;
};
template<std::size_t size>
struct __range_array {
    std::array<__range_array_elem, size> data;

    constexpr bool lookup(char32_t u) const {
        if((char32_t)u > 0x10FFFF)
            return false;
        auto it =
            uni::upper_bound(data.begin(), data.end(), u,
                             [](char32_t cp, const __range_array_elem& e) { return cp < e.c; });
        if(it == data.end())
            return false;
        it--;
        return it->b;
    }
};


constexpr char __propcharnorm(char a) {
    if(a >= 'A' && a <= 'Z')
        return a + 32;
    if(a == ' ' || a == '-')
        return '_';
    return a;
}
constexpr int __propcharcomp(char a, char b) {
    a = __propcharnorm(a);
    b = __propcharnorm(b);
    if(a == b)
        return 0;
    if(a < b)
        return -1;
    return 1;
}
constexpr int __pronamecomp(std::string_view sa, std::string_view sb) {
    auto a = sa.begin();
    auto b = sb.begin();

    for(; a != sb.end() && b != sb.end(); a++, b++) {
        auto res = __propcharcomp(*a, *b);
        if(res != 0)
            return res;
    }
    if(sa.size() < sb.size())
        return -1;
    else if(sb.size() < sa.size())
        return 1;
    return 0;
}

}    // namespace uni

namespace uni {


enum class script;
enum class block;
enum class category;
enum class property;
enum class age : uint8_t;

template<uni::script>
bool cp_is(char32_t) = delete;

template<uni::category>
bool cp_is(char32_t) = delete;

template<uni::property>
bool cp_is(char32_t) = delete;

constexpr category cp_category(char32_t cp);
constexpr age cp_age(char32_t cp);
constexpr block cp_block(char32_t cp);
constexpr script cp_script(char32_t cp);
constexpr bool cp_is_valid(char32_t cp);
constexpr bool cp_is_assigned(char32_t cp);
constexpr bool cp_is_ascii(char32_t cp);

struct numeric_value {

    constexpr double value() const {
        return numerator() / double(_d);
    }

    constexpr long long numerator() const {
        return _n;
    }

    constexpr int denominator() const {
        return _d;
    }

    constexpr bool is_valid() const {
        return _d != 0;
    }

protected:
    constexpr numeric_value() = default;
    constexpr numeric_value(long long n, int16_t d) : _n(n), _d(d) {}

    long long _n = 0;
    int16_t _d = 0;
    friend constexpr numeric_value cp_numeric_value(char32_t cp);
};

constexpr numeric_value cp_numeric_value(char32_t cp);

}    // namespace uni