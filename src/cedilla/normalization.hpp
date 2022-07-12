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

inline std::array<char32_t, 2> get_canonical_decomposition(char32_t c) {
    if(c <= 0xffff) {
        const auto & a = details::generated::canonical_decomposition_mapping_small_keys;
        auto it = details::branchless_lower_bound(a, c);
        if(it != std::ranges::end(a) && *it == c) {
            const auto n = 2 * std::ranges::distance(std::ranges::begin(a), it);
            const auto & v =   details::generated::canonical_decomposition_mapping_small_values;
            return {v[n], v[n+1]};
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

        void decompose_pair(const std::array<char32_t, 2> & canonical, uint8_t& last_ccc, bool& sorted) {
            assert(canonical[0] != 0 && "No canonical composition for a cp with nfd_qc == NO");
            for(std::size_t N = 0; N < 2; N++) {
                uint8_t ccc = details::generated::ccc_data.lookup(canonical[N]);;
                if(!canonical[N])
                    break;
                if(details::generated::nfd_qc_no.lookup(canonical[N])) {
                    decompose_recursively(canonical[N], last_ccc, sorted);
                }
                else {
                    buffer.emplace_back(canonical[N], ccc);
                }
            }
        }

        void decompose_recursively(char32_t c, uint8_t& last_ccc, bool& sorted) {
            assert(details::generated::nfd_qc_no.lookup(c)
                   && "called decompose_recursively on a cp with nfd_qc == NO");
            std::array<char32_t, 2> canonical = get_canonical_decomposition(c);
            decompose_pair(canonical, last_ccc, sorted);
        }
        static constexpr bool is_canonical() {
            return Form == normalization_form::nfd || Form == normalization_form::nfc;
        }

        constexpr void canonical_sort() {
            std::ranges::sort(buffer, [](const codepoint & a, const codepoint & b) {
                if(a.ccc == 0 || b.c == 0)
                    return false;
                return a.ccc < b.ccc;
            });
        }

        constexpr void decompose_next() {
            uint8_t last_ccc = 0;
            bool sorted = true;
            while(base_ != parent->base.end()) {
                auto n = *(base_);
                codepoint c = {n, (uint8_t)details::generated::ccc_data.lookup(n)};
                bool is_nfd = !details::generated::nfd_qc_no.lookup(n);
                if(is_nfd && c.ccc == 0 && !buffer.empty()) {
                    break;
                }
                base_ ++;
                if(c.ccc != 0 && c.ccc < last_ccc) {
                    sorted = false;
                }
                if(is_nfd) {
                    buffer.emplace_back(c);
                    continue;
                }
                std::array<char32_t, 2> canonical = get_canonical_decomposition(c.c);
                decompose_pair(canonical, last_ccc, sorted);
            }
            canonical_sort();
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
