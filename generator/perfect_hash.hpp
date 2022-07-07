#pragma once
#include "utils.hpp"
#include <map>
#include <unordered_set>
#include <fmt/ranges.h>

namespace cedilla::tools {

inline std::uint32_t hash(std::uint32_t c, std::uint32_t salt, std::uint64_t n) {
    std::uint32_t y = (c + salt) * 2654435769;
    y ^= c * 0x31415926;
    return (std::uint64_t(y) * std::uint64_t(n)) >> 32;
}


template <typename T>
struct hash_data {
    std::vector<std::uint16_t> salts;
    std::vector<T> keys;
};

template <typename T>
inline hash_data<T> create_perfect_hash(const std::vector<T> & data) {
    using Hash = std::uint32_t;
    const auto N = data.size();
    std::map<Hash, std::vector<T>> buckets;
    for(std::size_t i = 0; i < N; i++)
        buckets.emplace(i, std::vector<T>{});
    for(T c : data) {
        buckets[hash(c, 0, N)].push_back(c);
    }

    std::vector<std::tuple<std::size_t, Hash>> sorted;
    for(const auto & [k, v] : buckets) {
        sorted.emplace_back(v.size(), k);
    }

    std::ranges::sort(sorted, std::greater<>{});
    std::vector<bool>     claimed(N, false);
    std::vector<std::uint16_t> salts(N, 0);
    std::vector<T> keys(N, 0);

    for(auto [bucket_size, h] : sorted) {
        if(bucket_size == 0)
            break;
        for(std::size_t salt : std::views::iota(1u, std::numeric_limits<std::uint16_t>::max())) {
            std::unordered_set<Hash> rehashes;
            std::ranges::transform(buckets[h], std::inserter(rehashes, rehashes.begin()), [&] (char32_t c) {
                return hash(c, salt, N);
            });

            bool has_collision = std::ranges::any_of(rehashes, [&](const Hash & hash) {
                       return claimed[hash] == true;
            });

            if(has_collision || rehashes.size() < bucket_size)
                continue;

            salts[h] = salt;
            for(char32_t k : buckets[h]) {
                Hash rehash = hash(k, salt, N);
                claimed[rehash] = true;
                keys[rehash] = k;
            }
            break;
        }
        if(salts[h] == 0) {
            die("could not find a suitable salt - too many elements?");
        }
    }
    return {salts, keys};

}

template <typename T>
void print_hash_data(FILE* output,
                     std::string_view type,
                     std::string_view var_name,
                     const std::vector<std::uint16_t> & salts,
                     const std::vector<T> & values) {
constexpr const char* tpl = R"(
      {}<{}>  {} {{
         . salts  = {{ {} }},
         . values = {{ {} }}
      }};
)";
    fmt::print(output, tpl, type, salts.size(),
           var_name,
           fmt::join(salts, ", "),  fmt::join(values, ", "));
}

}
