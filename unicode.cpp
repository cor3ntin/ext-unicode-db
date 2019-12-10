#include "ext/unicode.hpp"

static_assert(uni::cp_script('C') == uni::script::latin);
static_assert(uni::cp_block(U'🎉') == uni::block::misc_pictographs);
static_assert(!uni::cp_is<uni::property::xid_start>('1'));
static_assert(uni::cp_is<uni::property::xid_continue>('1'));
static_assert(uni::cp_age(U'🤩') == uni::version::v10_0);
static_assert(uni::cp_is<uni::property::alphabetic>(U'ß'));
static_assert(uni::cp_category(U'🦝') == uni::category::so);
static_assert(uni::cp_is<uni::category::lowercase_letter>('a'));
static_assert(uni::cp_is<uni::category::letter>('a'));


void dummy_symbol() {}