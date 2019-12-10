#include "pugixml.hpp"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <iostream>
#include <charconv>
#include <utility>
#include <vector>
#include <string>
#include <fmt/ranges.h>
#include <fmt/ostream.h>
#include <format.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/algorithm/any_of.hpp>
#include <range/v3/algorithm/sort.hpp>
#include <range/v3/view/remove_if.hpp>
#include <range/v3/algorithm/equal.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/unique.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/numeric/accumulate.hpp>
#include <range/v3/algorithm/copy.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "pstl/algorithm"
#include "pstl/execution"
#include "tbb/task_scheduler_init.h"
#include "tbb/concurrent_unordered_map.h"
#include "tbb/concurrent_vector.h"
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

std::string fixup_name(std::string n, std::string_view cp) {
    auto p = n.find("#");
    if(p == std::string::npos)
        return n;
    auto c = n;
    c.replace(p, 1, cp);
    return c;
}

std::unordered_map<char32_t, std::string> load_data(std::string db) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(db.c_str());
    std::unordered_map<char32_t, std::string> characters;

    pugi::xml_node rep = doc.child("ucd").child("repertoire");
    for(pugi::xml_node cp : rep.children("char")) {
        try {
            auto code = char32_t(std::stoi(cp.attribute("cp").value(), 0, 16));
            if(generated(code))
                continue;

            auto name = fixup_name(std::string(cp.attribute("na").value()), cp.attribute("cp").value());
            if(name.empty())
                continue;

            // if(characters.size() > 100)
            //     break;

            characters.emplace(code, name);

        } catch(...) {
        }
    }
    return characters;
}

struct character_name {
    char32_t cp;
    std::string_view name;
    std::vector<std::pair<std::size_t, std::string_view>> sub;
    std::size_t total = 0;

    bool complete() const {
        return total == name.size();
    }
    bool add_sub(std::size_t pos, std::string_view s) {
        for(const auto used : sub) {
            if(pos >= used.first && pos < used.first + used.second.size())
                return false;
        }
        sub.emplace_back(pos, s);
        total += s.size();
        ranges::sort(sub, std::less<>{}, &std::pair<std::size_t, std::string_view>::first);
        return true;
    }

    std::vector<std::pair<std::size_t, std::string_view>> bits() const {
        std::vector<std::pair<std::size_t, std::string_view>> v;
        std::size_t start = 0;
        for(const auto used : sub) {
            const auto end = used.first;
            std::string_view s = name.substr(start, end - start);
            if(!s.empty())
                v.emplace_back(start, s);
            start = used.first + used.second.size();
        }
        std::string_view s = name.substr(start);
        if(!s.empty())
            v.emplace_back(start, s);
        return v;
    }
};

template<typename R>
std::unordered_set<std::string_view> substrings(R&& r) {
    std::unordered_set<std::string_view> set;
    for(auto&& c : r) {
        for(const auto& b : c.bits()) {
            for(auto i : ranges::view::iota(1, b.second.size() + 1)) {
                auto sub = b.second.substr(0, i);
                if(sub.size() == 0)
                    break;
                set.insert(sub);
            }
        }
    }
    return set;
}

template<typename Map, typename K>
void inc(Map& map, K&& k) {
    auto it = map.find(k);
    if(it != map.end()) {
        it->second++;
        return;
    }
    map.emplace(k, 1);
}

template<typename Map>
auto sorted_by_occurences(Map& map) {
    auto v = map | ranges::to<std::vector<std::pair<typename Map::key_type, int>>>;
    ranges::sort(v, std::greater<>{}, &std::pair<typename Map::key_type, int>::second);
    return v;
}

struct block {
public:
    std::size_t elem_size;
    std::vector<std::string_view> data;
    std::unordered_set<char> alphabet;
    std::size_t size() const {
        return data.size();
    }
};
struct block_group {
public:
    std::size_t elem_size;
    std::vector<std::string_view> data;
    std::unordered_map<char, int> alphabet_mapping;

    std::size_t s = 0;

    void add_string(std::string_view str) {
        data.push_back(str);
    }

    struct ba {
        std::unordered_set<std::string_view> strings;
        std::unordered_set<char> alphabet;
        void insert(std::string_view s) {
            strings.insert(s);
            for(auto& c : s) {
                alphabet.insert(c);
            }
        }
        void remove(const ba& other) {
            for(auto str : other.strings)
                strings.erase(str);
            alphabet.clear();
            for(auto s : strings) {
                for(auto& c : s) {
                    alphabet.insert(c);
                }
            }
        }
    };

    std::vector<block> construct_blocks() {
        std::vector<block> blocks;
        std::unordered_multimap<char, ba> alphabet_mapping;
        for(auto& str : data) {
            for(auto& c : str) {
                bool inserted = false;
                auto it = alphabet_mapping.find(c);
                while(it != alphabet_mapping.end() && it->first == c) {
                    if(it->second.strings.size() < 255) {
                        it->second.insert(str);
                        inserted = true;
                        break;
                    }
                    ++it;
                }
                if(!inserted) {
                    it = alphabet_mapping.insert({c, ba{}});
                    it->second.insert(str);
                }
            }
        }
        auto v = alphabet_mapping |
                 ranges::view::transform([](const auto& p) { return p.second; }) |
                 ranges::to<std::vector>;

        while(!v.empty()) {
            auto e = std::move(v.back());
            v.pop_back();
            if(e.strings.empty())
                continue;
            for(auto it = v.begin(); it != v.end();) {
                if(e.strings.size() >= 255) {
                    break;
                }
                std::vector<char> merged_alphabet;
                std::set_union(it->alphabet.begin(), it->alphabet.end(), e.alphabet.begin(),
                               e.alphabet.end(), std::back_inserter(merged_alphabet));
                if(e.alphabet.size() <= 24 && merged_alphabet.size() > 24) {
                    ++it;
                    continue;
                }
                std::vector<std::string_view> merged_strings;
                std::set_union(it->strings.begin(), it->strings.end(), e.strings.begin(),
                               e.strings.end(), std::back_inserter(merged_strings));
                if(merged_strings.size() > 255) {
                    ++it;
                    continue;
                }
                e.alphabet = merged_alphabet | ranges::to<std::unordered_set>;
                e.strings = merged_strings | ranges::to<std::unordered_set>;
                ;
                it = v.erase(it);
            };
            for(auto it = v.begin(); it != v.end(); ++it) {
                it->remove(e);
            }
            blocks.push_back(block{elem_size, e.strings | ranges::to<std::vector>, e.alphabet});
        }

        return blocks;
    }
};

void print_dict(std::FILE* f, const std::vector<block>& blocks) {
    struct data {
        std::size_t start;
        std::size_t elem_size;
        std::size_t count;
    };
    size_t start = 0;

    int idx = 1;
    std::unordered_map<int, data> table;
    fmt::print(f, "static constexpr const char* __name_dict = \"");
    for(const auto& b : blocks) {
        for(const auto& str : b.data) {
            for(auto c : str) {
                fmt::print(f, "\\x{:x}", c);
            }
        }
        table[idx++] = data{start, b.elem_size, b.data.size()};
        start += b.elem_size * b.data.size();
    }
    fmt::print(f, "\";");
    fmt::print(f,
               "constexpr std::string_view __get_name_segment(std::size_t b, std::size_t idx) {{");
    fmt::print(f, "switch(b) {{");
    for(const auto& [k, v] : table) {
        fmt::print(f, "case {}:", k);
        fmt::print(f, "return std::string_view{{__name_dict + {0} + idx * {1}, {1} }};", v.start,
                   v.elem_size);
    }
    fmt::print(f, "}} return {{}};}}");
}
/*
void print_indexes(std::FILE* f, int idx, const std::unordered_map<char32_t, uint64_t>& mapping) {
    auto v = mapping | ranges::to<std::vector<std::pair<char32_t, uint64_t>>>;
    std::vector<std::uint64_t> data;
    std::vector<std::pair<char32_t, std::size_t>> data;
    bool in = false;
    ranges::sort(v, {}, &std::pair<char32_t, uint64_t>::first);
}*/

void print_indexes(std::FILE* f,
                   const std::unordered_map<int, std::unordered_map<char32_t, uint64_t>>& mapping) {

    std::vector<std::pair<std::size_t, std::vector<std::pair<char32_t, uint64_t>>>>
        sorted_jump_table;
    std::size_t start = 0;
    std::vector<uint64_t> data;

    auto sorted_mapping =
        mapping | ranges::to<std::vector<std::pair<int, std::unordered_map<char32_t, uint64_t>>>>;
    ranges::sort(sorted_mapping, {},
                 &std::pair<int, std::unordered_map<char32_t, uint64_t>>::first);

    for(auto& [k, v] : sorted_mapping) {
        auto arr = v | ranges::to<std::vector<std::pair<char32_t, uint64_t>>>;
        ranges::sort(arr, {}, &std::pair<char32_t, uint64_t>::first);
        for(auto& d : arr) {
            data.push_back(d.second);
        }
        sorted_jump_table.push_back({start, arr});
        start = data.size();
    }

    fmt::print(f, "static constexpr uint64_t __name_indexes[] = {{");
    for(auto& elem : data) {
        fmt::print(f, "{:#018x},", elem);
    }
    fmt::print(f, "0xFFFF'FFFF'FFFF'FFFF}};");

    for(const auto& [index, data] : ranges::view::enumerate(sorted_jump_table)) {
        fmt::print(f, "static constexpr uint64_t __name_indexes_{}[] = {{", index);
        bool first = true;
        char32_t prev = 0;
        size_t next_start = data.first;
        for(auto& c : data.second) {
            if(first || c.first != prev + 1) {
                fmt::print(f, "{:#018x},", (uint64_t(prev + 1) << 32) | uint32_t(0xFFFFFFFF));
                fmt::print(f, "{:#018x},", (uint64_t(c.first) << 32) | uint32_t(next_start));
            }
            first = false;
            prev = c.first;
            next_start++;
        }
        fmt::print(f, "{:#018x}}};", (uint64_t(prev + 1) << 32) | uint32_t(0xFFFFFFFF));
    }

    fmt::print(f, "constexpr std::pair<const uint64_t* const, const uint64_t* const> "
                  "__get_table_index(std::size_t index) {{");
    fmt::print(f, "switch(index) {{");
    for(const auto& [index, data] : ranges::view::enumerate(sorted_jump_table)) {
        fmt::print(f, "case {}:", index);
        fmt::print(f,
                   "return {{  __name_indexes_{0},  __name_indexes_{0} +  "
                   "sizeof(__name_indexes_{0})/sizeof(uint64_t) }};",
                   index);
    }
    fmt::print(f, "}} return {{nullptr, nullptr}}; }}");
}

int main(int argc, char** argv) {

    // tbb::task_scheduler_init init(4);

    const auto data = load_data(argv[1]);
    auto names = data | ranges::view::transform([](const auto& p) {
                     return character_name{p.first, p.second, {}, 0};
                 }) |
                 ranges::to<std::vector>;


    std::unordered_map<std::string_view, int> all_used;

    while(true) {
        auto incomplete =
            names | ranges::view::remove_if([](const auto& c) { return c.complete(); });
        fmt::print("Count : {}\n", ranges::distance(incomplete));

        if(ranges::empty(incomplete)) {
            break;
        }

        const auto subs = [&incomplete] {
            auto tmp = substrings(incomplete) | ranges::to<std::vector>;
            ranges::sort(tmp, std::greater<>{}, &std::string_view::size);
            return tmp;
        }();

        std::unordered_map<std::string_view, int> used_substrings;
        used_substrings.reserve(subs.size());
        for(const auto& s : subs) {
            used_substrings.emplace(s, 0);
        }


        // Compute a list of all possible substrings for each char
        // the value is the distance
        tbb::concurrent_unordered_multimap<std::string_view, std::pair<const character_name&, int>,
                                           std::hash<std::string_view>>
            all_substrings;

        const auto materialized = incomplete | ranges::to<std::vector>;

        std::for_each(std::execution::par, materialized.begin(), materialized.end(),
                      [&all_substrings](const character_name& c) {
                          for(const auto& b : c.bits()) {
                              for(auto i : ranges::view::iota(0, b.second.size() + 1)) {
                                  for(auto j : ranges::view::iota(i, b.second.size() + 1)) {
                                      all_substrings.insert(
                                          {b.second.substr(i, j),
                                           std::pair<const character_name&, int>{c, i}});
                                  }
                              }
                          }
                      });


        fmt::print("Substrings : {}\n", subs.size());

        std::for_each(std::execution::par, subs.begin(), subs.end(),
                      [& all_substrings = std::as_const(all_substrings),
                       &used_substrings](const auto& needle) {
                          auto it = all_substrings.find(needle);
                          while(it != all_substrings.end() && it->first == needle) {
                              const auto distance_from_start = it->second.second;
                              if(distance_from_start == 0 || distance_from_start > 4) {
                                  auto it = used_substrings.find(needle);
                                  it->second += 1;
                              }
                              ++it;
                          };
                      });

        fmt::print("Used Substrings : {}\n", used_substrings.size());


        std::vector<std::pair<std::string_view, double>> weighted_substrings =
            used_substrings | ranges::view::transform([](const auto& p) {
                const double d =
                    p.first.size() < 5 ? 1.0 : double(p.second) / (1.0 / double(p.first.size()));
                return std::pair<std::string_view, double>{p.first, d};
            });
        ;
        ranges::sort(weighted_substrings, std::greater<>{},
                     &std::pair<std::string_view, double>::second);
        const auto count = std::size_t(1 + 0.01 * double(weighted_substrings.size()));
        fmt::print("{}", count);
        std::vector<std::string_view> filtered =
            weighted_substrings | ranges::view::take(count) |
            ranges::view::transform([](const auto& p) { return p.first; });

        ranges::sort(filtered, std::greater<>{}, &std::string_view::size);

        for(const auto& s : filtered | ranges::view::take(10)) {
            for(auto& c : incomplete) {
                const auto bits = c.bits();
                for(const auto& b : bits) {
                    const auto idx = b.second.find(s);
                    if(idx != std::string::npos) {
                        if(c.add_sub(b.first + idx, s))
                            inc(all_used, s);
                    }
                }
            }
        }
        fmt::print("------\n");
    }
    auto strings = sorted_by_occurences(all_used);


    std::unordered_map<int, block_group> grouped_blocks;
    for(const auto& s : strings) {
        auto it = grouped_blocks.find(s.first.size());
        if(it == grouped_blocks.end()) {
            it = grouped_blocks.emplace(s.first.size(), block_group{s.first.size()}).first;
        }
        it->second.add_string(s.first);
    }

    std::vector<block> blocks;
    for(auto& grp : grouped_blocks) {
        for(const auto& b : grp.second.construct_blocks()) {
            blocks.push_back(b);
        }
    }
    std::size_t dict_size = 0;
    for(const auto& b : blocks) {
        fmt::print("--- BLOCK : string size : {}, elements  {} -- total {} - alphabet: {} ({})\n",
                   b.elem_size, b.size(), b.elem_size * b.size(),
                   b.alphabet | ranges::to<std::string>, b.alphabet.size());
        dict_size += (b.elem_size * b.size()) / (b.alphabet.size() <= 24 ? 2 : 1);
    }
    fmt::print("Total blocks: {}\n", blocks.size());

    std::size_t index_bytes = 0;
    std::unordered_map<int, int> lengths;
    for(const auto& c : names) {
        inc(lengths, ranges::size(c.sub));
        index_bytes += 2 * ranges::size(c.sub);
    }

    auto sorted_lengths = sorted_by_occurences(lengths);
    // fmt::print("DICT : \n{}\n ----", strings);
    fmt::print("LENGTHS : \n{}\n ----", sorted_lengths);
    fmt::print("KBytes: {}  ( dict {}  + index : {} )\n", (index_bytes + dict_size) / 1024.0,
               dict_size / 1024.0, index_bytes / 1024.0);

    auto f = fopen("test.h", "w");
    fmt::print(f, "#pragma once\n#include <string_view>\n#include <array>\n\n");

    print_dict(f, blocks);

    struct pos {
        int b;
        int i;
    };

    std::unordered_map<std::string_view, pos> indexes;
    int bi = 1;
    for(const auto& b : blocks) {
        int idx = 0;
        for(const auto& str : b.data) {
            indexes[str] = pos{bi, idx++};
        }
        bi++;
    }

    std::unordered_map<int, std::unordered_map<char32_t, uint64_t>> mapping;
    for(const auto& c : names) {
        int seq = 0;
        int i = 0;

        uint64_t v = 0x00000000000000;
        while(i < c.sub.size()) {
            const auto n = 64 - 16 * (i % 4);
            const auto pos = indexes[c.sub.at(i).second];
            v |= (uint64_t(pos.b) << (n - 8)) | (uint64_t(pos.i) << (n - 16));
            if(((i % 4) == 3) || i == c.sub.size() - 1) {
                mapping[seq++].insert({c.cp, v});
                v = 0;
            }
            i++;
        }
    }
    print_indexes(f, mapping);


    fclose(f);
}


//    fmt::print("{}\n", names.size());
//}