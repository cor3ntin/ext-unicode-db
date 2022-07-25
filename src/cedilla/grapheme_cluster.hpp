#pragma once
#include <cedilla/details/generated_props.hpp>
#include <cedilla/details/hangul.hpp>
#include <span>
#include <ranges>

namespace cedilla {

namespace details {

// keep it sync with generator
enum class grapheme_cluster_break : uint8_t{
    any,

    hangul_l   = uint8_t(hangul_syllable_kind::leading),
    hangul_v   = uint8_t(hangul_syllable_kind::vowel),
    hangul_t   = uint8_t(hangul_syllable_kind::trailing),
    hangul_lv  = hangul_l | hangul_v,
    hangul_lvt = hangul_lv | hangul_t,

    control    = 0x10,
    prepend    = 0x20,
    extend     = 0x30,
    regional_indicator = 0x40,
    extended_pictographic = 0x50,
    spacing_mark = 0x60,
    cr  = 0x70,
    lf  = 0x80,
    zwj = 0x90
};

constexpr inline grapheme_cluster_break grapheme_cluster_break_property(char32_t cp) {
    if(is_hangul(cp)) [[unlikely]] {
        return static_cast<grapheme_cluster_break>(hangul_syllable_type(cp));
    }
    if(cp > 0x1F && cp <= 0x7F) // fast ascii path
        return grapheme_cluster_break::any;
    return static_cast<grapheme_cluster_break>(generated::grapheme_break_data.lookup(cp));
}

enum class grapheme_breakpoint_kind {
    nobreak,
    always_break,
    emoji_start,
    extend,
    emoji_end,
    regional
};

constexpr inline grapheme_breakpoint_kind pairwise_grapheme_breakpoint(grapheme_cluster_break a,
                                             grapheme_cluster_break b) {
    using g = grapheme_cluster_break;
    using k = grapheme_breakpoint_kind;
    if(b == g::lf) {
         // GB3. CR X LF
        return a == g::cr ? k::nobreak : k::always_break;
    }
    // GB4. (Control|CR|LF) ÷
    if(a == g::cr || a == g::lf || a == g::control)
        return k::always_break;
    // GB5. ÷ (Control|CR|LF)
    if(b == g::cr || b == g::control)
        return k::always_break;

    // GB6. L X (L|V|LV|LVT)
    if(a == g::hangul_l && ( uint8_t(b) & uint8_t(g::hangul_l) || uint8_t(b) & uint8_t(g::hangul_v)))
        return k::nobreak;
    // GB7. (LV|V) X (V|T)
    if((a == g::hangul_lv || a == g::hangul_v) && (b == g::hangul_v || b == g::hangul_t))
        return k::nobreak;
    // GB8. (LVT|T) X (T)
    if((a == g::hangul_lvt || a == g::hangul_t) &&  b == g::hangul_t)
        return k::nobreak;

    // GB9. any × (Extend | ZWJ)
    // We use a distinct value here for the emoji state machine
    if(a == g::extend && b == g::extend)
        return k::extend;

    // GB11. \p{Extended_Pictographic} Extend* ZWJ × \p{Extended_Pictographic}
    if(a == g::extended_pictographic && (b == g::extend || b == g::zwj))
        return k::emoji_start;

    // GB9. any × (Extend | ZWJ)
    if(b == g::zwj || b == g::extend)
        return k::nobreak;

    // GB9a. any × SpacingMark
    if(b == g::spacing_mark)
        return k::nobreak;
    // GB9b. Prepend × any
    if(a == g::prepend)
        return k::nobreak;

    // GB12. GB13.  RI x RI
    if(a == g::regional_indicator && b == g::regional_indicator)
        return k::regional;

    // GB11. \p{Extended_Pictographic} Extend* ZWJ × \p{Extended_Pictographic}
    if(a == g::zwj && b == g::extended_pictographic)
        return k::emoji_end;

    return k::always_break;
}


constexpr inline grapheme_breakpoint_kind pairwise_grapheme_breakpoint(char32_t first_cp, char32_t second_cp) {
    return pairwise_grapheme_breakpoint(grapheme_cluster_break_property(first_cp),
                                  grapheme_cluster_break_property(second_cp));
}

}


template <std::forward_iterator It, std::sentinel_for<It> End>
constexpr auto find_grapheme_cluster_end(It current, End end) {
    using k = details::grapheme_breakpoint_kind;
    std::size_t RI = 0;
    bool in_emoji = false;
    auto next = std::next(current);
    for(; next != end; next++, current++) {
        auto break_kind = details::pairwise_grapheme_breakpoint(*current, *next);
        if(RI != 0 && break_kind == k::regional) {
            return (RI % 2 == 0) ? next : current;
        }
        switch(break_kind) {
            case k::emoji_start:
                in_emoji = true;
                continue;
            case k::always_break:
                return next;
            case k::extend:
                continue;
            case k::nobreak:
                continue;
            case k::regional:
                RI = RI == 0 ? 2 : RI + 1;
                continue;
            case k::emoji_end:
                if(!in_emoji)
                    return next;
                continue;
        }
    }
    return next;
}


template<std::ranges::forward_range V>
requires std::ranges::view<V>
    && std::is_same_v<std::remove_cvref_t<std::ranges::range_reference_t<V>>, char32_t>
class grapheme_clusters_view : public std::ranges::view_interface<V> {

    V base_;

public:
    class outer_iterator {
        using BaseIt = std::ranges::iterator_t<V>;
        const grapheme_clusters_view* parent;
        BaseIt start;
        BaseIt next;
        constexpr void advance() {
            start = next;
            next  = find_grapheme_cluster_end(start, parent->base_.end());
        }

    public:
        using value_type = std::ranges::subrange<BaseIt>;
        using difference_type = std::ranges::range_difference_t<V>;
        using iterator_category = decltype([] {
            if constexpr(std::ranges::bidirectional_range<V>)
                return std::bidirectional_iterator_tag{};
            else
                return std::forward_iterator_tag{};
        }());

        constexpr outer_iterator() requires std::default_initializable<BaseIt> = default;
        constexpr outer_iterator(const grapheme_clusters_view* p, BaseIt it) noexcept
            : parent(p), start(it) {
            next = find_grapheme_cluster_end(start, parent->base_.end());
        }

        constexpr const BaseIt& base() const& noexcept {
            return start;
        }

        constexpr BaseIt base() && {
            return std::move(start);
        }

        constexpr value_type operator*() const noexcept {
            return {start, next};
        };

        constexpr outer_iterator& operator++() {
            advance();
            return *this;
        }

        constexpr outer_iterator operator++(int) const requires std::ranges::forward_range<V> {
            auto it = *this;
            advance();
            return it;
        }

        constexpr outer_iterator& operator--() requires std::ranges::bidirectional_range<V> {
            //back();
            return *this;
        }

        constexpr outer_iterator operator--(int) const requires std::ranges::bidirectional_range<V> {
            auto it = *this;
            //back();
            return it;
        }

       constexpr bool operator==(const outer_iterator& other) const
            requires std::equality_comparable<BaseIt> {
            return start == other.start;
        }

        constexpr bool operator==(const std::default_sentinel_t& sentinel) const
            requires std::sentinel_for<std::ranges::sentinel_t<V>, BaseIt> {
            return start == sentinel;
        }
    };


    constexpr grapheme_clusters_view() requires std::default_initializable<V> = default;
    constexpr grapheme_clusters_view(V v) : base_(std::forward<V>(v)) {}

    constexpr V base() const& requires std::copy_constructible<V> {
        return base_;
    }
    constexpr V base() && {
        return std::move(base_);
    }

    constexpr outer_iterator begin() const {
        return {this, std::ranges::begin(base_)};
    }

    constexpr std::default_sentinel_t end() const {
        return {};
    }

    constexpr outer_iterator end() const requires std::ranges::common_range<V> {
        return {this, std::ranges::end(base_)};
    }
};

template<std::ranges::forward_range R>
grapheme_clusters_view(R &&) -> grapheme_clusters_view<std::views::all_t<R>>;

}
