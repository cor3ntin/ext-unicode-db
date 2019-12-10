#include <string_view>
#include <iostream>
#include <tuple>
#include <charconv>
#include <format.hpp>
namespace uni {
    namespace details {
        struct node {
            char32_t value  = 0xFFFFFF;
            uint32_t children_offset = 0;
            bool has_sibling = false;
            uint32_t size = 0;
            std::string_view name;


            bool is_valid() const {
                return name.size() != 0;
            }
            bool has_children() const {
                return children_offset != 0;
            }
        };
        node read_node(uint32_t offset) {
            using namespace uni::details;
            const uint32_t origin = offset;
            node n;

            uint8_t name = index[offset++];
            if(offset + 6 >= sizeof(index))
                return n;

            const bool long_name = name & 0x40;
            const bool has_value = name & 0x80;
            name &= ~0xC0;
            if(long_name) {
                uint32_t name_offset = (index[offset++] << 8);
                name_offset |= index[offset++];
                n.name = std::string_view(dict + name_offset, name);
            }
            else {
               n.name = std::string_view(dict + name, 1);
            }
            if(has_value) {
                uint8_t h = index[offset++];
                uint8_t m = index[offset++];
                uint8_t l = index[offset++];
                n.value = ((h << 16) | (m << 8) | l) >> 3;

                bool has_children = l & 0x02;
                n.has_sibling = l & 0x01;

                if(has_children) {
                    n.children_offset  = (index[offset++] << 16);
                    n.children_offset |= (index[offset++] << 8);
                    n.children_offset |= index[offset++];
                }
            }
            else {
                uint8_t h = index[offset++];
                n.has_sibling = h & 0x80;
                bool has_children = h & 0x40;
                if(has_children) {
                    h &= ~0xC0;
                    n.children_offset = (h << 16);
                    n.children_offset |= (uint32_t(index[offset++]) << 8);
                    n.children_offset |= index[offset++];
                }
            }
            n.size = offset - origin;
            return n;
        }


        int compare(std::string_view str, std::string_view needle, uint32_t start) {
            int str_i = start;
            int needle_i = 0;
            if(needle.size() == 0)
                return -1;
            bool had_space = start == 0 ? true : str[start - 1] == ' ';
            while(true) {
                if(needle_i == needle.size())
                    return str_i;
                if(str_i == str.size())
                    return -1;
                const auto a = str[str_i];
                if(a == '-' && !had_space) {
                    str_i ++;
                    continue;
                }
                had_space = a == ' ';
                if(had_space) {
                    str_i ++;
                    continue;
                }

                if(a != needle[needle_i])
                    return -1;
                str_i ++;
                needle_i ++;
            }
            return -1;
        }

        std::tuple<node, bool, uint32_t> compare_node(uint32_t offset, std::string_view name, uint32_t start = 0) {
            auto n = details::read_node(offset);
            auto cmp = details::compare(name, n.name, start);
            if(cmp == -1) {
                return {n, false, 0};
            }
            start = cmp;
            if(name.size() == start)
                return {n, true, n.value};
            if(n.has_children()) {
                auto o = n.children_offset;
                for(;;) {
                    auto [c, res, value] = compare_node(o, name, start);
                    if(res) {
                        return {n, true, value};
                    }
                    o += c.size;
                    if(!c.has_sibling)
                        break;
                }
            }
            return {n, false, 0};
        }

        constexpr const char * const hangul_syllables[][3] = {
            { "G",  "A",   ""   },
            { "GG", "AE",  "G"  },
            { "N",  "YA",  "GG" },
            { "D",  "YAE", "GS" },
            { "DD", "EO",  "N", },
            { "R",  "E",   "NJ" },
            { "M",  "YEO", "NH" },
            { "B",  "YE",  "D"  },
            { "BB", "O",   "L"  },
            { "S",  "WA",  "LG" },
            { "SS", "WAE", "LM" },
            { "",   "OE",  "LB" },
            { "J",  "YO",  "LS" },
            { "JJ", "U",   "LT" },
            { "C",  "WEO", "LP" },
            { "K",  "WE",  "LH" },
            { "T",  "WI",  "M"  },
            { "P",  "YU",  "B"  },
            { "H",  "EU",  "BS" },
            { 0,    "YI",  "S"  },
            { 0,    "I",   "SS" },
            { 0,    0,     "NG" },
            { 0,    0,     "J"  },
            { 0,    0,     "C"  },
            { 0,    0,     "K"  },
            { 0,    0,     "T"  },
            { 0,    0,     "P"  },
            { 0,    0,     "H"  }
        };

        struct generated_name_data {
            std::string_view prefix;
            uint32_t start;
            uint32_t end;
        };

        constexpr const generated_name_data generated_name_data_table[] = {
            {"CJK UNIFIED IDEOGRAPH-", 0x3400, 0x4DB5},
            {"CJK UNIFIED IDEOGRAPH-", 0x4E00, 0x9FEA},
            {"CJK UNIFIED IDEOGRAPH-", 0x20000, 0x2A6D6},
            {"CJK UNIFIED IDEOGRAPH-", 0x2A700, 0x2B734},
            {"CJK UNIFIED IDEOGRAPH-", 0x2B740, 0x2B81D},
            {"CJK UNIFIED IDEOGRAPH-", 0x2B820, 0x2CEA1},
            {"CJK UNIFIED IDEOGRAPH-", 0x2CEB0, 0x2EBE0},
            {"TANGUT IDEOGRAPH-", 0x17000, 0x187EC},
            {"NUSHU CHARACTER-", 0x1B170, 0x1B2FB},
            {"CJK COMPATIBILITY IDEOGRAPH-", 0xF900, 0xFA6D},
            {"CJK COMPATIBILITY IDEOGRAPH-", 0xFA70, 0xFAD9},
            {"CJK COMPATIBILITY IDEOGRAPH-", 0x2F800, 0x2FA1D},
        };


        bool starts_with(std::string_view str, std::string_view needle) {
            return str.size() >= needle.size() && str.compare(0, needle.size(), needle) == 0;
        }

        static int find_syllable(std::string_view str, int & pos, int count, int column) {
            int len = -1;
            for (int i = 0; i < count; i++) {
                std::string_view s(hangul_syllables[i][column]);
                if (int(s.size()) <= len)
                    continue;
                if (starts_with(str, s)) {
                    len = s.size();
                    pos = i;
                }
            }
            if (len == -1)
                len = 0;
            return len;
        }

        constexpr const char32_t SBase = 0xAC00;
        constexpr const char32_t LBase = 0x1100;
        constexpr const char32_t VBase = 0x1161;
        constexpr const char32_t TBase = 0x11A7;
        constexpr const int LCount = 19;
        constexpr const int VCount = 21;
        constexpr const int TCount = 28;
        constexpr const int NCount = (VCount * TCount);
        constexpr const int SCount = (LCount * NCount);
    }


    char32_t cp_from_name(std::string_view name) {
        using namespace details;

        if (std::string_view prefix = "HANGUL SYLLABLE "; starts_with(name, prefix)) {
            name.remove_prefix(prefix.size());
            int L = -1, V = -1, T = -1;
            name.remove_prefix(find_syllable(name, L, LCount, 0));
            name.remove_prefix(find_syllable(name, V, VCount, 1));
            name.remove_prefix(find_syllable(name, T, TCount, 2));
            if (L != -1 && V != -1 && T != -1 && name.size() == 0) {
                return SBase + (L*VCount+V)*TCount + T;
            }
            // Otherwise, it's an illegal syllable name.
            return 0xFFFFFF;
        }
        for(auto && item : generated_name_data_table) {
            if (starts_with(name, item.prefix)) {
                auto gn = name;
                gn.remove_prefix(item.prefix.size());
                uint32_t v;
                const auto end = gn.data() + gn.size();
                auto [p, ec] = std::from_chars(gn.data(), end , v, 16);
                if(ec != std::errc() || p != end || v < item.start || v > item.end)
                    continue;
                return v;
            }
        }

        uint32_t offset = 0;
        for(;;) {
            auto [n, res, value] = details::compare_node(offset, name);
            if(!n.is_valid())
                break;
            if(res) {
                //HANGUL JUNGSEONG O-E - fix up hyphen
                if(value == 0x116c && name.find("O-E") != std::string_view::npos)
                    value = 0x1180;
                return value;
            }
            if(!n.has_sibling)
                break;
            offset += n.size;
        }
        return 0xFFFFFF;
    }
}
