#pragma once
#include <cstdint>
#include <algorithm>
#include <string_view>
namespace uni {

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
    first = uni::lower_bound(first, last, value);
    return (!(first == last) && !(value < *first));
}
template<typename T, auto N>
struct _compact_range {
    std::uint32_t _data[N];
    constexpr T value(char32_t cp, T default_value) const {
        const auto end = std::end(_data);
        auto it = uni::upper_bound(std::begin(_data), end, cp, [](char32_t cp, uint32_t v) {
            char32_t c = (v >> 8);
            return cp < c;
        });
        if(it == end)
            return default_value;
        it--;
        return *(it)&0xFF;
    }
};
template<class T, class... U>
_compact_range(T, U...)->_compact_range<T, sizeof...(U) + 1>;


template<typename T, auto N>
struct _compact_list {
    std::uint32_t _data[N];
    constexpr T value(char32_t cp, T default_value) const {
        const auto end = std::end(_data);
        auto it = uni::lower_bound(std::begin(_data), end, cp, [](uint32_t v, char32_t cp) {
            char32_t c = (v >> 8);
            return c < cp;
        });
        if(it == end || ((*it) >> 8) != cp)
            return default_value;
        return *(it)&0xFF;
    }
};
template<class T, class... U>
_compact_list(T, U...)->_compact_list<T, sizeof...(U) + 1>;


template<std::size_t r1_s, std::size_t r2_s, int16_t r2_t_f, int16_t r2_t_b, std::size_t r3_s,
         std::size_t r4_s, int16_t r4_t_f, int16_t r4_t_b, std::size_t r5_s, int16_t r5_t_f,
         int16_t r5_t_b, std::size_t r6_s>
struct __bool_trie {

    // not tries, just bitmaps for all code points 0..0x7FF (UTF-8 1- and 2-byte sequences)
    std::uint64_t r1[32];

    // trie for code points 0x800..0xFFFF (UTF-8 3-byte sequences, aka rest of BMP)
    uint8_t r2[r2_s];
    uint64_t r3[r3_s];    // leaves can be shared, so size isn't fixed

    // trie for 0x10000..0x10FFFF (UTF-8 4-byte sequences, aka non-BMP code points)
    std::uint8_t r4[r4_s];
    std::uint8_t r5[r5_s];   // two level to exploit sparseness of non-BMP
    std::uint64_t r6[r6_s];  // again, leaves are shared

    constexpr bool lookup(char32_t u) const {
        std::uint32_t c = u;
        if(c < 0x800) {
            if constexpr(r1_s == 0)
                return false;
            return trie_range_leaf(c, r1[c >> 6]);
        } else if(c < 0x10000) {
            if constexpr(r3_s == 0)
                return false;
            std::size_t i = ((c >> 6) - 0x20);
            auto child = 0;
            if(i >= r2_t_f && i < r2_t_f + r2_s)
                child = r2[i - r2_t_f];

            return trie_range_leaf(c, r3[child]);
        } else {
            if constexpr(r6_s == 0)
                return false;
            std::size_t i4 = (c >> 12) - 0x10;
            auto child = 0;
            if(i4 >= r4_t_f && i4 < r4_t_f + r4_s)
                child = r4[i4 - r4_t_f];


            std::size_t i5 = (child << 6) + ((c >> 6) & 0x3f);
            auto leaf = 0;
            if(i5 >= r5_t_f && i5 < r5_t_f + r5_s)
                leaf = r5[i5 - r5_t_f];
            return trie_range_leaf(c, r6[leaf]);
        }
    }

    constexpr bool trie_range_leaf(std::uint32_t c, std::uint64_t chunk) const {
        return (chunk >> (c & 0b111111)) & 0b1;
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
        } else {
            return uni::binary_search(std::begin(data), std::end(data), u);
        }
        return false;
    }
};


template<auto N>
struct __range_array {
    std::uint32_t _data[N];
    constexpr bool lookup(char32_t cp) const {
        const auto end = std::end(_data);
        auto it = uni::upper_bound(std::begin(_data), end, cp, [](char32_t cp, uint32_t v) {
            char32_t c = (v >> 8);
            return cp < c;
        });
        if(it == end)
            return false;
        it--;
        return (*it) & 0xFF;
    }
};

template<class... U>
__range_array(U...)->__range_array<sizeof...(U)>;


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
    // workaround, iterators in std::string_view are not constexpr in libc++ (for now)
    const char* a = sa.begin();
    const char* b = sb.begin();

    const char* ae = sa.end();
    const char* be = sb.end();

    for(; a != ae && b != be; a++, b++) {
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

template <typename A, typename B>
struct pair
{
    A first;
    B second;
};

template <typename A, typename B>
pair(A, B) -> pair<A, B>;


}    // namespace uni

namespace uni {


enum class script;
enum class block;
enum class category;
enum class property;
enum class version : uint8_t;

template<uni::script>
bool cp_is(char32_t) = delete;


template<uni::property>
bool cp_is(char32_t) = delete;


constexpr version cp_age(char32_t cp);
constexpr block cp_block(char32_t cp);

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
