#!/usr/bin/python3

import xml.etree.cElementTree as etree
import sys
import os
import re
import collections
#from intbitset import intbitset

DIR = os.path.dirname(os.path.realpath(__file__))
LAST_VERSION = "12.0"
STANDARD_VERSION = "12.0"
SUPPORTED_VERSIONS = ["11.0", "10.0"] # "9.0", "8.0", "7.0"]
MIN_VERSION = "10.0"
PROPS_VALUE_FILE = os.path.join(DIR, "ucd", "PropertyValueAliases.txt")
BINARY_PROPS_FILE = os.path.join(DIR, "ucd", "binary_props.txt")

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
    def __init__(self, char):
        self.cp  = cp_code(char.get('cp'))
        self.scx = [s.lower() for s in char.get("scx").split(" ")]
        self.sc  = char.get("sc").lower()
        if [self.sc] != self.scx:
            self.scx = [self.sc] + self.scx
        self.gc  = char.get("gc").lower()
        self.age = char.get("age")
        self.block = char.get("blk").lower().replace("-", "_").replace(" ", "_")
        self.nv = None if char.get("nv") == 'NaN' else char.get("nv").split("/")
        self.props = {}
        for p in [ "AHex",
            "Alpha",
            "Bidi_C",
            "Bidi_M",
            "Cased",
            "CE",
            "CI",
            "Comp_Ex",
#            "CWCF",
#            "CWCM",
#            "CWKCF",
#            "CWL",
#            "CWT",
#            "CWU",
            "Dash",
            "Dep",
            "DI",
            "Dia",
            "Ext",
            "Gr_Base",
            "Gr_Ext",
#            "Gr_Link",
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
                self.props[p.lower()] =  char.get(p) == 'Y'

        for p in EMOJI_PROPERTIES :
            self.props[p] = False


def write_string_array(f, array_name, strings_with_idx):
    dct = dict(strings_with_idx)
    f.write("static constexpr const std::array {}  = {{".format(array_name))
    keys = list(filter(None,  dct.keys()))
    keys.sort()

    for idx, key in enumerate(keys):
        f.write('__string_with_idx{{ "{}", {} }} {}'.format(key, dct[key], "," if idx < len(keys) -1 else ""))
    f.write("};\n")

def get_scripts_names():
    scripts = []
    regex = re.compile("^sc\\s*;\\s*(\\w+)\\s*;\\s*(\\w+).*$")
    lines = [line.rstrip('\n') for line in open(PROPS_VALUE_FILE, 'r')]
    for line in lines:
        m = regex.match(line)
        if m:
            #long, short names
            scripts.append((m.group(2).lower(), m.group(1).lower()))
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
            #long, short names
            cats.append((m.group(2).lower(), m.group(1).lower()))
    return cats

class ucd_block:
    def __init__(self, block):
        self.first   = cp_code(block.get('first-cp'))
        self.last    = cp_code(block.get('last-cp'))
        self.name    = block.get('name')

def get_unicode_data(version = LAST_VERSION):
    root = etree.parse(os.path.join(DIR, "ucd", version, "ucd.nounihan.flat.xml")).getroot()
    repertoire = root.find("{http://www.unicode.org/ns/2003/ucd/1.0}repertoire")
    characters = [None] * (0x110000)
    zfound = False
    for elem in repertoire:
        if elem.tag != '{http://www.unicode.org/ns/2003/ucd/1.0}char':
            continue
        cp = ucd_cp(elem)
        if cp.cp == 0 and zfound:
            continue
        zfound = True
        characters[cp.cp] = cp
    blocks_node = root.find("{http://www.unicode.org/ns/2003/ucd/1.0}blocks")
    blocks = []
    for elem in blocks_node:
        if elem.tag != '{http://www.unicode.org/ns/2003/ucd/1.0}block':
            continue
        blocks.append(ucd_block(elem))

    regex = re.compile(r"([0-9A-F]+)(?:\.\.([0-9A-F]+))\s*;\s*([a-z_A-Z]+).*")
    lines = [line.rstrip('\n') for line in open(os.path.join(DIR, "ucd", version, "emoji-data.txt"), 'r')]
    for line in lines:
        m = regex.match(line)
        if m:
            start = int(m.group(1), 16)
            end   = int(m.group(2), 16) if m.groups(2) != None else start
            v = m.group(3).lower()
            if not v in EMOJI_PROPERTIES:
                continue
            for i in range(start, end + 1):
                if characters[i] != None:
                    characters[i].props[v] = True
    return (characters, blocks)


def write_script_data(characters, changed, scripts_names, file):
    f.write("enum class script {")
    for i, script in enumerate(scripts_names):
        f.write(script[0] + ", //{}\n".format(i))
        if script[1] != script[0]:
            f.write(script[1] + " = " + script[0] +",")
    f.write("__max };\n")

    write_string_array(f, "__scripts_names", [(script[0], idx) for idx, script in enumerate(scripts_names)]
                                           + [(script[1], idx) for idx, script in enumerate(scripts_names)])

    f.write("struct __script_data_t {char32_t first; script s;};")
    f.write("template <auto N> struct __script_data;")

    def write_block(idx, characters, changed):
        f.write("template <> struct __script_data<{}> {{".format(idx))
        f.write("static constexpr const std::array scripts_data= {")
        prev = 'zzzz'
        block = dict([(cp.cp, cp.scx) for cp in characters])
        for cp in range(0x10FFFF):
            script = block[cp][idx] if (cp in block and len(block[cp])) > idx else 'zzzz'
            if script != prev:
                f.write("__script_data_t{{ {}, script::{} }},".format(to_hex(cp, 6), script))
            prev = script
        f.write("__script_data_t{0x110000, script::zzzz }")
        f.write("};")

        modified = block
        data = []
        for k, v in changed.items():
            old = collections.OrderedDict([(cp.cp, cp.scx) for cp in v if not cp.cp in modified or cp.scx != modified[cp.cp]])
            for cp, v in old.items():
                if len(v) < len(modified[cp]):
                    old[cp].append('zzzz')
            old = [(cp, scx) for cp, scx in old.items() if len(scx) > idx and (len(modified[cp]) <= idx or scx[idx] != modified[cp][idx])]

            if len(old) > 0:
                data.append((age_name(k), old))
            modified.update(dict(old))

        for version, d in data:
            f.write("static constexpr const std::array scripts_data_compat_{} = {{".format(version))
            for i, (cp, script) in enumerate(d):
                f.write("std::pair<char32_t, script>{{ {}, script::{} }} {}".format(cp, script[idx], "," if i < len(d) - 1 else ""))
            f.write("};")
        f.write("template <uni::version v> constexpr script older_cp_script(char32_t cp, script c) {")
        for version, _ in data:
            f.write("""
                if constexpr(v <= uni::version::{0}) {{
                    const auto it = uni::lower_bound(scripts_data_compat_{0}.begin(), scripts_data_compat_{0}.end(), cp,
                        [](const auto & e, char32_t cp) {{ return e.first < cp; }});
                    if (it != scripts_data_compat_{0}.end() && cp == it->first)
                    c = it->second;
                }}
    """.format(version))
        f.write("return c;}")
        f.write("};")

    l = max([len(cp.scx) for cp in characters])
    for _, v in changed.items():
        m = max([len(cp.scx) for cp in v])
        if m > l: l = m

    for i in range(l):
        write_block(i, characters, changed)

    f.write("""
    template<auto N, uni::version v = uni::version::standard_unicode_version>
    constexpr script __cp_script(char32_t cp) {
    if(cp > 0x10FFFF)
        return script::unknown;

    constexpr const auto end = __script_data<N>::scripts_data.end();
    auto it = uni::upper_bound(__script_data<N>::scripts_data.begin(), end, cp,
                               [](char32_t cp, const __script_data_t& s) { return cp < s.first; });
    if(it == end)
        return script::unknown;
    it--;
    if constexpr(v < uni::version::v12_0) {
        return __script_data<N>::older_cp_script(cp, it->s);
    }
    return it->s;
    }
    """)

    f.write("template<uni::version v = uni::version::standard_unicode_version>")
    f.write("script __get_cp_script(char32_t cp, int idx) {")
    f.write("switch(idx) {")
    for i in range(l):
        f.write("case {0}: return __cp_script<{0}, v>(cp);".format(i))
    f.write("} return script::zzzz;}")


def write_blocks_data(blocks_names, blocks, file):
    f.write("enum class block {")
    for block in blocks_names:
        f.write(block[0] + ",")
        if block[1] != block[0]:
            f.write(block[1] + " = " + block[0] +",")
    f.write("__max };\n")

    write_string_array(f, "__blocks_names",  [(b[0], idx) for idx, b in enumerate(blocks_names)]
                                           + [(b[1], idx) for idx, b in enumerate(blocks_names)]
                                           + [(b[2], idx) for idx, b in enumerate(blocks_names)])

    f.write("\nstruct __block_data_t { uint32_t first; block b;};\n")
    f.write("static constexpr const std::array __block_data = {")


    known  = dict([(cp.cp, cp.block) for cp in characters])
    prev  = ""
    size = 0
    for cp in range(0, 0x10FFFF):
        block = known[cp] if cp in known else 'no_block'
        if prev != block:
            f.write("__block_data_t{{ {}, block::{} }},".format(to_hex(cp, 6), block))
            size = size + 1
            prev = block
    f.write("__block_data_t{0x110000, block::no_block} };\n")


def compute_trie(rawdata, chunksize):
    root = []
    childmap = {}
    child_data = []
    for i in range(len(rawdata) // chunksize):
        data = rawdata[i * chunksize: (i + 1) * chunksize]
        child = '|'.join(map(str, data))
        if child not in childmap:
            childmap[child] = len(childmap)
            child_data.extend(data)
        root.append(childmap[child])
    return (root, child_data)


def construct_bool_trie_data(data):
    CHUNK = 64
    rawdata = [False] * 0x110000
    for cp in data:
        rawdata[cp] = True


    def trim(arr):
        import numpy
        a = numpy.trim_zeros(arr, 'f')
        f = len(arr) - len(a)
        c = numpy.trim_zeros(a, 'b')
        b = len(a) - len(c)
        return (c, f, b)


    # convert to bitmap chunks of 64 bits each
    chunks = []
    for i in range(0x110000 // CHUNK):
        chunk = 0
        for j in range(64):
            if rawdata[i * 64 + j]:
                chunk |= 1 << j
        chunks.append(chunk)


    r1 = chunks[0:0x800 // CHUNK]
    if len([x for x in r1 if x == 0]) == len(r1):
        r1 = []


    # 0x800..0x10000 trie
    (r2, r3) = compute_trie(chunks[0x800 // CHUNK : 0x10000 // CHUNK], 64 // CHUNK)
    if len([x for x in r3 if x == 0]) == len(r3):
        r2 = []
        r3 = []

    #remove leading and trailing
    r2 = trim(r2)

    # 0x10000..0x110000 trie
    (mid, r6) = compute_trie(chunks[0x10000 // CHUNK : 0x110000 // CHUNK], 64 // CHUNK)
    if len([x for x in r6 if x == 0]) == len(r6):
        r6 = []
        r4 = []
        r5 = []
    else:
        (r4, r5)  = compute_trie(mid, 64)

    r4 = trim(r4)
    r5 = trim(r5)

    size = len(r1) * 8 + len(r2) + len(r3) * 8 + len(r4) + len(r5) + len(r6) * 8
    return (size, (r1, r2, r3, r4, r5, r6))

def emit_bool_trie(f, name, trie_data):

    r1data = ','.join('0x%016x' % chunk for chunk in trie_data[0])
    r2data = ','.join(str(node) for node in trie_data[1][0])
    r3data = ','.join('0x%016x' % chunk for chunk in  trie_data[2])
    r4data = ','.join(str(node) for node in trie_data[3][0])
    r5data = ','.join(str(node) for node in trie_data[4][0])
    r6data = ','.join('0x%016x' % chunk for chunk in trie_data[5])
    f.write("static constexpr __bool_trie<{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}> {} {{".format(
        len(trie_data[0]),      #r1
        len(trie_data[1][0]),   #r2
        trie_data[1][1],
        trie_data[1][2],
        len(trie_data[2]),      #r3
        len(trie_data[3][0]),   #r4
        trie_data[3][1],
        trie_data[3][2],
        len(trie_data[4][0]),   #r5
        trie_data[4][1],
        trie_data[4][2],
        len(trie_data[5]),      #r6
        name))
    f.write("{{ {} }}, {{ {} }}, {{ {} }}, {{ {} }}, {{ {} }}, {{ {} }}".format(r1data, r2data, r3data, r4data, r5data, r6data))
    f.write("};")


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
    f.write("static constexpr __range_array<{}> {} = {{ {{".format(len(range_data), name))
    for idx, e in enumerate(range_data):
        f.write("__range_array_elem{{ {}, {} }} /*{}*/".format(to_hex(e[0], 6), 1 if e[1] else 0, e[2]))
        if idx != len(range_data) - 1: f.write(",")
    f.write("}};")


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
        tsize, tdata = construct_bool_trie_data(data)

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
        emit_bool_trie(f, name, tdata)

    print("{} : {} element(s) - type: {} - size: {}  (array: {}, range: {}, trie : {}".format(name, len(data), t, size, asize, rsize, tsize))
    return size

def write_categories_data(characters, changed, categories_names, file):
    f.write("enum class category {")
    for cat in categories_names:
        f.write(cat[0] + ",")
        f.write(cat[1] + " = " + cat[0] +",")
    f.write("__max };\n")

    write_string_array(f, "__categories_names", [(c[0], idx) for idx, c in enumerate(categories_names)]
                                              + [(c[1], idx) for idx, c in enumerate(categories_names)])

    size = 0
    cats = {}
    for cp in characters:
        if not cp.gc in cats:
            cats[cp.gc] = []
        cats[cp.gc].append(cp.cp)
    sorted_by_len = []
    for k, v in cats.items():
        size += emit_trie_or_table(f, "__cat_{}".format(k), v)
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
    template<uni::version v = uni::version::standard_unicode_version>
    constexpr category cp_category(char32_t cp);
    template<version v>
    constexpr category __get_category_for_version(char32_t, category c);
    template <version v>
    constexpr category __get_category(char32_t c) {
    if(auto cat = __get_category_for_version<v>(c, category::cn); cat != category::cn)
        return cat;

    """)
    for _, cat in sorted_by_len:
        f.write("if (__cat_{0}.lookup(c)) return category::{0};".format(cat))
    f.write("return category::cn;}")


    for _, cat in sorted_by_len:
        f.write("""template <category category_, version v = uni::version::standard_unicode_version, std::enable_if_t<category_ == category::{0}, int> = 0>
        constexpr bool cp_is(char32_t c) {{
            return __get_category_for_version<v>(c, __cat_{0}.lookup(c) ?  category::{0} : category::cn) == category::{0}; }}
        """.format(cat))

    for name, cats in meta_cats.items():
        f.write("""template <category category_, version v = uni::version::standard_unicode_version, std::enable_if_t<category_ == category::{}, int> = 0>
        constexpr bool cp_is(char32_t c) {{
            category cat = category::cn;
            """.format(name))
        for _, cat in sorted_by_len:
            if cat in cats:
                f.write("if(cat == category::cn && __cat_{0}.lookup(c)) {{ if constexpr(v == uni::version::latest_version) return true; cat = category::{0}; }}".format(cat))
        f.write("cat =  __get_category_for_version<v>(c, cat); return ")
        for _, cat in sorted_by_len:
            if cat in cats:
                f.write("cat == category::{0} ||".format(cat))
        f.write("true;}")


    #changed characters


    print("total size : {}".format(size / 1024.0))
    with_data = []
    modified = dict([(cp.cp, cp.gc) for cp in characters])
    for k, v in changed.items():
        old =  [(cp.cp, cp.gc) for cp in v if cp.gc != modified[cp.cp]]
        if len(old) == 0:
            continue
        f.write("static constexpr std::array __cat_version_data_{} {{".format(age_name(k)))
        for idx, (cp, gc) in enumerate(old):
            f.write("std::pair{{ {}, category::{} }}{}".format(to_hex(cp, 6), gc, "," if idx < len(old) - 1 else ""))
        f.write("};")
        modified.update(old)
        with_data.append(k)

    f.write("template <version v> constexpr category __get_category_for_version(char32_t cp, category c) {")
    for v in with_data:
        f.write("""
            if constexpr( v<= uni::version::{0} ) {{
                const auto it = uni::lower_bound(__cat_version_data_{0}.begin(), __cat_version_data_{0}.end(), cp,
                [](const auto & e, char32_t cp) {{ return e.first < cp; }});
            if (it != __cat_version_data_{0}.end() && cp == it->first)
                c = it->second;
        }}""".format(age_name(v)))
    f.write("return c; }")

def write_age_data(characters, f):
    ages = list(set([float(cp.age) for cp in characters if cp.age != 'unassigned']))
    ages.sort()
    f.write("enum class version : uint8_t {")
    f.write("unassigned,")
    for age in ages:
        f.write(age_name(age) + ",")
    f.write("standard_unicode_version = {},".format(age_name(STANDARD_VERSION)))
    f.write("minimum_version = {},".format(age_name(MIN_VERSION)))
    f.write("latest_version = {}".format(age_name(LAST_VERSION)))
    f.write("};")


    f.write("static constexpr std::array __age_strings = { \"unassigned\",")
    for idx, age in enumerate(ages):
        f.write('"{}"{}'.format(age, "," if idx < len(ages) - 1 else ""))
    f.write("};\n")

    f.write("\nstruct __age_data_t { char32_t first; version a;};\n")
    f.write("static constexpr std::array __age_data = {")
    known  = dict([(cp.cp, age_name(cp.age)) for cp in characters])
    prev  = ""
    size = 0
    for cp in range(0, 0x10FFFF):
        age = known[cp] if cp in known else 'unassigned'
        if prev != age:
            f.write("__age_data_t{{ {}, version::{} }},".format(to_hex(cp, 6), age))
            size = size + 1
            prev = age
    f.write("__age_data_t{0x110000, version::unassigned} };\n")
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
        if abs(n > 2147483647):
            values["64"].append((cp.cp, n))
        elif abs(n > 32767):
            values["32"].append((cp.cp, n))
        elif abs(n > 127):
            values["16"].append((cp.cp, n))
        else:
            values["8"].append((cp.cp, n))

    for s, characters in values.items():
        f.write("static constexpr std::array __numeric_data{} = {{ ".format(s))
        for cp in characters:
            f.write("std::pair<char32_t, int{}_t> {{ {}, {} }},".format(s, to_hex(cp[0], 6), cp[1]))
        f.write("std::pair<char32_t, int{}_t>{{0x110000, 0}} }};\n".format(s))

    f.write("static constexpr std::array __numeric_data_d = {")
    for cp in dvalues:
        f.write("std::pair<char32_t, int16_t> {{ {}, {} }},".format(to_hex(cp[0], 6), cp[1]))
    f.write("std::pair<char32_t, int16_t>{0x110000, 0} };\n")


def write_binary_properties(characters, f):

    unsupported_props = [
        "gr_link", # Grapheme_Link is deprecated
        "hyphen",  # Hyphen is deprecated
        "xo_nfc", # Expands_On_NFC is deprecated
        "xo_nfd", # Expands_On_NFD is deprecated
        "xo_nfkc", # Expands_On_NFKC is deprecated
        "xo_nfkd", # Expands_On_NFKD is deprecated
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

        "cwcf",  # Changes_When_Casefolded
        "cwcm",  # Changes_When_Casemapped
        "cwkcf", # Changes_When_NFKC_Casefolded
        "cwl",   # Changes_When_Lowercased
        "cwt",   # Changes_When_Titlecased
        "cwu",   # Changes_When_Uppercased
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
    f.write("__max };\n")


    for prop in props:
        if not prop in custom_impl:
            emit_binary_data(f, "__prop_{}_data".format(prop), characters, lambda c : c.props[prop])

    for prop in props:
        if not prop in custom_impl and not prop in details:
            f.write("template <> constexpr bool cp_is<property::{0}>(char32_t c) {{ return __prop_{0}_data.lookup(c); }}".format(prop))

def write_regex_support(f, characters, categories_names, scripts_names):
    lines = [line.rstrip('\n') for line in open(BINARY_PROPS_FILE, 'r')]
    values = [["any"], ["ascii"], ["assigned"]]
    known  = set()

    for line in lines:
        values.append([n.strip().lower().replace(" ", "_").replace("-", "_") for n in line.split(";")])
    for cat in categories_names:
        values.append(cat)
    for script in scripts_names:
        values.append(script)
    values.sort(key = lambda x: x[0])

    f.write("enum class __binary_prop {")
    for v in values:
        if not v[0] in known:
            f.write("{} ,".format(v[0]))
            known.add(v[0])
            for alias in v[1:]:
                if not alias in known:
                    f.write("{} = {} ,".format(alias, v[0]))
                    known.add(alias)
    f.write("__max };\n")

    f.write("""
        template<uni::__binary_prop p>
        constexpr bool get_binary_prop(char32_t) = delete;
        //Forward declared - defined in unicode.h
        constexpr script cp_script(char32_t cp);
    """)

    cats = []
    for cp in characters:
        if not cp.gc in cats:
            cats.append(cp.gc)

    for cat in cats:
        f.write("""
        template<>
        constexpr bool get_binary_prop<__binary_prop::{0}>(char32_t c) {{
            return __cat_{0}.lookup(c);
        }}
    """.format(cat))

    for script in scripts_names:
        if len(script) < 2:
            continue
        f.write("""
        template<>
        constexpr bool get_binary_prop<__binary_prop::{0}>(char32_t c) {{
            return cp_script(c) == script::{0};
        }}
    """.format(script[1]))




def emit_binary_data(f, name, characters, pred):
    d = set([c.cp for c in filter(pred, characters)])
    emit_trie_or_table(f, name, d)

if __name__ == "__main__":

    scripts_names = get_scripts_names()
    block_names = get_blocks_names()
    all_characters, blocks = get_unicode_data()
    characters = list(filter(lambda c : c != None, all_characters))
    categories_name = get_cats_names()


    changed = {}
    new = all_characters
    for v in  SUPPORTED_VERSIONS:
        changed[v] = []
        print("Getting informations for Unicode {}".format(v))
        old, _ = get_unicode_data(v)
        for o in old:
            try:
                c = new[o.cp]
                if c.sc != o.sc or c.scx != o.scx or c.gc != o.gc or c.nv != o.nv:
                    changed[v].append(o)
                    continue
                for k in c.props.keys():
                    if c.props[k] != o.props[k]:
                        changed[v].append(o)
                        break
            except:
                pass
        new = old

    with open(sys.argv[1], "w") as f:
        f.write("""
#ifndef UNI_SINGLE_HEADER
#pragma once
#include "base.h"
#endif
namespace uni {
""")
        f.write("struct __string_with_idx { const char* name; uint32_t value; };")

        print("Age data")
        write_age_data(characters, f)

        print("Script data")
        write_script_data(characters, changed, scripts_names, f)
        print("Block data")
        write_blocks_data(block_names, blocks, f)
        print("Category data")
        write_categories_data(characters, changed, categories_name,f)


        print("Numeric Data")
        write_numeric_data(characters, f)

        #write_regex_support(f, characters, categories_name, scripts_names)

        print("Binary properties")
        write_binary_properties(characters, f)

        emit_binary_data(f, "__prop_assigned", characters, lambda c : True)


        f.write("}//namespace uni")




















