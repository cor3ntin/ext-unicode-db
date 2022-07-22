#pragma once
#include <cedilla/details/generated_normalization.hpp>
#include <ranges>
#include <vector>
#include <assert.h>

namespace cedilla {

enum class normalization_form {
    nfd,
    nfc,
    nfdk,
    nfck,
};

namespace details {


enum class hangul_syllable_kind {
    invalid,
    leading   = 0x01,
    vowel     = 0x02,
    trailing  = 0x04
};

constexpr inline char32_t hangul_lbase = 0x1100;    // First leading Jamo
static const char32_t hangul_lcount = 19;
static const char32_t hangul_vbase = 0x1161;    // First vowel
static const char32_t hangul_vcount = 21;
static const char32_t hangul_tbase = 0x11A7;    // First trailing
static const char32_t hangul_tcount = 28;
static const char32_t hangul_sbase = 0xAC00;    // First composed
static const char32_t hangul_ncount = 588;
static const char32_t hangul_scount = 11172;

constexpr inline bool is_decomposable_hangul(char32_t codepoint) {
    return codepoint >= hangul_sbase && codepoint < 0xD800;
}

constexpr inline char32_t hangul_syllable_index(char32_t codepoint) {
    return codepoint - hangul_sbase;
}

constexpr inline bool is_hangul_lpart(char32_t codepoint) {
    return codepoint >= 0x1100 && codepoint <= 0x1112;
}

constexpr inline bool is_hangul_vpart(char32_t codepoint) {
    return codepoint >= 0x1161 && codepoint <= 0x1175;
}

constexpr inline bool is_hangul_tpart(char32_t codepoint) {
    return codepoint >= 0x11A7 && codepoint <= 0x11C2;
}

constexpr inline bool is_hangul(char32_t codepoint) {
    return codepoint >= 0x1100 && codepoint < 0xD800;
}

inline hangul_syllable_kind hangul_syllable_type(char32_t codepoint) {
    if(!is_decomposable_hangul(codepoint))
        return hangul_syllable_kind::invalid;

    const auto & table = details::generated::hangul_syllables;
    if(codepoint < table->cp)
        return hangul_syllable_kind::invalid;
    auto it = details::branchless_lower_bound(table, codepoint,
                                              [] (details::generated::hangul_syllable s, char32_t needle) {
        return s.cp < needle;
    });

    if(it == std::ranges::end(table))
        return hangul_syllable_kind::invalid;

    return hangul_syllable_kind(it->kind);
}


constexpr auto no_composition = static_cast<char32_t>(0x10FFFF);


inline char32_t get_canonical_composition(char32_t L, char32_t C) {
    const auto & map = details::generated::canonical_composition_mapping;
    const auto & small = details::generated::canonical_decomposition_mapping_small;
    const auto & large = details::generated::canonical_decomposition_mapping_large;

    auto it = details::branchless_lower_bound(map, L, [] (uint32_t value, char32_t needle) {
        return (value >> 15) < needle;
    });


    for(; it != std::ranges::end(map) && uint32_t(*it >> 15) == L; ++it) {
        auto idx = *it &  0x3FFF;
        if(L <= 0xFFFF && idx < std::ranges::size(small) && small[idx].r1 == L) {
            if(small[idx].r2 == C)
                return small[idx].key;
            continue;
        }
        else if(idx < std::ranges::size(large)) {
            const auto & e = large[idx];
            auto eL = char32_t(e >> 21) & 0x1FFFFF;
            auto eC = char32_t(e) & 0x1FFFFF;
            if(eL == L && eC == C)
                return (e >> 42);
        }
    }
    // TODO assert ?
    return no_composition;
};

inline std::array<char32_t, 2> get_canonical_decomposition(char32_t c) {
    if(c <= 0xffff) {
        const auto & a = details::generated::canonical_decomposition_mapping_small;
        auto it = details::branchless_lower_bound(a, c, [](const details::generated::small_decomposition & a, const auto& b){
            return a.key < b;
        });
        if(it != std::ranges::end(a) && it->key == c) {
            return {it->r1, it->r2};
        }
        // a few codepoints are smaller than 16 bits but have a larger decomposition, so we need to
        // fall through to the large array search
    }
    //  k << 42 | v1 << 21 | v2
    const auto & a = details::generated::canonical_decomposition_mapping_large;
    auto comp = [](std::uint64_t k, char32_t needle) {
        return (k >> 42) < needle;
    };
    auto it = details::branchless_lower_bound(a, c, comp);
    if(it != std::ranges::end(a) && (*it >> 42) == c) {
        return {char32_t(*it >> 21) & 0x1FFFFF, char32_t(*it) & 0x1FFFFF};
    }
    return {};
}

}

template <normalization_form Form, std::ranges::input_range V>
requires std::ranges::view<V>
       && std::is_same_v<std::remove_cvref_t<std::ranges::range_reference_t<V>>, char32_t>
    class normalization_view : public std::ranges::view_interface<normalization_view<Form, V>>
{

public:
    constexpr normalization_view(V v): base(std::move(v)) {}

    struct iterator {
    private:
        using base_it = std::ranges::iterator_t<V>;
        const normalization_view* parent;
        base_it base_;
        struct codepoint {
            char32_t c   : 21;
            uint8_t  ccc : 8 ;
        };
        std::vector<codepoint> buffer;
        std::size_t idx = 0;

        void decompose_pair(const std::array<char32_t, 2> & canonical, uint8_t& last_ccc) {
            assert(canonical[0] != 0 && "No canonical composition for a cp with nfd_qc == NO");
            for(std::size_t N = 0; N < 2; N++) {
                uint8_t ccc = details::generated::ccc_data.lookup(canonical[N]);
                if(!canonical[N])
                    break;
                if(details::generated::nfd_qc_no.lookup(canonical[N])) {
                    decompose_recursively(canonical[N], last_ccc);
                }
                else {
                    buffer.emplace_back(canonical[N], ccc);
                }
            }
        }

        void decompose_recursively(char32_t c, uint8_t& last_ccc) {
            assert(details::generated::nfd_qc_no.lookup(c)
                   && "called decompose_recursively on a cp with nfd_qc == NO");
            std::array<char32_t, 2> canonical = details::get_canonical_decomposition(c);
            decompose_pair(canonical, last_ccc);
        }
        static constexpr bool is_canonical() {
            return Form == normalization_form::nfd || Form == normalization_form::nfc;
        }
        static constexpr bool is_composition() {
            return Form == normalization_form::nfc || Form == normalization_form::nfck;
        }

        constexpr void canonical_sort() {
            if(buffer.size() < 2)
                return;
            using std::swap;
            bool found = false;
            do {
                found = false;
                for(auto it = buffer.begin(); it + 1 != buffer.end(); ++it) {
                    const auto a = it->ccc, b = (it + 1)->ccc;
                    if(a > b && b > 0) {
                        swap(*it, *(it + 1));
                        found = true;
                    }
                }
            } while(found);
        }

        constexpr int recompose_hangul(int start_index, int index) requires (is_composition()) {
            if(start_index < 0)
                return 0;
            const auto LPart = buffer[start_index].c;

            if(details::is_hangul_lpart(buffer[start_index].c)) {
                const auto VPart = buffer[index].c;
                if(details::is_hangul_vpart(VPart)) {
                    bool has_tpart = false;
                    if(index + 1 < int(buffer.size()) && details::is_hangul_tpart(buffer[index+1].c)) {
                        has_tpart = true;
                    }
                    char32_t T = has_tpart ? buffer[index+1].c : details::hangul_tbase;;
                    const auto lindex  = LPart - details::hangul_lbase;
                    const auto vindex  = VPart - details::hangul_vbase;
                    const auto lvindex = lindex * details::hangul_ncount + vindex * details::hangul_tcount;
                    const auto tindex  = T - details::hangul_tbase;
                    const auto replacement = details::hangul_sbase + lvindex + tindex;
                    buffer[start_index] = { replacement, 0 };
                    buffer[index] = {details::no_composition, 0};
                    if(!has_tpart)
                        return index;

                    buffer[index+1] = {details::no_composition, 0};
                    return index + 1;
                }
                return 0;
            }

            if(details::is_hangul_tpart(buffer[index].c) && details::is_decomposable_hangul(LPart)) {
                const auto TPart = buffer[index].c;
                 auto index = details::hangul_syllable_index(LPart);
                 if((index % details::hangul_tcount) != 0) {
                     return index;
                }
                const auto tindex = TPart - (details::hangul_tbase);
                const auto replacement = LPart + tindex;
                buffer[start_index] = { replacement, 0 };
                buffer[index] = {details::no_composition, 0};
                return index;
            }
            return 0;
        }

        constexpr void recompose() requires (is_composition()) {
            int starter_idx = buffer[0].ccc == 0 ?  0 : -1;
            uint8_t ccc = 0;
            bool blocked = false;
            int  erased = 0;
            for(int i = 1; i < int(buffer.size()); i++) {
                if(buffer[i].c == details::no_composition)
                    continue;
                blocked = starter_idx == -1 || (starter_idx != (i-erased)-1 && ccc >= buffer[i].ccc);
                ccc = std::max(ccc, buffer[i].ccc);
                if(!blocked) {
                    if(auto n = recompose_hangul(starter_idx, i); n != 0) {
                        i = n;
                        erased += n;
                        ccc = 0;
                        continue;
                    }
                    char32_t L = buffer[starter_idx].c;
                    char32_t C = buffer[i].c;
                    char32_t replacement = details::get_canonical_composition(L, C);
                    if(replacement !=  details::no_composition) {
                        buffer[starter_idx] = {replacement, (uint8_t)details::generated::ccc_data.lookup(replacement)};
                        buffer[i] = codepoint(details::no_composition, 0);
                        erased++;
                        ccc = 0;
                        continue;
                    }
                }
                if(buffer[i].ccc == 0) {
                    starter_idx = i;
                    blocked = false;
                    erased  = 0;
                    ccc = 0;
                }
            }

            std::erase_if(buffer, [](const codepoint & c) {
                return c.c == details::no_composition;
            });
        }

        constexpr bool is_nfc(char32_t c) {
            return !details::generated::nfc_qc_no.lookup(c);
        }

        constexpr bool is_nfd(char32_t c) {
            return !details::generated::nfd_qc_no.lookup(c);
        }

        void decompose_hangul(char32_t codepoint) {
            using namespace details;
            auto index = hangul_syllable_index(codepoint);
            auto lpart = hangul_lbase + (index / hangul_ncount);
            auto vpart = hangul_vbase + (index % hangul_ncount) / hangul_tcount;

            buffer.emplace_back(lpart, 0);
            buffer.emplace_back(vpart, 0);

            auto tpart = index % hangul_tcount;
            if(tpart > 0) {
                tpart = tpart + hangul_tbase;
                buffer.emplace_back(tpart, 0);
            }
        }


        constexpr void decompose_next() {
            uint8_t last_ccc = 0;
            while(base_ != parent->base.end()) {
                auto n = *(base_);
                codepoint c = {n, (uint8_t)details::generated::ccc_data.lookup(n)};
                bool is_nfd = this->is_nfd(n);
                if constexpr(is_composition()) {
                    if(c.ccc == 0 && !buffer.empty() && is_nfc(n)) {
                        break;
                    }
                }
                else if(is_nfd && c.ccc == 0 && !buffer.empty()) {
                    break;
                }
                base_ ++;
                if(is_nfd) {
                    buffer.emplace_back(c);
                    continue;
                }
                if(details::is_decomposable_hangul(c.c)) {
                    decompose_hangul(c.c);
                }
                else {
                    std::array<char32_t, 2> canonical = details::get_canonical_decomposition(c.c);
                    decompose_pair(canonical, last_ccc);
                }
            }

            canonical_sort();

            if constexpr(is_composition()) {
                if(buffer.size()>=2)
                    recompose();
            }

        }

        void advance() {
            idx++;
            if(idx < buffer.size()) {
                return;
            }
            buffer.clear();
            idx = 0;
            decompose_next();
        }
        void back();
    public:
        using value_type = char32_t;
        using reference = char32_t;
        using difference_type   = std::ranges::range_difference_t<V>;
        using iterator_category = decltype([] {
            if constexpr (std::ranges::bidirectional_range<V>)
                return std::bidirectional_iterator_tag{};
            else if constexpr (std::ranges::forward_range<V>)
                return std::forward_iterator_tag{};
            else
                return std::input_iterator_tag{};
        }());

        constexpr iterator() requires std::default_initializable<base_it> = default;
        constexpr iterator(const normalization_view* p, base_it base): parent(p), base_(base) {
            decompose_next();
        };

        constexpr const base_it& base() const & noexcept {
            return base_;
        }

        constexpr base_it base() && {
            return std::move(base_);
        }

        constexpr char32_t operator*() const noexcept {
            return buffer[idx].c;
        };

        constexpr iterator& operator++() {
            advance();
            return *this;
        }

        constexpr void operator++(int) const requires std::ranges::input_range<V>{
            advance();
        }

        constexpr iterator operator++(int) const requires std::ranges::forward_range<V> {
            auto it = *this;
            advance();
            return it;
        }

        constexpr iterator& operator--() requires std::ranges::bidirectional_range<V> {
            back();
            return *this;
        }

        constexpr iterator operator--(int) const requires std::ranges::bidirectional_range<V> {
            auto it = *this;
            back();
            return it;
        }

        constexpr bool operator==(const iterator& other) const requires std::equality_comparable<base_it> {
            return other.base_ == base_ && other.idx == idx;
        }

        constexpr bool operator==(const std::default_sentinel_t &) const
            requires std::sentinel_for<std::ranges::sentinel_t<V>, base_it> {
            return base_ == parent->base.end() && idx == buffer.size();
        }
    };

    constexpr iterator begin() const {
        return {this, std::ranges::begin(base)};
    }

    constexpr std::default_sentinel_t end() const {
        return {};
    }

private:
    V base;
};

using x = normalization_view<normalization_form::nfd, std::u32string_view>;
static_assert(std::input_iterator<typename x::iterator>);
static_assert(std::ranges::input_range<x>);


//template <normalization_form Form, std::ranges::input_range V>
//normalization_view(V&&) -> normalization_view<Form, V>;

}
