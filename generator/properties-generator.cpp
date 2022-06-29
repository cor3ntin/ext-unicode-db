#include "load.hpp"
#include "fmt/format.h"
#include "fmt/color.h"
#include <filesystem>
#include <ranges>

using namespace cedilla::tools;

void usage() {
    fmt::print(fg(fmt::color::red),
               "Usage : unicode-properties-data-generator ucd.nounihan.flat.xml\n");
    exit(1);
}

void dump(const char* ucdxml) {
    auto codepoints = load_codepoints(ucdxml);
    for(const codepoint & c : codepoints | std::views::filter([] (const codepoint & c) {
       return !c.reserved;
    })) {
        fmt::print("{}:{}\n", c.value, c.name);
    }
}

int main(int argc, const char** argv) {
    if(argc != 2 || !std::filesystem::exists(argv[1])) {
        usage();
    }
    dump(argv[1]);
}
