#pragma once
#include <cedilla/details/generated_props.hpp>
#include <cedilla/utf.hpp>
#include <cedilla/word_break.hpp>
#include <span>
#include <ranges>

namespace cedilla {

namespace details {

    struct casing_data {
        std::span<const char32_t[2]> data;
        std::span<const std::size_t> start_indexes;
        std::size_t length;
    };

    enum case_transformation_kind : unsigned char {
        case_transformation_upper,
        case_transformation_lower,
        case_transformation_title,
        case_transformation_fold,
    };

    constexpr char32_t mapping_sentinel = 0xffffffff;

    constexpr inline casing_data casing_datas[] = {
        {details::generated::casing_data_uc, details::generated::casing_starts_uc,
         details::generated::casing_max_len_uc},
        {details::generated::casing_data_lc, details::generated::casing_starts_lc,
         details::generated::casing_max_len_lc},
        {details::generated::casing_data_tc, details::generated::casing_starts_tc,
         std::max(details::generated::casing_max_len_tc, details::generated::casing_max_len_lc)},
        {details::generated::casing_data_cf, details::generated::casing_starts_cf,
         details::generated::casing_max_len_cf}};

    template<case_transformation_kind CTK, bool is_title = false>
    constexpr auto get_case_maping(char32_t c) {
        constexpr const casing_data& d = casing_datas[CTK];
        constexpr std::size_t buffer_size = is_title? casing_datas[case_transformation_title].length : d.length;
        std::array<char32_t, buffer_size> out;
        std::ranges::fill(out, mapping_sentinel);
        for(std::size_t i = 0; i < d.length; i++) {
            auto start = d.data.begin() + d.start_indexes[i];
            auto end = d.data.begin() + d.start_indexes[i + 1];
            auto it = std::lower_bound(start, end, c,
                                       [](const char32_t(&mapping)[2], const char32_t probe) {
                                           return mapping[0] < probe;
                                       });
            if((it == end || (*it)[0] != c) && i == 0) {
                out[i] = c;
                break;
            }
            if(it == end)
                break;
            out[i] = (*it)[1] & ~(1 << 31);
            if(!((*it)[1] & (1 << 31)))
                [[likely]] break;
        }
        return out;
    }

    template<std::ranges::input_range V>
    auto casing_view_iterator_concept() {
        if constexpr(std::ranges::bidirectional_range<V>)
            return std::bidirectional_iterator_tag{};
        if constexpr(std::ranges::forward_range<V>)
            return std::forward_iterator_tag{};
        return std::input_iterator_tag{};
    }

    template<std::ranges::input_range V, case_transformation_kind CTK>
    requires std::ranges::view<V> class casing_view
        : public std::ranges::view_interface<casing_view<V, CTK>> {
        V base_;

    static constexpr bool is_title = CTK == case_transformation_title;

    public:
        class iterator {
            using BaseIt = std::ranges::iterator_t<V>;
            struct empty_iterator {
                empty_iterator(const BaseIt&){}
                empty_iterator() = default;
            };
            using WordBoundary = std::conditional_t<is_title, BaseIt, empty_iterator>;


            static constexpr const casing_data& d = casing_datas[CTK];
            using buffer = std::array<char32_t, d.length>;
            buffer b_;
            std::size_t pos = 0;
            const casing_view* parent;
            [[no_unique_address]] WordBoundary word_start;
            [[no_unique_address]] WordBoundary word_end;
            BaseIt base_;


            void advance() {
                pos++;
                if(pos >= b_.size() || b_[pos] == mapping_sentinel) {
                    base_++;
                    pos = 0;
                    if(base_ != std::ranges::end(parent->base_)) {
                        if constexpr(is_title) {
                            if(word_end == base_) {
                                word_start = word_end;
                                word_end   = find_word_end(word_start, std::ranges::end(parent->base_));
                            }
                            if(base_ == word_start) {
                                b_ = get_case_maping<case_transformation_title>(*base_);
                            }
                            else {
                                b_ = get_case_maping<case_transformation_lower, /*is title*/true>(*base_);
                            }
                        }
                        else {
                            b_ = get_case_maping<CTK>(*base_);
                        }
                    }
                }
            }

            void back() {
                if(pos == 0) {
                    --base_;
                    b_ = get_case_maping<CTK>(*base_);
                    for(pos = d.length - 1;; --pos) {
                        if(b_[pos] != mapping_sentinel)
                            break;
                    }
                } else {
                    --pos;
                }
            }

        public:
            using value_type = char32_t;
            using reference = char32_t;
            using difference_type = std::ranges::range_difference_t<V>;
            using iterator_category = decltype([] {
                if constexpr(std::ranges::bidirectional_range<V>)
                    return std::bidirectional_iterator_tag{};
                else if constexpr(std::ranges::forward_range<V>)
                    return std::forward_iterator_tag{};
                else
                    return std::input_iterator_tag{};
            }());

            constexpr iterator() requires std::default_initializable<BaseIt> = default;
            constexpr iterator(const casing_view* p, BaseIt it) noexcept : parent(p),
                word_start(it), word_end(it), base_(std::move(it)) {

                if(base_ != std::ranges::end(parent->base_)) {
                    if constexpr(is_title) {
                        word_end = find_word_end(it, std::ranges::end(parent->base_));
                    }
                    b_ = get_case_maping<CTK>(*base_);
                }
            }

            constexpr const BaseIt& base() const& noexcept {
                return base_;
            }

            constexpr BaseIt base() && {
                return std::move(base_);
            }

            constexpr char32_t operator*() const noexcept {
                return b_[pos];
            };

            constexpr iterator& operator++() {
                advance();
                return *this;
            }

            constexpr void operator++(int) requires std::ranges::input_range<V> {
                advance();
            }

            constexpr iterator operator++(int) requires std::ranges::forward_range<V> {
                auto it = *this;
                advance();
                return it;
            }

            constexpr iterator& operator--() requires std::ranges::bidirectional_range<V> {
                back();
                return *this;
            }

            constexpr iterator operator--(int) requires std::ranges::bidirectional_range<V> {
                auto it = *this;
                back();
                return it;
            }

            constexpr bool operator==(const iterator& other) const
                requires std::equality_comparable<BaseIt> {
                return other.base_ == base_ && other.pos == pos;
            }

            constexpr bool operator==(const std::default_sentinel_t& sentinel) const
                requires std::sentinel_for<std::ranges::sentinel_t<V>, BaseIt> {
                return base_ == sentinel && pos == mapping_sentinel;
            }
        };


        constexpr casing_view() requires std::default_initializable<V> = default;
        constexpr casing_view(V v) : base_(std::move(v)) {}
        constexpr V base() const& {
            return base_;
        }
        constexpr V base() && {
            return std::move(base_);
        }

        constexpr iterator begin() const {
            return iterator(this, std::ranges::begin(base_));
        }

        constexpr std::default_sentinel_t end() const {
            return {};
        }

        constexpr iterator end() const requires std::ranges::common_range<V> {
            return {this, std::ranges::end(base_)};
        }
    };
    template<std::ranges::range R, case_transformation_kind CTK>
    casing_view(R &&) -> casing_view<std::views::all_t<R>, CTK>;

template <case_transformation_kind CTK>
struct casing_view_fn {
    template <std::ranges::input_range R>
    requires std::is_same_v<std::remove_cvref_t<std::ranges::range_reference_t<R>>, char32_t>
    constexpr auto operator()(R && r) const {
        using T = std::views::all_t<R>;
        return casing_view<T, CTK>{T(r)};
    }

    template <std::ranges::input_range R>
    requires std::is_same_v<std::remove_cvref_t<std::ranges::range_reference_t<R>>, char32_t>
    constexpr friend auto operator|(R &&r, const casing_view_fn &) {
        using T = std::views::all_t<R>;
        return casing_view<T, CTK>{T(r)};
    }

    template <std::ranges::input_range R, typename __CT = std::remove_cvref_t<std::ranges::range_reference_t<R>>>
    requires std::is_same_v<__CT, char8_t> || std::is_same_v<__CT, char16_t>
        constexpr auto operator()(R && r) const {
        using T = std::views::all_t<R>;
        auto cv = codepoint_view(T(r));
        casing_view<decltype(cv), CTK> casing(cv);
        return codeunit_view<decltype(casing), __CT, /*implicit*/true>(casing);
    }

    template <std::ranges::input_range R, typename __CT = std::remove_cvref_t<std::ranges::range_reference_t<R>>>
    requires std::is_same_v<__CT, char8_t> || std::is_same_v<__CT, char16_t>
    constexpr friend auto operator|(R && r, const casing_view_fn &) {
        using T = std::views::all_t<R>;
        codepoint_view<T> cv(std::views::all(r));
        casing_view<decltype(cv), CTK> casing(std::move(cv));
        return codeunit_view<decltype(casing), __CT, /*implicit*/true>(std::move(casing));
    }

    template <std::ranges::input_range R, details::utf_code_unit CodeUnitType>
    constexpr auto operator()(cedilla::codeunit_view<R, CodeUnitType, /*implicit*/true> && r) const {
        using T = std::views::all_t<R>;
        return codeunit_view<casing_view<T, CTK>, CodeUnitType, /*implicit*/true>{casing_view<T, CTK>{T(std::move(r).base())}};
    }

    template <std::ranges::input_range R, details::utf_code_unit CodeUnitType>
    constexpr friend auto operator|(cedilla::codeunit_view<R, CodeUnitType, /*implicit*/true> && r,
                                        const casing_view_fn &) {
        using T = std::views::all_t<R>;
        return codeunit_view<casing_view<T, CTK>, CodeUnitType, /*implicit*/true>{casing_view<T, CTK>{T(std::move(r).base())}};
    }
};

}


constexpr inline details::casing_view_fn<details::case_transformation_upper> upper;
constexpr inline details::casing_view_fn<details::case_transformation_lower> lower;
constexpr inline details::casing_view_fn<details::case_transformation_fold>  case_fold;
constexpr inline details::casing_view_fn<details::case_transformation_title> title;

template<std::ranges::input_range V>
using uppercase_default_view = details::casing_view<V, details::case_transformation_upper>;

template<std::ranges::input_range V>
using lowercase_default_view = details::casing_view<V, details::case_transformation_lower>;

template<std::ranges::input_range V>
using casefold_default_view = details::casing_view<V, details::case_transformation_fold>;

template<std::ranges::input_range V>
using titlecase_default_view = details::casing_view<V, details::case_transformation_title>;

}    // namespace cedilla
