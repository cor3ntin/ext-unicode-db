namespace uni {

namespace details {

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

struct name_view {
    constexpr name_view(char32_t c) : c(c){};

    struct sentinel {};
    struct iterator {

        using value_type = char;
        using iterator_category = std::forward_iterator_tag;

        constexpr iterator(char32_t c) : m_c(c), m_block(-1) {
            get_next_segment();
        }
        constexpr char32_t operator*() const {
            return m_str[m_str_pos];
        }

        constexpr iterator& operator++(int) {
            m_str_pos++;
            if(m_str_pos >= m_str.size()) {
                get_next_segment();
            }
            return *this;
        }
        constexpr iterator operator++() {
            auto c = *this;
            m_str_pos++;
            if(m_str_pos >= m_str.size()) {
                get_next_segment();
            }
            return c;
        }
        constexpr bool operator==(sentinel) const {
            return m_block == -2;
        };
        constexpr bool operator!=(sentinel) const {
            return m_block != -2;
        };
        constexpr bool operator==(iterator it) const {
            return m_c && it.m_c && m_block == it.m_block && m_str == it.m_str &&
                   m_str_pos == it.m_str_pos && m_chunk == it.m_chunk &&
                   m_chunk_pos == it.m_chunk_pos;
        };
        constexpr bool operator!=(iterator it) const {
            return !(*this == it);
        };

    private:
        constexpr void get_next_segment() {
            if(m_chunk_pos >= 3 || m_block == -1) {
                const auto range = __get_table_index(++m_block);
                if(range.first == nullptr) {
                    m_block = -2;
                    return;
                }

                const auto end = range.second;
                auto it = upper_bound(range.first, end, m_c, [](char32_t cp, uint64_t v) {
                    char32_t c = (v >> 32) & 0x00000000FFFFFFFF;
                    return cp < c;
                });
                if(it == end) {
                    m_block = -2;
                    return;
                }
                it--;
                if(it == end) {
                    m_block = -2;
                    return;
                }
                auto start = (*it) & 0x00000000FFFF'FFFF;
                if(start == 0xFFFFFFFF) {
                    m_block = -2;
                    return;
                }
                auto offset = m_c - char32_t((*it) >> 32);
                m_chunk = __name_indexes[start + offset];
                m_chunk_pos = -1;
            }
            m_chunk_pos++;
            uint16_t data = (m_chunk >> (48 - (16 * m_chunk_pos))) & 0x000000000000ffff;
            if(data == 0x0000) {
                m_block = -2;
                return;
            }
            uint8_t dict = data >> 8;
            uint8_t entry = data & 0x00ff;
            m_str = __get_name_segment(dict, entry);
            m_str_pos = 0;
        }


        char32_t m_c;
        int8_t m_block = -2;
        std::string_view m_str;
        std::size_t m_str_pos = 0;
        uint64_t m_chunk = 0xFFFFFFFFFFFFFFFF;
        int8_t m_chunk_pos = -1;
    };

    constexpr iterator begin() const {
        return iterator{c};
    }
    constexpr sentinel end() const {
        return {};
    }

    std::string to_string() const {
        std::string s;
        for(auto c : *this)
            s.push_back(c);
        return s;
    }

private:
    char32_t c;
};

}

constexpr auto cp_name(char32_t cp) {
    return details::name_view(cp);
}


}    // namespace uni