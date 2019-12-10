#include "pugixml.hpp"
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <string>
#include <iostream>
#include <deque>
#include <charconv>
#include <utility>
#include <vector>
#include <string>
#include <set>
#include <fmt/ranges.h>
#include <fmt/ostream.h>
#include <format.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/algorithm/any_of.hpp>
#include <range/v3/algorithm/sort.hpp>
#include <range/v3/algorithm/find.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/algorithm/remove_if.hpp>
#include <locale>
#include "range/v3/view/span.hpp"

#include <mutex>

bool generated(char32_t c) {
    const std::array ranges = {
        std::pair{0xAC00, 0xD7A3},   std::pair{0x3400, 0x4DB5},   std::pair{0x4E00, 0x9FFA},
        std::pair{0x20000, 0x2A6D6}, std::pair{0x2A700, 0x2B734}, std::pair{0x2B740, 0x2B81D},
        std::pair{0x2B820, 0x2CEA1}, std::pair{0x2CEB0, 0x2EBE0}, std::pair{0x17000, 0x187EC},
        std::pair{0x1B170, 0x1B2FB}, std::pair{0x0F900, 0xFA6D},  std::pair{0x0FA70, 0xD7F9},
        std::pair{0x2F800, 0x2FA1D}};
    for(auto r : ranges) {
        if(c >= r.first && c <= r.second)
            return true;
    }
    return false;
}

std::unordered_multimap<char32_t, std::string> load_data() {
    pugi::xml_document doc;
    pugi::xml_parse_result result =
        doc.load_file("/home/cor3ntin/dev/unicode/props/ucd/12.0/ucd.nounihan.flat.xml");
    std::unordered_multimap<char32_t, std::string> characters;

    pugi::xml_node rep = doc.child("ucd").child("repertoire");
    for(pugi::xml_node cp : rep.children("char")) {
        try {
            auto code = char32_t(std::stoi(cp.attribute("cp").value(), 0, 16));
            
            auto name = std::string(cp.attribute("na").value());
            if(!generated(code) && !name.empty()) {
                characters.insert({code, name});
            }

            for(pugi::xml_node alias : cp.children("name-alias")) {
                name = std::string(cp.attribute("alias").value());
                characters.insert({code, name});
            }

        } catch(...) {
        }
    }
    return characters;
}

struct character_name {
    char32_t cp;
    std::string name;
};


class trie {
    struct node;
public:
    void insert(std::string_view name, char32_t v) {
        node* n = root.get();
        for(auto ch : name) {
            auto it = ranges::find_if(n->children, [name  = std::string(1, ch) ](const auto & c) {
                return c->name == name;
            });
            if(it == n->children.end()) {
                it = n->children.insert(it, std::make_unique<node>(std::string(1, ch), n));
            }
            n = it->get();
        }
        n->value = v;
    }
    void compact() {
        compact(root.get());
    }

    int count() const {
        return count(root.get());
    }

    int max_children_count() const {
        int max = 0;
        max_children_count(root.get(), max);
        return max;
    }

    void max_children_count(node* n, int & max) const {
        if(n->children.size() > max)
            max = n->children.size();
        for(auto & c : n->children) {
            max_children_count(c.get(), max);
        }
    }

    int count(node* n) const {
        int the_count = 1;
        for(auto & c : n->children) {
            the_count += count(c.get());
        }
        return the_count;
    }

    void print() {
        print(root.get());
    }

    void print(node* n, int indent = 0) {
        fmt::print("{:>{}} {} (value: {})\n", "{", indent * 4, n->name, n->value ? *n->value : 0 );
        for(auto && c : n->children) {
            print(c.get(), indent + 1);
        }
        fmt::print("{:>{}}\n", "}", indent * 4 );
    }

    int bytes() {
        return bytes(root.get());
    }

    std::set<std::string> names() {
        std::set<std::string> set;
        collect_keys(root.get(), set); 
        return set;
    }

    int bytes(node* n) {
        int s =  2 + (n->name.size() > 1 ? 3 : 1) + (n->value ? 3 : 0);
        for(auto & c : n->children) {
            s += bytes(c.get());
        }
        return s;
    }
    std::tuple<std::string, std::vector<uint8_t>, uint32_t> dump() {
        std::set<std::string> names = this->names();
        std::vector<std::string> sorted(names.begin(), names.end());
        ranges::sort(sorted, [](const auto &a, const auto & b) {
            return a.size() > b.size();
        });
        std::string dict = "\t #ABCDEFGHIJKLMNOPQRSTUVWXYZ_-0123456789";
        dict.reserve(50000);
        for(const auto & n : sorted) {
            if(n.size() <= 1)
                continue;
            if(dict.find(n) != std::string::npos)
               continue;
            dict += n;
        }
        auto [bytes, non_leaf_offset] = dump2(dict);
        return {dict, bytes, non_leaf_offset};
    }

    uint8_t letter(char c ) {
        static const std::string letters = "\t #ABCDEFGHIJKLMNOPQRSTUVWXYZ_-0123456789";
        return letters.find(c);
    }

    void collect_children(node* n, std::deque<node*> & children) {
        for(auto & c : n->children) {
            collect_children(c.get(), children);
        }
        if(!!n->value) {
            children.push_back(n);
        } 
    }

    std::pair<std::vector<uint8_t>, uint32_t> dump2(const std::string & dict) {
        std::unordered_map<node*, int32_t> offsets;
        std::unordered_map<int32_t, node*> parent_offsets;
        std::unordered_map<int32_t, node*> parent_offsets_leafs;
        std::deque<node*> nodes;
        std::vector<uint8_t> bytes;
        uint32_t non_leaf_offset = 0;
        
        bytes.reserve(200'000);
        offsets.reserve(100000);
        parent_offsets.reserve(100000);
        parent_offsets_leafs.reserve(100000);

        collect_children(root.get(), nodes);
        ranges::sort(nodes, [](const auto & a, const auto & b) {
            return a->name < b->name;
        });

        auto leafs = nodes.size();
        bool add_terminal = true;

        while(!nodes.empty()) {
            auto offset = bytes.size();
            node* const n = nodes.front();
            nodes.pop_front();

            if(offsets.count(n))
                continue;
            
            offsets[n] = offset;
            
            if(n->name.size() == 0) {
                if(n->parent)
                    throw std::runtime_error("NULL name");
                continue;
            }

            //fmt::print("// {} \n", n->name );
            
            if(n->name.size() == 1 ) {
                bytes.push_back(letter(n->name[0]));
            }
            else {
                uint8_t b = uint8_t(n->name.size()) | uint8_t(0x80);
                bytes.push_back(b);
                auto pos = dict.find(n->name);
                if(pos == std::string::npos) {
                    std::runtime_error("oups");
                }
                uint8_t l = pos;
                uint8_t h = ((pos>>8)&0xFF);
                bytes.push_back(h);
                bytes.push_back(l);
            }

            if(leafs > 0) {
                uint32_t v = (*(n->value) << 3);
                
                uint8_t h = ((v>>16) & 0xFF);
                uint8_t m = ((v>>8) & 0xFF);
                uint8_t l = (v & 0xFF);


                bytes.push_back(h);
                bytes.push_back(m);

                parent_offsets_leafs[bytes.size()] = n->parent;

                bytes.push_back(l);
                bytes.push_back(0xFF);
                bytes.push_back(0xFF);

                leafs --;
            }
            else if(n->parent) {
                parent_offsets[bytes.size()] = n->parent;
                
                bytes.push_back(0xFF);
                bytes.push_back(0xFF);
                bytes.push_back(0xFF);
            }

            if(n->parent && ranges::find(nodes, n->parent) == nodes.end())
                nodes.push_back(n->parent);

            if(leafs == 0 && add_terminal) {
                bytes.push_back(0x00);
                add_terminal = false;
                non_leaf_offset = bytes.size();
            }
        }
        
        bytes.push_back(0);
        bytes.push_back(0);
        bytes.push_back(0);
        bytes.push_back(0);
        bytes.push_back(0);
        bytes.push_back(0);

        for(auto && [k, v] : parent_offsets) {

            const auto it = offsets.find(v);
            if(it == offsets.end()) {
                throw std::runtime_error("oups");
            }
            uint32_t pos =  it->second;

            bytes[k] = ((pos>>16) & 0xFF);
            bytes[k + 1] = ((pos>>8) & 0xFF);
            bytes[k + 2] = pos & 0xFF;
        }
        for(auto && [k, v] : parent_offsets_leafs) {

            const auto it = offsets.find(v);
            if(it == offsets.end()) {
                throw std::runtime_error("oups");
                break;
            }
            uint32_t pos =  it->second;

            bytes[k] =  bytes[k] | uint8_t((pos>>16) & 0xFF);
            bytes[k + 1] = ((pos>>8) & 0xFF);
            bytes[k + 2] = pos & 0xFF;
        }

        return {bytes, non_leaf_offset};
    }


private:
    void collect_keys(node* n, std::set<std::string> & v) {
        v.insert(n->name);
        for(auto && child : n->children) {
            collect_keys(child.get(), v);
        }
    }
    void compact(node* n) {
        for(auto && child : n->children) {
            compact(child.get());
        }
        if(n->parent && n->parent->children.size() == 1 && !n->parent->value) {
            n->parent->value = n->value;
            n->parent->name += n->name;
            n->parent->children = std::move(n->children);
            for(auto & c : n->parent->children) {
                c->parent = n->parent;
            }
        }
    }
    struct node {
        node(std::string name, node* parent = nullptr) : name(name), parent(parent) {}
        std::vector<std::unique_ptr<node>> children;
        std::string name;
        node* parent = nullptr;
        std::optional<char32_t> value;
    };
    std::unique_ptr<node> root = std::make_unique<node>("");
};

int main() {
    trie t;
    for(auto [v, name] : load_data()) {

        name.erase(ranges::remove_if(name.begin(), name.end(),[](auto c) {
            return c == '-' || c == ' ';
        }), name.end());
        t.insert(name, v);
    }
    fmt::print("//{} / {} / {} \n", t.count(), t.bytes()/1024, t.max_children_count());
    t.compact();
    fmt::print("//{} / {} / {} \n", t.count(), t.bytes()/1024, t.max_children_count());
    auto names = t.names();


    auto it = std::max_element(names.begin(), names.end(), [] (const auto & a, const auto & b) {
        return a.size() < b.size();
    });
    fmt::print("//{}\n", it->size());

    auto [dict, bytes, non_leaf_offset] = t.dump();
    fmt::print("//dict : {} / tree : {} \n", dict.size()/1024, bytes.size()/1024);
    //t.print();

    
    fmt::print("#include <cstdint>\n");
    fmt::print("namespace uni::details {{\n");
    fmt::print("constexpr const uint32_t non_leaf_offset = {};\n", non_leaf_offset);
    fmt::print("constexpr const char* dict = \"{}\";\n", dict);
    fmt::print("constexpr const uint8_t index[] = {{\n");
    for(auto b : bytes) {
        fmt::print("0x{:02x},", b);
    }
    fmt::print("0}};\n}}\n");
}