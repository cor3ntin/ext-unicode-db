#include "bitset_generator.hpp"
#include "skiplist_generator.hpp"

namespace cedilla::tools {

inline void print_binary_data_best(FILE* output, const range_of<char32_t> auto & data, std::string_view var_name) {
    std::optional<bitset_data> bitset; // = create_bitset(data);
    skiplist_data skiplist = create_skiplist(data);
    if(!bitset || skiplist.size() < bitset->size()) {
        print_skiplist_data(output, var_name, skiplist);
    }
    else {
        print_bitset_data(output, var_name, *bitset);
    }
}

}
