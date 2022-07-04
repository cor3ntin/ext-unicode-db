#include <cedilla/details/generated_props.hpp>
#include <ranges>

namespace cedilla {

using script = details::generated::script;

namespace details {

struct script_extensions_view {
    constexpr script_extensions_view(char32_t) noexcept;
    struct iterator {
        using iterator_category = std::forward_iterator_tag;
        using value_type = script;
        using pointer = value_type*;
        using reference = value_type;
        using difference_type = std::ptrdiff_t;

        constexpr iterator() noexcept = default;
        constexpr iterator(char32_t c) noexcept;
        constexpr script operator*() const noexcept;

        constexpr iterator operator++(int) noexcept;

        constexpr iterator& operator++() noexcept;

        constexpr bool operator==(std::default_sentinel_t) const noexcept;
        constexpr bool operator!=(std::default_sentinel_t) const noexcept;

    private:
        char32_t m_c;
        script m_script;
        int idx = 1;
    };

    constexpr iterator begin() const noexcept;
    constexpr std::default_sentinel_t end() const noexcept;

private:
    char32_t c;
};

static_assert(std::input_iterator<script_extensions_view::iterator>);
static_assert(std::ranges::input_range<script_extensions_view>);

constexpr script_extensions_view::script_extensions_view(char32_t c_) noexcept : c(c_) {}

constexpr script_extensions_view::iterator::iterator(char32_t c_) noexcept :
    m_c(c_), m_script(generated::get_script_ext(c_, 0)) {
}

constexpr script script_extensions_view::iterator::operator*() const noexcept {
    return m_script;
}

constexpr script_extensions_view::iterator script_extensions_view::iterator::operator ++(int) noexcept {
    auto c = *this;
    idx++;
    m_script = generated::get_script_ext(m_c, idx);
    return c;
}

constexpr auto script_extensions_view::iterator::operator++() noexcept -> iterator& {
    idx++;
    m_script = generated::get_script_ext(m_c, idx);
    return *this;
}

constexpr bool script_extensions_view::iterator::operator==(std::default_sentinel_t) const noexcept{
    return m_script == script::unknown;
}

constexpr bool script_extensions_view::iterator::operator!=(std::default_sentinel_t) const noexcept{
    return m_script != script::unknown;
}

constexpr script_extensions_view::iterator script_extensions_view::begin() const noexcept {
    return iterator{c};
}
constexpr std::default_sentinel_t script_extensions_view::end() const noexcept {
    return {};
}

} // namepace details

constexpr script cp_script(char32_t cp)  noexcept {
    return static_cast<script>(details::generated::script_properties<0>.lookup(cp, uint16_t(script::unknown)));
}

constexpr details::script_extensions_view cp_script_extensions(char32_t cp) noexcept {
    return {cp};
}

}
