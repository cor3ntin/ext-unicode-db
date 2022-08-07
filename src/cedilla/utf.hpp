#pragma once
#include <ranges>

namespace cedilla {

namespace details {
    template <typename T>
    concept utf_code_unit =
           std::is_same_v<char8_t, T>
        || std::is_same_v<char16_t, T>
        || std::is_same_v<char32_t, T>;
    constexpr char32_t replacement_codepoint = 0xFFFD;

    //https://bjoern.hoehrmann.de/utf-8/decoder/dfa/
    constexpr const uint8_t forward_utf8_automaton[] = {
        // The first part of the table maps bytes to character classes that
        // to reduce the size of the transition table and create bitmasks.
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
        10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,
        // The second part is a transition table that maps a combination
        // of a state of the automaton and a character class to a state.
        0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
        12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
        12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
        12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
        12,36,12,12,12,12,12,12,12,12,12,12
    };
    constexpr uint8_t forward_utf8_automaton_invalid = 12;
    constexpr uint8_t forward_utf8_automaton_valid = 0;

    constexpr uint32_t decode(uint32_t & state, char32_t& cp, uint32_t byte) {
        uint32_t type = forward_utf8_automaton[byte];
        cp = (state != forward_utf8_automaton_valid) ? (byte & 0x3fu) | (cp << 6) : (0xff >> type) & (byte);
        state = forward_utf8_automaton[256 + state + type];
        return state;
    }

    // https://gershnik.github.io/2021/03/24/reverse-utf8-decoding.html
    static constexpr const uint8_t backward_utf8_automaton[] = {
        // The first part of the table maps bytes to character classes that
        // to reduce the size of the transition table and create bitmasks.
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
        10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

        // The second part is a transition table that maps a combination
        // of a state of the automaton and a character class to a state.
        //   0  1  2  3  4  5  6  7  8  9 10 11
        0,24,12,12,12,12,12,24,12,24,12,12,
        0,24,12,12,12,12,12,24,12,24,12,12,
        12,36, 0,12,12,12,12,48,12,36,12,12,
        12,60,12, 0, 0,12,12,72,12,72,12,12,
        12,60,12, 0,12,12,12,72,12,72, 0,12,
        12,12,12,12,12, 0, 0,12,12,12,12,12,
        12,12,12,12,12,12,12,12,12,12,12, 0
    };
    static constexpr uint8_t backward_utf8_automaton_valid = 0;
    static constexpr uint8_t backward_utf8_automaton_invalid = 12;
    constexpr uint32_t backward_decode(uint32_t & state, char32_t& cp, uint8_t& shift, uint32_t byte) {
        uint32_t type = backward_utf8_automaton[byte];
        state = backward_utf8_automaton[256 + state + type];
        if(state <= backward_utf8_automaton_invalid) {
            cp |= (((0xffu >> type) & (byte)) << shift);
            shift = 0;
        }
        else {
            cp |= ((byte & 0x3fu) << shift);
            shift += 6;
        }
        return state;
    }
}

template<std::ranges::input_range V>
requires std::ranges::view<V> && details::utf_code_unit<std::remove_cvref_t<std::ranges::range_reference_t<V>>>
class codepoint_view
    : public std::ranges::view_interface<codepoint_view<V>> {
    V base_;

public:
    class iterator {
        using BaseIt = std::ranges::iterator_t<V>;
        using __code_unit_type = std::remove_cvref_t<std::ranges::range_reference_t<V>>;
        const codepoint_view* parent;
        BaseIt base_;
        char32_t cp = details::replacement_codepoint;
        uint32_t n  = 0;

        void do_advance()
            requires std::same_as<__code_unit_type, char8_t>  {
            cp = details::replacement_codepoint;
            uint32_t state = details::forward_utf8_automaton_valid;
            for(;base_ != std::ranges::end(parent->base()); base_++) {
                details::decode(state, cp, *base_);
                if(state == details::forward_utf8_automaton_valid) {
                    return;
                }
                if(state == details::forward_utf8_automaton_invalid) {
                    cp = details::replacement_codepoint;
                    while(base_ != std::ranges::end(parent->base()) && *base_ >= 0x80)
                        base_++;
                    return;
                }
            }
            if(base_ == std::ranges::end(parent->base()))
                cp = details::replacement_codepoint;
        }

        void do_advance() requires std::same_as<__code_unit_type, char8_t>
            && std::forward_iterator<BaseIt> {
            n = 1;
            cp = details::replacement_codepoint;
            uint32_t state = details::forward_utf8_automaton_valid;
            auto it = base_;
            for(; it != std::ranges::end(parent->base()); it++, n++) {
                details::decode(state, cp, *it);
                if(state == details::forward_utf8_automaton_valid) {
                    return;
                }
                if(state == details::forward_utf8_automaton_invalid) {
                    cp = details::replacement_codepoint;
                    while(it != std::ranges::end(parent->base()) && *it >= 0x80)
                        it++, n++;
                    return;
                }
            }
            if(it == std::ranges::end(parent->base()))
                cp = details::replacement_codepoint;
        }

        void do_back() requires std::is_same_v<__code_unit_type, char8_t> {
            uint8_t  shift = 0;
            uint32_t state = details::backward_utf8_automaton_valid;
            cp = 0;
            while(true) {
                uint32_t byte = *base_;
                state = details::backward_decode(state, cp, shift, byte);
                if(state == details::backward_utf8_automaton_valid)
                    return;
                if(state == details::backward_utf8_automaton_invalid) {
                    cp = details::replacement_codepoint;
                    while(base_ != std::ranges::begin(parent->base())) {
                        if(*base_ < 0x80)
                            break;
                        base_--;
                    }
                }
                if(base_-- == std::ranges::begin(parent->base()))
                    break;
            }
            if(base_ == std::ranges::begin(parent->base()))
                cp = details::replacement_codepoint;
        }

        void do_advance() requires std::same_as<__code_unit_type, char16_t> {
            cp = *base_;
            n  = 1;
            if(cp < 0xd800 || cp > 0xDFFF)
                return;
            if(base_+1 == std::ranges::end(parent->base())) {
                cp = details::replacement_codepoint;
                return;
            }
            uint16_t low = *(++base_+1);
            if(low < 0xD800 && low < 0xDFFF) {
                cp = details::replacement_codepoint;
                return;
            }
            cp = ((cp & 0x03FF) << 10 | (low & 0x3FF)) + 0x00010000;
        }

        void do_advance() requires std::same_as<__code_unit_type, char16_t> && std::forward_iterator<BaseIt> {
            cp = *base_;
            n  = 1;
            if(cp < 0xd800 || cp > 0xDFFF)
                return;
            if(base_+1 == std::ranges::end(parent->base())) {
                cp = details::replacement_codepoint;
                return;
            }
            uint16_t low = *(base_+1);
            n++;
            if(low < 0xD800 && low < 0xDFFF) {
                cp = details::replacement_codepoint;
                return;
            }
            cp = ((cp & 0x03FF) << 10 | (low & 0x3FF)) + 0x00010000;
        }

        void do_back() requires std::is_same_v<__code_unit_type, char16_t> {
            cp = *base_;
            if(cp < 0xd800 || cp > 0xdfff) {
                return;
            }
            if(base_ == std::ranges::begin(parent->base())) {
                cp = details::replacement_codepoint;
                return;
            }
            uint32_t high = *(--base_);
            if(cp < 0xD800 || cp > 0xDFFF) {
                cp = details::replacement_codepoint;
                return;
            }
            cp = (((high & 0x03FF) << 10) | ((cp & 0x3FF))) + 0x00010000;
        }


        void do_advance() requires std::is_same_v<__code_unit_type, char32_t> {
            n = 1;
            cp = *base_;
        }

        void do_back() requires std::is_same_v<__code_unit_type, char32_t> {
            cp = *base_;
        }

        void advance() {
            base_ ++;
            do_advance();
        }

        void advance() requires std::forward_iterator<BaseIt> {
            base_ += n;
            do_advance();
        }

        void back() {
            if(base_ != std::ranges::begin(parent->base()))
                base_--;
            do_back();
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
        constexpr iterator(const codepoint_view* p, BaseIt it) noexcept : parent(p), base_(it) {
            if(base_ != std::ranges::end(parent->base())) {
                do_advance();
            }
        }

        constexpr const BaseIt& base() const& noexcept {
            return base_;
        }

        constexpr BaseIt base() && {
            return std::move(base_);
        }

        constexpr char32_t operator*() const noexcept {
            return cp;
        };

        constexpr iterator& operator++() {
            advance();
            return *this;
        }

        constexpr void operator++(int) const requires std::ranges::input_range<V> {
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

        constexpr bool operator==(const iterator& other) const
            requires std::equality_comparable<BaseIt> {
            return other.base_ == base_;
        }

        constexpr bool operator==(const std::default_sentinel_t& sentinel) const
            requires std::sentinel_for<std::ranges::sentinel_t<V>, BaseIt> {
            return base_ == sentinel;
        }
    };


    constexpr codepoint_view() requires std::default_initializable<V> = default;
    constexpr codepoint_view(V&& v) : base_(std::forward<V>(v)) {}
    constexpr V base() const& requires std::copy_constructible<V> {
        return base_;
    }
    constexpr V base() && {
        return std::move(base_);
    }

    constexpr iterator begin() const {
        return {this, std::ranges::begin(base_)};
    }

    constexpr std::default_sentinel_t end() const {
        return {};
    }

    constexpr iterator end() const requires std::ranges::common_range<V> {
        return {this, std::ranges::end(base_)};
    }
};

template<std::ranges::range R>
codepoint_view(R &&) -> codepoint_view<std::views::all_t<R>>;


template<std::ranges::input_range V, details::utf_code_unit CodeUnitType>
requires std::ranges::view<V> && std::is_same_v<char32_t, std::remove_cvref_t<std::ranges::range_reference_t<V>>>
class codeunit_view
    : public std::ranges::view_interface<codepoint_view<V>> {
    V base_;

public:
    class iterator {
        using BaseIt = std::ranges::iterator_t<V>;
        const codeunit_view* parent;
        BaseIt base_;
        CodeUnitType codeunits[4/sizeof(CodeUnitType)];
        uint8_t size  = 0;
        uint8_t pos   = 0;

        void do_advance()
            requires std::same_as<CodeUnitType, char8_t>  {
            char32_t c = *base_;
            if(c >= 0x110000 || (c >= 0xD800 && c < 0xDFFF))
                c = details::replacement_codepoint;
            if(c < 0x80) {
                size = 1;
                codeunits[0] = static_cast<char8_t>(c);
                return;
            }

            if(c < 0x800) {
                size = 2;
                codeunits[0] = static_cast<char8_t>(c >> 6) | 0xC0;
                codeunits[1] = static_cast<char8_t>(c & 0x3F) | 0x80;
                return;
            }

            if(c < 0x800) {
                size = 2;
                codeunits[0] = static_cast<char8_t>(c >> 6) | 0xC0;
                codeunits[1] = static_cast<char8_t>(c & 0x3F) | 0x80;
                return;
            }

            if(c < 0x10000) {
                size = 3;
                codeunits[0] = static_cast<char8_t>(c >> 12) | 0xE0;
                codeunits[1] = static_cast<char8_t>((c >> 6) & 0x3F)  | 0x80;
                codeunits[2] = static_cast<char8_t>(c & 0x3F) | 0x80;
                return;
            }

            size = 4;
            codeunits[0] = static_cast<char8_t>( c >> 18) | 0xF0;
            codeunits[1] = static_cast<char8_t>((c >> 12) & 0x3F) | 0x80;
            codeunits[2] = static_cast<char8_t>((c >> 6) & 0x3F)  | 0x80;
            codeunits[3] = static_cast<char8_t>(c & 0x3F) | 0x80;
        }

        void do_advance() requires std::same_as<CodeUnitType, char16_t> {
            char32_t c = *base_;
            if(c >= 0x110000 || (c >= 0xD800 && c < 0xDFFF))
                c = details::replacement_codepoint;

            if(c <= 0xFFFF) {
                size = 1;
                codeunits[0] = static_cast<char16_t>(c);
                return;
            }
            size = 2;
            c -= 0x0010000U;
            codeunits[0] = static_cast<char16_t>(c >> 10) + 0xD800;
            codeunits[1] = static_cast<char16_t>(c & 0x3FFU) + 0xDC00;
        }

        void do_advance() requires std::is_same_v<CodeUnitType, char32_t> {
            size = 1;
            codeunits[0] = *base_;
        }

        void advance() {
            pos++;
            if((size == 0 || pos == size) && base_ != std::ranges::end(parent->base())) {
                base_++;
                do_advance();
                pos = 0;
            }
        }

        friend class codeunit_view;

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
        constexpr iterator(const codeunit_view* p, BaseIt it) noexcept : parent(p), base_(it) {
            if(base_ != std::ranges::end(parent->base())) {
                do_advance();
            }
        }

        constexpr const BaseIt& base() const& noexcept {
            return base_;
        }

        constexpr BaseIt base() && {
            return std::move(base_);
        }

        constexpr CodeUnitType operator*() const noexcept {
            return codeunits[pos];
        };

        constexpr iterator& operator++() {
            advance();
            return *this;
        }

        constexpr void operator++(int) const requires std::ranges::input_range<V> {
            advance();
        }

        constexpr iterator operator++(int) const requires std::ranges::forward_range<V> {
            auto it = *this;
            advance();
            return it;
        }

        constexpr bool operator==(const iterator& other) const
            requires std::equality_comparable<BaseIt> {
            return other.base_ == base_ && other.pos == pos;
        }

        constexpr bool operator==(const std::default_sentinel_t& sentinel) const
            requires std::sentinel_for<std::ranges::sentinel_t<V>, BaseIt> {
            return base_ == sentinel && pos == size;
        }
    };


    constexpr codeunit_view() requires std::default_initializable<V> = default;
    constexpr codeunit_view(V&& v) : base_(std::forward<V>(v)) {}
    constexpr V base() const& requires std::copy_constructible<V> {
        return base_;
    }
    constexpr V base() && {
        return std::move(base_);
    }

    constexpr iterator begin() const {
        return {this, std::ranges::begin(base_)};
    }

    constexpr std::default_sentinel_t end() const {
        return {};
    }

    constexpr iterator end() const requires std::ranges::common_range<V> {
         auto it = iterator{this, std::ranges::end(base_)};
         return it;
    }
};

namespace details {

template <details::utf_code_unit CU>
struct codeunit_view_fn {
    template <std::ranges::input_range R>
    constexpr auto operator()(R && r) const {
        using T = std::views::all_t<R>;
        return codeunit_view<T, CU>{T(r)};
    }
    template <std::ranges::input_range R>
    constexpr friend auto operator|(R &&r, const codeunit_view_fn &) {
        using T = std::views::all_t<R>;
        return codeunit_view<T, CU>{T(r)};
    }
};
}

inline details::codeunit_view_fn<char8_t> to_utf8;
inline details::codeunit_view_fn<char16_t> to_utf16;


}
