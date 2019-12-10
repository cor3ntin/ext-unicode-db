#include <string_view>
#include <iostream>
#include <tuple>
#include "output.h"
namespace uni {
    namespace details {
        struct node {
            char32_t value  = 0xFFFFFF;
            uint32_t parent_offset = 0;
            uint32_t size = 0;
            std::string_view name;

            bool is_valid() const {
                return name.size() != 0;
            }
            bool has_parent() const {
                return parent_offset != 0xFFFFFF && parent_offset != 0;
            }
        };
        node read_node(uint32_t offset) {
            const uint32_t origin = offset;
            const bool leaf = offset < non_leaf_offset;
            node n;
            using namespace uni::details;
            if(offset + 6 > sizeof(index))
                return n;
            uint8_t name = index[offset++];
            if(name == 0) {
                n.size = offset - origin;
                return n;
            }
            if(name & 0x80) {
                uint8_t size = name & ~0x80;
                uint32_t name_offset = (index[offset++] << 8);
                name_offset |= index[offset++];
                n.name = std::string_view(dict + name_offset, size);
            }
            else {
               n.name = std::string_view(dict + name, 1);
            }
            if(leaf) {
                uint8_t h = index[offset++];
                uint8_t m = index[offset++];
                uint8_t l = index[offset++];
                n.value = ((h << 16) | (m << 8) | l) >> 3;

                n.parent_offset = ((l & 0x7) << 16);
            }
            else {
                n.parent_offset =  (uint32_t(index[offset++]) << 16);
            }
            n.parent_offset |= (uint32_t(index[offset++]) << 8);
            n.parent_offset |= index[offset++];
            n.size = offset - origin;
            return n;
        };

        void print_node(uint32_t offset, bool child) {
            auto n = read_node(offset);
            if(!n.is_valid())
                return;

            //fmt::print("{} -  ", n.name);
            //fmt::print("{}\n", n.parent_offset);
            if(n.has_parent())  
                print_node(n.parent_offset, false);
            std::cout << n.name;
            if(child) {
                std::cout << "\n";
            }
        }

        int compare_end(std::string_view str, std::string_view needle) {
            std::size_t na = str.size() - 1;
            std::size_t nb = needle.size() - 1;
            while(true) {
                auto a = str[na];
                if(a == '_' || a == '-' || a == ' ') {
                    na --;
                    continue;
                }
                
                if(a >= 'a' && a <= 'z')
                    a -= ('a' - 'A');
                if(a != needle[nb])
                    return -1;

                if(nb == 0)
                    return na;
                if(na == 0)
                    return -1;
                na --;
                nb --;
            }
            return -1;
        }

        std::tuple<node, bool> compare_parent(int offset, std::string_view name) {
            auto n = details::read_node(offset);
            if(!n.is_valid())
                return {n, false};
            auto cmp = details::compare_end(name, n.name);
            std::cout << name << " " << n.name << " " << cmp << "\n";
            if(cmp == -1) {
                return {n, false};
            }
            name = name.substr(0, cmp);
            if(name.empty() && !n.has_parent())
                return {n, true};
            auto [_, res] = compare_parent(n.parent_offset, name);
            return {n, res};
        }
    }
    char32_t cp_from_name(std::string_view name) {
        uint32_t offset = 0;
        for(;;) {
            auto [n, res] = details::compare_parent(offset, name);
            if(res)
                return n.value;
            if(!n.is_valid())
                break;
            offset += n.size;
        }
        return 0;
    }
}
