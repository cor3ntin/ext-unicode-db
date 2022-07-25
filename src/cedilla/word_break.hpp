#pragma once
#include <cedilla/details/generated_props.hpp>
#include <span>
#include <ranges>

namespace cedilla {

namespace details {

    enum class word_break : uint8_t {
        any,
        cr,
        double_quote,
        extended_num_let,
        extend,
        format,
        hebrew_letter,
        katakana,
        aletter,
        lf,
        midnumlet,
        midletter,
        midnum,
        newline,
        numeric,
        regional_indicator,
        single_quote,
        wseg_space,
        extended_pictographic,
        zwj
    };

    constexpr inline word_break word_break_property(char32_t cp) {
        // word_break_data does not contain alphabetic characters with the "aletter" value
        auto prop = static_cast<word_break>(generated::word_break_data.lookup(cp));
        if(prop == word_break::any && is_alpha(cp))
            return word_break::aletter;
        return prop;
    }

    enum class word_break_kind {
        nobreak,
        always_break,
        punct_after_ahletter,
        dq_after_hebrew,
        punct_after_numeric,
        regional_indicator,
        extend,
        ignore
    };

    constexpr inline word_break_kind pairwise_word_break_kind(word_break a, word_break b,
                                                              word_break previous,
                                                              bool with_ignored_extended = false) {
        using wb = word_break;
        using k = word_break_kind;

        auto AHLetter = [](word_break w) { return w == wb::aletter || w == wb::hebrew_letter; };
        if(!with_ignored_extended) {
            // WB3. CR × LF
            if(a == wb::cr && b == wb::lf)
                return k::nobreak;

            // WB3a. (Newline | CR | LF) ÷ any
            if((a == wb::lf || a == wb::cr || a == wb::newline))
                return k::always_break;

            // WB3b. any ÷ (Newline | CR | LF)
            if((b == wb::lf || b == wb::cr || b == wb::newline))
                return k::always_break;

            // WB3c. ZWJ × \p{Extended_Pictographic}
            if(a == wb::zwj && b == wb::extended_pictographic)
                return k::nobreak;

            // WB3d. WSegSpace × WSegSpace
            if(a == wb::wseg_space && b == wb::wseg_space)
                return k::nobreak;

            // WB4. any × (Format | Extend | ZWJ)
            if(a == wb::zwj || a == wb::extend || a == wb::format)
                return k::ignore;
        }

        // WB4. any × (Format | Extend | ZWJ)
        if(b == wb::zwj || b == wb::extend || b == wb::format)
            return k::extend;

        // WB5. AHLetter × AHLetter
        if(AHLetter(a) && AHLetter(b))
            return k::nobreak;

        // WB7a. Hebrew_Letter ×	Single_Quote
        if(a == wb::hebrew_letter && b == wb::single_quote)
            return k::nobreak;

        // WB6.	AHLetter × (MidLetter | MidNumLetQ) AHLetter
        if(AHLetter(a) && (b == wb::midnumlet || b == wb::single_quote || b == wb::midletter))
            return k::punct_after_ahletter;

        // WB7.	AHLetter (MidLetter | MidNumLetQ) × AHLetter
        if(AHLetter(previous) &&
           (a == wb::midnumlet || a == wb::single_quote || a == wb::midletter) && AHLetter(b))
            return k::nobreak;

        // WB7b. Hebrew_Letter ×	Double_Quote Hebrew_Letter
        if(a == wb::hebrew_letter && b == wb::double_quote)
            return k::dq_after_hebrew;

        // WB7c. Hebrew_Letter Double_Quote × Hebrew_Letter
        if(a == wb::double_quote && previous == wb::hebrew_letter && b == wb::hebrew_letter)
            return k::nobreak;

        // WB8. Numeric × Numeric
        // WB9. AHLetter × Numeric
        // WB10. Numeric × AHLetter
        if((AHLetter(a) || a == wb::numeric) && (AHLetter(b) || b == wb::numeric))
            return k::nobreak;

        // WB11. Numeric (MidNum | MidNumLetQ) × Numeric
        if((a == wb::midnumlet || a == wb::single_quote || a == wb::midnum) &&
           previous == wb::numeric && b == wb::numeric)
            return k::nobreak;

        // WB12. Numeric × (MidNum | MidNumLetQ) Numeric
        if(a == wb::numeric && (b == wb::midnumlet || b == wb::single_quote || b == wb::midnum))
            return k::punct_after_numeric;

        // WB13. Katakana × Katakana
        if(a == wb::katakana && b == wb::katakana)
            return k::nobreak;

        // WB13a (AHLetter | Numeric | Katakana | ExtendNumLet)	× ExtendNumLet
        if(b == wb::extended_num_let && (AHLetter(a) || a == wb::numeric || a == wb::katakana ||
                                         a == word_break::extended_num_let))
            return k::nobreak;

        // WB13b ExtendNumLet × (AHLetter | Numeric | Katakana | ExtendNumLet)
        if(a == wb::extended_num_let && (AHLetter(b) || b == wb::numeric || b == wb::katakana ||
                                         b == word_break::extended_num_let))
            return k::nobreak;

        // WB15. WB16.  RI x RI
        if(a == wb::regional_indicator && b == wb::regional_indicator)
            return k::regional_indicator;

        return k::always_break;
    }


}    // namespace details


template<std::forward_iterator It, std::sentinel_for<It> End>
constexpr auto find_word_end(It current, End end) {
    if(current == end)
        return current;

    using k = details::word_break_kind;
    using wb = details::word_break;
    std::size_t RI = 0;
    It not_extended = current;

    auto next = std::next(current);
    auto current_break_prop = details::word_break_property(*current);
    auto next_break_prop = details::word_break_property(*next);

    details::word_break_kind break_kind = details::word_break_kind::always_break;
    details::word_break_kind previous_kind = details::word_break_kind::always_break;
    details::word_break previous_prop = details::word_break::any;
    details::word_break prevprev = details::word_break::any;

    for(; current != end; next++, current++, current_break_prop = next_break_prop) {
        if(current_break_prop != wb::extend && current_break_prop != wb::zwj &&
           current_break_prop != wb::format)
            not_extended = current;

        if(next == end)
            break;

        next_break_prop = details::word_break_property(*next);

        if(next_break_prop != wb::extend && next_break_prop != wb::format &&
           next_break_prop != wb::zwj) {
            if(previous_kind == k::punct_after_ahletter) {
                if(next_break_prop != wb::aletter && next_break_prop != wb::hebrew_letter)
                    return not_extended;
            } else if(previous_kind == k::punct_after_numeric) {
                if(next_break_prop != wb::numeric)
                    return not_extended;
            } else if(previous_kind == k::dq_after_hebrew) {
                if(next_break_prop != wb::hebrew_letter)
                    return not_extended;
            }
        }

        details::word_break_kind true_break_kind =
            details::pairwise_word_break_kind(current_break_prop, next_break_prop, previous_prop);
        if(true_break_kind == k::ignore) {
            break_kind = details::pairwise_word_break_kind(
                details::word_break_property(*not_extended), next_break_prop, prevprev,
                /*with_ignored_extended*/ true);
        } else {
            break_kind = true_break_kind;
            prevprev = previous_prop;
            previous_prop = current_break_prop;
        }

        if(RI != 0 && break_kind == k::regional_indicator) {
            return (RI % 2 == 0) ? next : current;
        }

        switch(break_kind) {
            case k::always_break: return next;
            case k::ignore:
            case k::extend: continue;
            case k::punct_after_ahletter:
            case k::punct_after_numeric:
            case k::dq_after_hebrew:
            case k::nobreak: previous_kind = break_kind; continue;
            case k::regional_indicator:
                RI = RI == 0 ? 2 : RI + 1;
                previous_kind = break_kind;
                continue;
        }
    }
    if(previous_kind == k::punct_after_ahletter || previous_kind == k::punct_after_numeric ||
       previous_kind == k::dq_after_hebrew)
        return not_extended;

    return next;
}


template<std::ranges::forward_range V>
requires std::ranges::view<V>&& std::is_same_v<
    std::remove_cvref_t<std::ranges::range_reference_t<V>>, char32_t> class words_view
    : public std::ranges::view_interface<V> {

    V base_;

public:
    class outer_iterator {
        using BaseIt = std::ranges::iterator_t<V>;
        const words_view* parent;
        BaseIt start;
        BaseIt next;
        constexpr void advance() {
            start = next;
            next = find_word_end(start, parent->base_.end());
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
        constexpr outer_iterator(const words_view* p, BaseIt it) noexcept : parent(p), start(it) {
            next = start == parent->base_.end() ? start : find_word_end(start, parent->base_.end());
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
            // back();
            return *this;
        }

        constexpr outer_iterator operator--(int) const
            requires std::ranges::bidirectional_range<V> {
            auto it = *this;
            // back();
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


    constexpr words_view() requires std::default_initializable<V> = default;
    constexpr words_view(V v) : base_(std::forward<V>(v)) {}

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
words_view(R &&) -> words_view<std::views::all_t<R>>;

}    // namespace cedilla
