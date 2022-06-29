#!/usr/bin/python3

import xml.etree.cElementTree as etree
import sys
import os
import re
import collections
import copy
from io import StringIO
from bool_trie import *

DIR_WITH_UCD = os.path.realpath(sys.argv[4])
LAST_VERSION = "14.0"
PROPS_VALUE_FILE = os.path.join(DIR_WITH_UCD, "PropertyValueAliases.txt")
BINARY_PROPS_FILE = os.path.join(DIR_WITH_UCD, "binary_props.txt")

EMOJI_PROPERTIES  = ["emoji", "emoji_presentation", "emoji_modifier", "emoji_modifier_base", "emoji_component", "extended_pictographic"]

def cp_code(cp):
    try:
        return int(cp, 16)
    except:
        return 0
def to_hex(cp, n = 6):
    return "{0:#0{fill}X}".format(int(cp), fill=n).replace('X', 'x')

def age_name(age):
    if age == 'unassigned':
        return age
    return "v" + str(age).replace(".", '_')

class ucd_cp(object):
    def __init__(self, cp, char):
        self.cp  = cp
        self.gc  = char.get("gc").lower()
        self.age = char.get("age")
        self.reserved = False
        if self.gc in ['co', 'cn', 'cs']:
            self.reserved = True
            return



        self.props = {}
        self.casing_props = {}
        self.casing = {}
        self.scx = [s.lower() for s in char.get("scx").split(" ")]
        self.sc  = char.get("sc").lower()
        self.scx.sort()
        if [self.sc] != self.scx:
            self.scx = [self.sc] + self.scx



        self.block = char.get("blk").lower().replace("-", "_").replace(" ", "_")
        self.nv = None if char.get("nv") == 'NaN' else char.get("nv").split("/")
        for p in [ "AHex",
            "Alpha",
            "Bidi_C",
            "Bidi_M",
            "Cased",
            "CE",
            "CI",
            "Comp_Ex",
            "Dash",
            "Dep",
            "DI",
            "Dia",
            "Ext",
            "Gr_Base",
            "Gr_Ext",
            "Hex",
#            "Hyphen",
            "IDC",
            "Ideo",
            "IDS",
            "IDSB",
            "IDST",
            "Join_C",
            "LOE",
            "Lower",
            "Math",
            "NChar",
            "OAlpha",
            "ODI",
            "OGr_Ext",
            "OIDC",
            "OIDS",
            "OLower",
            "OMath",
            "OUpper",
            "Pat_Syn",
            "Pat_WS",
            "PCM",
            "QMark",
            "Radical",
            "RI",
            "SD",
            "STerm",
            "Term",
            "UIdeo",
            "Upper",
            "VS",
            "WSpace",
            "XIDC",
            "XIDS",
            "XO_NFC",
            "XO_NFD",
            "XO_NFKC",
            "XO_NFKD" ]:
                if  char.get(p) == 'Y':
                    self.props[p.lower()] =  True

        for p in EMOJI_PROPERTIES :
            self.props[p] = False

        # Casing properties
        for p in [ "CWL", "CWT", "CWU", "CWCF"]:
             if  char.get(p) == 'Y':
                self.casing_props[p.lower()] =  True

        # Case mapping
        for p in [ "uc", "lc", "tc", "cf"]:
             if char.get(p) != '#':
                self.casing[p] = [cp_code(x) for x in char.get(p).split(" ")]



def write_string_array(f, array_name, strings_with_idx):
    dct = dict(strings_with_idx)
    f.write("static constexpr string_with_idx {}[]  = {{".format(array_name))
    keys = list(filter(None,  dct.keys()))
    keys.sort()

    for idx, key in enumerate(keys):
        f.write('string_with_idx{{ "{}", {} }} {}'.format(key, dct[key], "," if idx < len(keys) -1 else ""))
    f.write("};\n")

def get_scripts_names():
    scripts = []
    regex = re.compile("^sc\\s*;\\s*(\\w+)\\s*;\\s*(\\w+).*$")
    lines = [line.rstrip('\n') for line in open(PROPS_VALUE_FILE, 'r')]
    for line in lines:
        m = regex.match(line)
        if m:
            scripts.append((m.group(1).lower(), m.group(2).lower()))
    return scripts

def get_blocks_names():
    blocks = []
    regex = re.compile("^blk\\s*;\\s*(\\w+)\\s*;\\s*(\\w+)(?:\\s*;\\s*(\\w+))?.*$")
    lines = [line.rstrip('\n') for line in open(PROPS_VALUE_FILE, 'r')]
    for line in lines:
        m = regex.match(line)
        if m:
            blocks.append((m.group(1).lower(), m.group(2).lower(), m.group(3).lower() if len(m.groups()) > 3 else None))
    return blocks

def get_cats_names():
    cats = []
    regex = re.compile("^gc\\s*;\\s+(\\w+)\\s+;\\s+(\\w+).*$")
    lines = [line.rstrip('\n') for line in open(PROPS_VALUE_FILE, 'r')]
    for line in lines:
        m = regex.match(line)
        if m:
            cats.append((m.group(1).lower(), m.group(2).lower()))
    return cats

class ucd_block:
    def __init__(self, block):
        self.first   = cp_code(block.get('first-cp'))
        self.last    = cp_code(block.get('last-cp'))
        self.name    = block.get('name').lower().replace("-", "_").replace(" ", "_")

def get_unicode_data(version = LAST_VERSION):
    doc = etree.iterparse(os.path.join(DIR_WITH_UCD, version, "ucd.nounihan.flat.xml"), events=['end'])
    characters = [None] * (0x110000)
    zfound = False
    blocks = []
    for _, elem in doc:
        if elem.tag in["{http://www.unicode.org/ns/2003/ucd/1.0}char", "{http://www.unicode.org/ns/2003/ucd/1.0}reserved", "{http://www.unicode.org/ns/2003/ucd/1.0}surrogate"]:
            f = elem.get("first-cp")
            l = elem.get("last-cp")

            if f != None and l != None: # Handle range
                f = cp_code(f)
                l = cp_code(l)
                template = ucd_cp(c, elem)
                for c in range(f, l +1):
                    template.cp = c
                    characters[c] = copy.copy(template)
                    #print(to_hex(c))
                continue
            c = elem.get("cp")
            if c == None:
                continue
            c = cp_code(c)
            if c == 0 and zfound:
                continue
            zfound = True
            characters[c] = ucd_cp(c, elem)
            #print(to_hex(c))
        elif elem.tag in["{http://www.unicode.org/ns/2003/ucd/1.0}block"]:
            blocks.append(ucd_block(elem))

        elem.clear()

    regex = re.compile(r"([0-9A-F]+)(?:\.\.([0-9A-F]+))?\s*;\s*([a-z_A-Z]+).*")
    lines = [line.rstrip('\n') for line in open(os.path.join(DIR_WITH_UCD, version, "emoji-data.txt"), 'r')]
    for line in lines:
        m = regex.match(line)
        if m:
            end = start = int(m.group(1), 16)
            if m.group(2) != None:
                end = int(m.group(2), 16)
            v = m.group(3).lower()
            if not v in EMOJI_PROPERTIES:
                continue
            for i in range(start, end + 1):
                if characters[i] != None and not characters[i].reserved:
                    characters[i].props[v] = True
    return (characters, blocks)

def write_enum_scripts(scripts_names, file):
    f.write("enum class script {")
    for i, script in enumerate(scripts_names):
        f.write(script[0] + ", //{}\n".format(i))
        if script[1] != script[0]:
            f.write(script[1] + " = " + script[0] +",")
    f.write("max };\n")

def write_script_data(characters, scripts_names, file):
    indexes = {}
    for i, script in enumerate(scripts_names):
        indexes[script[0]] = i
        indexes[script[1]] = i
    write_string_array(f, "scripts_names", [(script[0], idx) for idx, script in enumerate(scripts_names)]
                                           + [(script[1], idx) for idx, script in enumerate(scripts_names)])

    f.write("template <auto N> struct script_data;")

    character_map = dict((c.cp, c.scx) for c in characters)

    def write_block(idx, characters):
        f.write("template <> struct script_data<{}> {{".format(idx))
        f.write("static constexpr const compact_range scripts_data= {")
        prev = ''
        for cp in range(0x10FFFF):
            script = 'zzzz'
            if (cp in characters and len(characters[cp]) > idx):
                script = characters[cp][idx]
            if script != prev:
                f.write("{},".format(to_hex((cp << 8) | indexes[script], 10)))
            prev = script
        f.write("0xFFFFFFFF")
        f.write("};};")

    l = max([len(cp.scx) for cp in characters])

    for i in range(l):
        write_block(i, character_map)

    f.write("""
    template<auto N>
    constexpr script cp_script(char32_t cp) {
        if(cp > 0x10FFFF)
            return script::unknown;

        uni::script sc = static_cast<uni::script>(script_data<N>::scripts_data.value(cp, uint8_t(script::unknown)));
        return sc;
    }
    """)

    f.write("constexpr script get_cp_script(char32_t cp, int idx) {")
    f.write("switch(idx) {")
    for i in range(l):
        f.write("case {0}: return cp_script<{0}>(cp);".format(i))
    f.write("} return script::zzzz;}")


def write_enum_blocks(blocks_names, blocks, file):
    aliases = dict([(block[0], block[1:]) for block in blocks_names])
    aliases.update(dict([(block[1], block) for block in blocks_names]))
    all = set()
    file.write("enum class block { ")
    names = []
     # no_block needs to be first
    for i, block in enumerate(["no_block"] + [block.name for block in blocks]):
        f.write(block + ", // " + str(i) + "\n")
        all.add(block)
        names.append((block, i))
        if block in aliases:
            for alias in aliases[block]:
                if not alias or alias in all:
                    continue
                f.write(alias + " = " + block +",")
                all.add(alias)
                names.append((alias, i))
    file.write("__max };\n")
    return names

def write_blocks_data(indexed_names, blocks, file):
    write_string_array(file, "blocks_names", indexed_names)
    ## Blocks are stored as a compact range where
    ##  * if the low bit is 0, the range is not assigned to a block
    ##  * otherwise the low bit is an offset to substract from the index of the range entry to get the
    ##  value of the block as specified by the enum
    f.write("static constexpr const compact_range block_data = {")
    prev = -1
    offset = 0;
    for i, b in enumerate(blocks):
        if (b.first != prev + 1):
            f.write("{},".format(to_hex(((prev + 1) << 8), 10)))
            offset = offset + 1
        f.write("{},".format(to_hex(((b.first) << 8) | (offset + 1), 10)))
        prev = b.last
    f.write("0xFFFFFFFF")
    f.write("};\n")


def emit_bool_table(f, name, data):
    f.write("static constexpr flat_array<{}> {} {{{{".format(len(data), name))
    for idx, cp in enumerate(data):
        f.write(to_hex(cp, 6))
        if idx != len(data) - 1: f.write(",")
    f.write("}};")

def construct_range_data(data):
    prev = None
    prev_cp = 0
    elems = []
    minc = min(data)
    maxc = max(data)
    for i in range(0x110000):
        res = i >= minc and i <= maxc and i in data
        if prev == None or res != prev:
            if len(elems)!=0: elems[-1] = (elems[-1][0], elems[-1][1], i - prev_cp)
            elems.append((i, res, 0))
            prev = res
            prev_cp = i
    if len(elems) != 0: elems[-1] = (elems[-1][0], elems[-1][1], i - prev_cp)
    return len(elems) * 4, elems

def emit_bool_ranges(f, name, range_data):
    f.write("static constexpr range_array {} = {{".format(name))
    for idx, e in enumerate(range_data):
        f.write("{}".format(to_hex((e[0] << 8) | (1 if e[1] else 0), 10)))
        if idx != len(range_data) - 1: f.write(",")
    f.write("};")


def emit_trie_or_table(f, name, data):
    t = 'a'
    adata = data
    asize = len(data) * 4
    rdata = None
    rsize = 0xFFFFFFFF
    tdata = None
    tsize = 0xFFFFFFFF
    size  = asize

    if len(data):
        rsize, rdata = construct_range_data(data)
        tsize, tdata = construct_trie_data(data, 1)

    if rsize < size:
        t = 'r'
        size = rsize
    if size > 200:
        t = 't'
        size = tsize

    if t == 'a':
        emit_bool_table(f, name, adata)
    elif t == 'r':
        emit_bool_ranges(f, name, rdata)
    elif t == 't':
        emit_trie(f, name, tdata, 1)

    print("{} : {} element(s) - type: {} - size: {}  (array: {}, range: {}, trie : {}".format(name, len(data), t, size, asize, rsize, tsize))
    return size

def write_enum_categories(categories_names, file):
    f.write("enum class category {")
    for cat in categories_names:
        f.write(cat[0] + ",")
        f.write(cat[1] + " = " + cat[0] +",")
    f.write("max };\n")

def write_categories_data(characters, categories_names, file):

    write_string_array(f, "  categories_names", [(c[0], idx) for idx, c in enumerate(categories_names)]
                                              + [(c[1], idx) for idx, c in enumerate(categories_names)])

    size = 0
    cats = {}
    for cp in characters:
        if cp.gc == 'cn':
            continue
        if not cp.gc in cats:
            cats[cp.gc] = []
        cats[cp.gc].append(cp.cp)
    sorted_by_len = []
    for k, v in cats.items():
        size += emit_trie_or_table(f, "cat_{}".format(k), set(v))
        sorted_by_len.append((len(v), k))
    meta_cats = {
        "cased_letter" : ['lu', 'lt', 'll'],
        "letter" : ['lu', 'lt', 'll', 'lm', 'lo'],
        "mark"   : ['mn', 'mc', 'me'],
        "number" : ['nd', 'nl', 'no'],
        "punctuation" :  ['pc', 'pd', 'ps', 'pe', 'pi', 'pf', 'po'],
        "symbol" : ['sm', 'sc', 'sk', 'so'],
        "separator" : ['zs', 'zl', 'zp'],
        "other"    : ['cc', 'cf', 'cs', 'co', 'cn']
    }

    sorted_by_len.sort(reverse=True)

    f.write("""
    constexpr category get_category(char32_t c) {
    """)
    for _, cat in sorted_by_len:
        f.write("if (cat_{0}.lookup(c)) return category::{0};".format(cat))
    f.write("return category::cn;}")


    f.write("}")

    for _, cat in sorted_by_len:
        f.write("""template <>
        constexpr bool cp_category_is<category::{0}>(char32_t c) {{
            return detail::tables::cat_{0}.lookup(c); }}
        """.format(cat))

    for name, cats in meta_cats.items():
        f.write("""template <>
        constexpr bool cp_category_is<category::{}>(char32_t c) {{
            """.format(name))
        for _, cat in sorted_by_len:
            if cat in cats:
                f.write("if(detail::tables::cat_{0}.lookup(c)) return true;".format(cat))
        f.write("return false;}")

    f.write("""
        template <>
        constexpr bool cp_category_is<category::unassigned>(char32_t c) {
            return cp_category(c) == category::unassigned;
        }
    """
    )

    f.write("namespace detail::tables {")
    print("total size : {}".format(size / 1024.0))

def characters_ages(characters):
    return sorted(list(set([float(cp.age) for cp in characters if cp.age != 'unassigned'])))

def write_enum_age(characters, f):
    ages = characters_ages(characters)
    f.write("enum class version : uint8_t {")
    f.write("unassigned,")
    for i, age in enumerate(ages):
        f.write(age_name(age) + ",")
    f.write("latest_version = {}".format(age_name(LAST_VERSION)))
    f.write("};")

def write_age_data(characters, f):
    ages = characters_ages(characters)
    indexes = {}
    indexes["unassigned"] = 0
    for i, age in enumerate(ages):
        indexes[age_name(age)] = i+1;


    f.write("static constexpr const char* age_strings[] = { \"unassigned\",")
    for idx, age in enumerate(ages):
        f.write('"{}"{}'.format(age, "," if idx < len(ages) - 1 else ""))
    f.write("};\n")

    f.write("static constexpr compact_range age_data = {")
    known  = dict([(cp.cp, age_name(cp.age)) for cp in characters])
    prev  = ""
    size = 0
    for cp in range(0, 0x10FFFF):
        age = known[cp] if cp in known else 'unassigned'
        if prev != age:
            f.write("{},".format(to_hex((cp << 8) | indexes[age], 10)))
            size = size + 1
            prev = age
    f.write("0xFFFFFFFF};\n")
    print(size)

def write_numeric_data(characters, f):

    values = dict({
        "8"   : [],
        "16"  : [],
        "32"  : [],
        "64"  : []
    })
    dvalues = []

    for cp in characters:
        if not cp.nv:
            continue
        n = int(cp.nv[0])
        d = int(cp.nv[1] if len(cp.nv) == 2 else '1')
        if d != 1:
            dvalues.append((cp.cp, d))
        if abs(n) > 2147483647:
            values["64"].append((cp.cp, n))
        elif abs(n) > 32767:
            values["32"].append((cp.cp, n))
        elif n < 0 or n >= 127 :
            values["16"].append((cp.cp, n))
        else:
            values["8"].append((cp.cp, n))

    for s, characters in values.items():
        if s == '8' :
            f.write("static constexpr compact_list numeric_data8 = {")
        else:
            f.write("static constexpr uni::detail::pair<char32_t, int{0}_t> numeric_data{0}[] = {{ ".format(s))


        for idx, cp in enumerate(characters):
            if s == '8' :
                f.write("{}{}".format(to_hex(((cp[0] << 8) | cp[1]) , 10), ',' if idx < len(characters) - 1 else ''))
            else:
                f.write("uni::detail::pair<char32_t, int{}_t> {{ {}, {} }},".format(s, to_hex(cp[0], 6), cp[1]))
        f.write("};")

    f.write("static constexpr uni::detail::pair<char32_t, int16_t> numeric_data_d[] = {")
    for cp in dvalues:
        f.write("uni::detail::pair<char32_t, int16_t> {{ {}, {} }},".format(to_hex(cp[0], 6), cp[1]))
    f.write("uni::detail::pair<char32_t, int16_t>{0x110000, 0} };\n")


def write_binary_properties(characters, f):

    unsupported_props = [
        "gr_link", # Grapheme_Link is deprecated
        "hyphen",  # Hyphen is deprecated
        "xo_nfc", # Expands_On_NFC is deprecated
        "xo_nfd", # Expands_On_NFD is deprecated
        "xo_nfkc", # Expands_On_NFKC is deprecated
        "xo_nfkd", # Expands_On_NFKD is deprecated

        "cwcf",  # Changes_When_Casefolded
        "cwcm",  # Changes_When_Casemapped
        "cwkcf", # Changes_When_NFKC_Casefolded
        "cwl",   # Changes_When_Lowercased
        "cwt",   # Changes_When_Titlecased
        "cwu",   # Changes_When_Uppercased
    ]

    details = [
        "ce",
        "comp_ex",
        "oids",
        "odi",
        "oalpha",
        "ogr_ext",
        "oidc",
        "olower",
        "omath",
        "oupper"
    ]

    custom_impl = [
        "cased",
        "ci",
        "di",
        "idc",
        "ids",
        "lower",
        "upper",
        "math",
        "nchar",
        "gr_ext"
    ]

    props = []

    lines = [line.rstrip('\n') for line in open(BINARY_PROPS_FILE, 'r')]

    values = []
    for ep in EMOJI_PROPERTIES:
        values.append([ep])
    for line in lines:
        values.append([n.strip().lower().replace(" ", "_").replace("-", "_") for n in line.split(";")])

    values.sort()

    known  = set()
    f.write("enum class property {")
    for v in values:
        if v[0] in known or v[0] in unsupported_props:
            continue
        props.append(v[0])
        if v[0] in details:
            continue
        f.write("{} ,".format(v[0]))
        known.add(v[0])
        for alias in v[1:]:
            if not alias in known:
                f.write("{} = {} ,".format(alias, v[0]))
                known.add(alias)
    f.write("max };\n")


    f.write("namespace detail::tables {")
    for prop in props:
        if not prop in custom_impl:
            emit_binary_data(f, "prop_{}_data".format(prop), characters, lambda c : prop in c.props and c.props[prop])
    f.write("}")


    for prop in props:
        if not prop in custom_impl and not prop in details:
            f.write("template <> constexpr bool cp_property_is<property::{0}>(char32_t c) {{ return detail::tables::prop_{0}_data.lookup(c); }}".format(prop))


    return [prop for prop in values if not prop[0] in details and not prop[0] in unsupported_props]


def write_regex_support(f, characters, supported_properties, categories_names, scripts_names):
    all = supported_properties + [["any"], ["ascii"], ["assigned"]] + categories_names + scripts_names

    d = collections.OrderedDict()
    for p in all:
        d[p[0]] = p


    f.write("enum class binary_prop {")
    for p in d.keys():
        f.write("{} ,".format(p))
    f.write("unknown };\n")

    for prop in supported_properties:
        f.write("""
        template<>
        constexpr bool get_binary_prop<binary_prop::{0}>(char32_t c) {{
            return cp_property_is<property::{0}>(c);
        }}
    """.format(prop[0]))


    for cat in categories_names:
        f.write("""
        template<>
        constexpr bool get_binary_prop<binary_prop::{0}>(char32_t c) {{
            return cp_category_is<category::{0}>(c);
        }}
    """.format(cat[0]))

    for script in scripts_names:
        f.write("""
        template<>
        constexpr bool get_binary_prop<binary_prop::{0}>(char32_t c) {{
            return cp_script(c) == script::{0};
        }}
    """.format(script[0]))

    names = []
    for idx, (_, aliases) in enumerate(d.items()):
        names = names + [(n, idx) for n in aliases]

    f.write("namespace tables{")
    write_string_array(f, "binary_prop_names", names)
    f.write("}")


def emit_binary_data(f, name, characters, pred):
    d = set([c.cp for c in filter(pred, characters)])
    emit_trie_or_table(f, name, d)

# To store the casing data (for fullmapping casing),
# create a single array of char32 -> char32
# where the array starts with all the first codepoint of each mapping
# then the seconds, etc
# A second list "casing_data_starts" indicate where each "level"
# begins in the primary array
# To speed up lookup the higest bit of each but the last codepoint of mapped-to
# sequence is set
# we also record the size of the longest mapping
def emit_casing_data(f, prop_name, characters):
    starts = [0]
    end = 1
    m = max([len(c.casing[p]) for c in characters])
    count = len(characters)
    f.write("constexpr char32_t casing_{}_data[][2] = {{".format(prop_name))
    for i in range(0, m):
        for c in characters:
            if not prop_name in c.casing or i >= len(c.casing[prop_name]):
                continue
            hasNext = i+1 < len(c.casing[prop_name])
            value = c.casing[prop_name][i]
            if hasNext:
                value |= (1 << 31)
            f.write("{{ {}, {} }},".format(to_hex(c.cp), to_hex(value)))
            end = end+1
        starts.append(end)
    f.write("{0, 0}};\n")
    f.write("constexpr std::size_t  casing_{}_starts[] ={{ {} }};".format(prop_name,
            ",".join([to_hex(s) for s in starts])))
    f.write("constexpr std::size_t  casing_{}_length = {};".format(prop_name, m))
    print("Casing data for {}: max: {}, count: {}, bytes: {}".format(prop_name, m, count, end))

if __name__ == "__main__":

    scripts_names = get_scripts_names()
    block_names = get_blocks_names()
    all_characters, blocks = get_unicode_data()
    characters = sorted(list(filter(lambda c : c != None, all_characters)), key=lambda c: c.cp)

    categories_name = get_cats_names()

    with open(sys.argv[1], "w") as f:
        f.write("""
#pragma once
#include "cedilla/synopsis.hpp"
#include "cedilla/base.h"
namespace uni {
""")

        write_enum_age(characters, f)
        write_enum_categories(categories_name,f)
        indexed_block_name = write_enum_blocks(block_names, blocks,f)
        write_enum_scripts(scripts_names, f)



        f.write("namespace detail::tables {")

        print("Age data")
        write_age_data(characters, f)

        print("Category data")
        write_categories_data(characters, categories_name,f)

        print("Block data")
        write_blocks_data(indexed_block_name, blocks, f)

        characters = list(filter(lambda c: not c.reserved, characters))

        print("Script data")
        write_script_data(characters, scripts_names, f)

        print("Numeric Data")
        write_numeric_data(characters, f)


        print("Binary properties")
        emit_binary_data(f, "prop_assigned", characters, lambda c : True)

        # exit detail ns
        f.write("}")
        supported_properties = write_binary_properties(characters, f)

        f.write("}//namespace uni")

    with open(sys.argv[2], "w") as f:
        f.write("""
#pragma once
#include "cedilla/synopsis.hpp"
#include "cedilla/base.h"
namespace uni::detail {
""")
        write_regex_support(f, characters, supported_properties, categories_name, scripts_names)
        f.write("}\n\n")

    with open(sys.argv[3], "w") as f:
        f.write("""
#pragma once
#include "cedilla/base.h"
namespace uni::detail {
""")
        for prop in ["cwl", "cwt", "cwu", "cwcf"]:
            emit_binary_data(f, "{}_data".format(prop), characters,
            lambda c : prop in c.casing_props)

        for p in [ "uc", "lc", "tc", "cf"]:
            emit_casing_data(f, p, [c for c in characters if p in c.casing])

        f.write("}\n\n")
