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
#include <range/v3/numeric/accumulate.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "pstl/algorithm"
#include "pstl/execution"
#include "tbb/task_scheduler_init.h"

bool generated(char32_t c) {
    const std::array ranges = {
            std::pair{0xAC00, 0xD7A3},
            std::pair{0x3400, 0x4DB5},
            std::pair{0x4E00, 0x9FFA},
            std::pair{0x20000, 0x2A6D6},
            std::pair{0x2A700, 0x2B734},
            std::pair{0x2B740, 0x2B81D},
            std::pair{0x2B820, 0x2CEA1},
            std::pair{0x2CEB0, 0x2EBE0},
            std::pair{0x17000, 0x187EC},
            std::pair{0x1B170, 0x1B2FB},
            std::pair{0x0F900, 0xFA6D},
            std::pair{0x0FA70, 0xD7F9},
            std::pair{0x2F800, 0x2FA1D}
    };
    for(auto r : ranges) {
        if (c >= r.first && c <= r.second)
            return true;
    }
    return false;
}

std::unordered_map<char32_t, std::string> load_data() {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file("/home/cor3ntin/dev/unicode/props/ucd/12.0/ucd.nounihan.flat.xml");
    std::unordered_map<char32_t, std::string> characters;

    pugi::xml_node rep = doc.child("ucd").child("repertoire");
    for(pugi::xml_node cp : rep.children("char")) {
        try {
            auto code = char32_t(std::stoi(cp.attribute("cp").value(), 0, 16));
            if(generated(code))
                continue;

            auto name = std::string(cp.attribute("na").value());
            characters.emplace(code, name);

            //if(characters.size() > 10000)
            //    return characters;

        }catch(...) {}
    }
    return characters;
}

struct character_name {
    char32_t cp;
    std::string_view name;
    std::vector<std::string_view> sub;
    std::size_t pos = 0;

    bool complete() const {
        return pos == name.size();
    }
    void add_sub(std::string_view s) {
        sub.push_back(s);
        pos += s.size();
    }
};

template <typename R>
std::vector<std::string_view> substrings(R && r) {
    std::unordered_set<std::string_view> set;
    for(auto && c : r) {
        for(auto i : ranges::view::iota(1, c.name.size())) {
            auto sub = c.name.substr(c.pos, c.pos + i);
            if(sub.size() == 0)
                break;
            set.insert(sub);
        }
    }
    return set | ranges::to<std::vector>();
}

template <typename Map, typename K>
void inc(Map & map, K && k) {
    auto it = map.find(k);
    if( it != map.end()) {
        it->second++;
        return;
    }
    map.emplace(k, 1);
}

template <typename Map>
auto sorted_by_occurences(Map & map) {
    auto v = map | ranges::to<std::vector<std::pair<typename Map::key_type, int>>>;
    ranges::sort(v, std::greater<>{}, &std::pair<typename Map::key_type, int>::second);
    return v;
}

int main() {

    cpu_set_t mask;
    int       status;
    CPU_ZERO(&mask);
    const auto NUMCORES = sysconf(_SC_NPROCESSORS_ONLN);
    for (int64_t core = 0; core < NUMCORES; core++) CPU_SET(core, &mask);
    sched_setaffinity(0, sizeof(cpu_set_t), &mask);

    tbb::task_scheduler_init init(4);


    const auto data = load_data();
    auto names = data | ranges::view::transform([](const auto & p){
        return character_name{p.first, p.second, {}, 0};
    }) | ranges::to<std::vector>;


    std::unordered_map<std::string_view, int> all_used;

    while(true) {
        auto incomplete = names | ranges::view::remove_if([](const auto &c) {
            return c.complete();
        });
        fmt::print("Count : {}\n", ranges::distance(incomplete));

        if(ranges::empty(incomplete)) {
            break;
        }

        auto subs = substrings(incomplete);
        std::unordered_map<std::string_view, std::atomic_int> used_substrings;
        for(const auto & s : subs) {
            used_substrings.emplace(s, 0);
        }



        fmt::print("Substrings : {}\n", subs.size());

        ranges::sort(subs, std::greater<>{}, &std::string_view::size);

        const auto materialized = incomplete | ranges::to<std::vector>;

        std::for_each(std::execution::par_unseq, subs.begin(), subs.end(), [materialized, &used_substrings](const auto & s) {
           std::for_each(std::execution::par_unseq, materialized.begin(), materialized.end(), [&used_substrings, &s](const auto & c) {
               const auto n = c.name.substr(c.pos);
               if(boost::starts_with(n, s)) {
                   used_substrings[s] += 1;
               }
           });
        });

        fmt::print("Used Substrings : {}\n", used_substrings.size());


         std::vector<std::pair<std::string_view, double>> weighted_substrings
                 = used_substrings | ranges::view::transform([](const auto & p) {
                const double d = p.first.size() < 4 ? 1.0 : double(p.second) / (1.0/double(p.first.size()));
             return  std::pair<std::string_view, double>{p.first, d};});
         ;
         ranges::sort(weighted_substrings, std::greater<>{}, &std::pair<std::string_view, double>::second);
         const auto count = 50;
         std::vector<std::string_view> filtered = weighted_substrings | ranges::view::take(count) | ranges::view::transform([](const auto & p) {
             return p.first;});

        ranges::sort(filtered, std::greater<>{}, &std::string_view::size);

         for(const auto & s : filtered) {
             for(auto & c : incomplete) {
                 const auto n = c.name.substr(c.pos, c.name.size());
                 if(boost::starts_with(n, s)) {
                     c.add_sub(s);
                     inc(all_used, s);
                 }
             }
         }
         fmt::print("------\n");
    }
    auto strings = sorted_by_occurences(all_used);
    const auto string_bytes = ranges::accumulate(strings, 0, std::plus{}, [](auto &p) {
        return p.first.size();
    });

    std::size_t index_bytes = 0;

     std::unordered_map<int, int> lengths;
     for(const auto & c : names ) {
         inc(lengths, ranges::size(c.sub));
         index_bytes += 2 * ranges::size(c.sub);
     }

     auto sorted_lengths = sorted_by_occurences(lengths);
     fmt::print("DICT : \n{}\n ----", strings);
     fmt::print("LENGTHS : \n{}\n ----", sorted_lengths);
     fmt::print("KBytes: {}  ( dict {}  + index : {} )\n", (index_bytes + string_bytes)/1024.0, string_bytes/1024.0, index_bytes/1024.0);
}




//    fmt::print("{}\n", names.size());
//}