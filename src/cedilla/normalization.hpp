#pragma once
#include <cedilla/details/generated_normalization.hpp>
#include <ranges>
#include <vector>

namespace cedilla {

enum class normalization_form {
    nfd
};

template <normalization_form Form, std::ranges::input_range V>
requires std::ranges::view<V>
       && std::is_same_v<std::remove_cvref_t<std::ranges::range_reference_t<V>>, char32_t>
    class normalization_view : public std::ranges::view_interface<normalization_view<Form, V>>
{

    constexpr normalization_view(V&& v): base(std::forward<V>(v)) {}

    struct iterator {
    private:
        using base_it = std::ranges::iterator_t<V>;
        base_it base_;
        normalization_view* parent;
        struct codepoint {
            char32_t c   : 21;
            uint8_t  ccc : 8 ;
        };
        std::vector<codepoint> buffer;
        std::size_t idx = 0;

        void decompose_next() {
            uint8_t last_ccc = 0;
            bool sorted = true;
            while(base_ != parent->end()) {
                codepoint c = {*base_, details::generated::ccc_data.lookup(c)};
                bool is_nfd = !details::generated::nfd_qc_no.lookup(c);
                if(is_nfd && c.ccc  == 0) {
                    break;
                }
                if(c.ccc != 0 && c.ccc < last_ccc) {
                    sorted = false;
                }
                if(!is_nfd) {
                    // find mapping
                }
            }
        }

        void advance() {
            if(idx < buffer.size()) {
                idx++;
                return;
            }
            buffer.clear();
            idx = 0;
            base_++;
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
        constexpr iterator(base_it base): base_(base) {};

        constexpr const base_it& base() const & noexcept {
            return base_;
        }

        constexpr base_it base() && {
            return std::move(base_);
        }

        constexpr char32_t operator*() const noexcept {
            return buffer[idx];
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

        constexpr bool operator==(const std::default_sentinel_t & sentinel) const
            requires std::sentinel_for<std::ranges::sentinel_t<V>, base_it> {
            return base_ == sentinel && idx == buffer.size();
        }
    };

    constexpr iterator begin() const {
        return std::ranges::begin(base);
    }

    constexpr std::default_sentinel_t end() const {
        return {};
    }

private:
    V base;
};


}
