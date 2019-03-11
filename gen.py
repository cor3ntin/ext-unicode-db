#!/usr/bin/python3

from lxml import etree
import sys
import os
import pystache
import re
import collections
from intbitset import intbitset

DIR = os.path.dirname(os.path.realpath(__file__))
UNICODE_DATA = os.path.join(DIR, "ucd", "ucd.nounihan.flat.xml")
PROPS_VALUE_FILE = os.path.join(DIR, "ucd", "PropertyValueAliases.txt")
BINARY_PROPS_FILE = os.path.join(DIR, "ucd", "binary_props.txt")

def cp_code(cp):
    try:
        return int(cp, 16)
    except:
        return 0
def to_hex(cp, n = 6):
    return "{0:#0{fill}X}".format(int(cp), fill=n).replace('X', 'x')

class ucd_cp(object):
    def __init__(self, char):
        self.cp  = cp_code(char.get('cp'))
        self.scx = [s.lower() for s in char.get("scx").split(" ")]
        self.sc  = char.get("sc").lower()
        self.scx = [self.sc] + self.scx

        self.gc  = char.get("gc").lower()
        self.age = char.get("age")
        self.block = char.get("blk").lower().replace("-", "_").replace(" ", "_")
        self.olower = char.get("OLower") == 'Y'
        self.oupper = char.get("OUpper") == 'Y'
        self.alpha = char.get("Alpha") == 'Y'
        self.wspace = char.get("WSpace") == 'Y'
        self.other_ignorable = char.get("ODI") == 'Y'
        self.variation_selector = char.get("VS") == 'Y'
        self.prepend_concat_mark = char.get("PCM") == 'Y'
        self.oidstart = char.get("OIDS") == 'Y'
        self.oidcontinue = char.get("OIDC") == 'Y'
        self.xidstart = char.get("XIDS") == 'Y'
        self.xidcontinue = char.get("XIDC") == 'Y'
        self.pattern_ws = char.get("Pat_WS") == 'Y'
        self.pattern_syntax = char.get("Pat_Syn") == 'Y'
        self.nv = None if char.get("nv") == 'NaN' else char.get("nv").split("/")
        self.omath = char.get("OMath") == 'Y'
        self.qmark = char.get("QMark") == 'Y'
        self.dash  = char.get("Dash") == 'Y'
        self.sentence_terminal = char.get("STerm") == 'Y'
        self.terminal_punctuation = char.get("Term") == 'Y'
        self.deprecated = char.get("Dep") == 'Y'
        self.diatric = char.get("Dia") == 'Y'
        self.extender = char.get("Ext") == 'Y'


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

def get_unicode_data():
    context = etree.iterparse(UNICODE_DATA, events=('end',))
    blocks = []
    characters = []
    for _, elem in context:
        if elem.tag =='{http://www.unicode.org/ns/2003/ucd/1.0}char':
            cp = ucd_cp(elem)
            if cp.cp != 0 or len(characters) == 0:
                characters.append(cp)
        elif elem.tag =='{http://www.unicode.org/ns/2003/ucd/1.0}block':
            blocks.append(ucd_block(elem))
    return (characters, blocks)


def write_script_data(characters, scripts_names, file):
    f.write("enum class script {")
    for i, script in enumerate(scripts_names):
        f.write(script[0] + ", //{}\n".format(i))
        if script[1] != script[0]:
            f.write(script[1] + " = " + script[0] +",")
    f.write("__max };\n")

    write_string_array(f, "__scripts_names", [(script[0], idx) for idx, script in enumerate(scripts_names)]
                                           + [(script[1], idx) for idx, script in enumerate(scripts_names)])

    script_blocks = []
    for cp in characters:
        for i, script in enumerate(cp.scx):
            script = script.lower()
            if len(script_blocks) <= i:
                script_blocks.append([])
            script_blocks[i].append((cp.cp, script))

    indexes = []
    begin = 0
    f.write("struct __script_data_t {char32_t first; script s;};")
    f.write("static constexpr const std::array __scripts_data = {")
    for bidx, block in enumerate(script_blocks):
        indexes.append(begin)
        prev = 'zzzz'
        f.write("\n//block {}\n".format(bidx))
        block = dict(block)
        for cp in range(0x10FFFF):
            script = block[cp] if cp in block else 'zzzz'
            if script != prev:
                f.write("__script_data_t{{ {}, script::{} }},".format(to_hex(cp, 6), script))
                begin = begin + 1
            prev = script

    f.write("__script_data_t{0x110000, script::zzzz }")
    f.write("};")
    f.write("static constexpr const std::array __scripts_data_indexes = {")
    for ii, i in enumerate(indexes):
        f.write("{} {}".format(i, "," if ii < len(indexes) -1 else "" ))
    f.write("};")

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

    # convert to bitmap chunks of 64 bits each
    chunks = []
    for i in range(0x110000 // CHUNK):
        chunk = 0
        for j in range(64):
            if rawdata[i * 64 + j]:
                chunk |= 1 << j
        chunks.append(chunk)


    r1 = chunks[0:0x800 // CHUNK]
    # 0x800..0x10000 trie
    (r2, r3) = compute_trie(chunks[0x800 // CHUNK : 0x10000 // CHUNK], 64 // CHUNK)
    # 0x10000..0x110000 trie
    (mid, r6) = compute_trie(chunks[0x10000 // CHUNK : 0x110000 // CHUNK], 64 // CHUNK)
    (r4, r5)  = compute_trie(mid, 64)

    size = len(r1) * 8 + len(r2) + len(r3) * 8 + len(r4) + len(r5) + len(r6) * 8
    return (size, (r1, r2, r3, r4, r5, r6))

def emit_bool_trie(f, name, trie_data):

    r1data = ','.join('0x%016x' % chunk for chunk in trie_data[0])
    r2data = ','.join(str(node) for node in trie_data[1])
    r3data = ','.join('0x%016x' % chunk for chunk in  trie_data[2])
    r4data = ','.join(str(node) for node in trie_data[3])
    r5data = ','.join(str(node) for node in trie_data[4])
    r6data = ','.join('0x%016x' % chunk for chunk in trie_data[5])
    f.write("static constexpr __bool_trie<{}, {}, {}, {}, {}> {} {{".format(len(trie_data[1]), len(trie_data[2]), len(trie_data[3]), len(trie_data[4]), len(trie_data[5]), name))
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
    elems = []
    minc = min(data)
    maxc = max(data)
    for i in range(0x110000):
        res = i >= minc and i <= maxc and i in data
        if prev == None or res != prev:
            elems.append((i, res))
            prev = res
    return len(elems) * 4, elems

def emit_bool_ranges(f, name, range_data):
    f.write("static constexpr __range_array<{}> {} = {{ {{".format(len(range_data), name))
    for idx, e in enumerate(range_data):
        f.write("__range_array_elem{{ {}, {} }}".format(to_hex(e[0], 6), 1 if e[1] else 0))
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

def write_categories_data(characters, categories_names, file):
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

    f.write("constexpr category __get_category(char32_t c) {")
    sorted_by_len.sort(reverse=True)
    for _, cat in sorted_by_len:
        f.write("if (__cat_{0}.lookup(c)) return category::{0};".format(cat))
    f.write("return category::cn;}")

    f.write("constexpr bool  __cp_is_letter(char32_t c) { return ")
    sorted_by_len.sort(reverse=True)
    for _, cat in sorted_by_len:
        if cat in ['lu', 'lt', 'll', 'lc', 'lm', 'lo']:
            f.write("__cat_{0}.lookup(c) || ".format(cat))
    f.write("true;}")

    print("total size : {}".format(size / 1024.0))

def write_age_data(characters, f):
    ages = list(set([cp.age for cp in characters]))
    ages.sort()
    f.write("enum class age : uint8_t {")
    f.write("unassigned,")
    for age in ages:
        f.write("v" + str(age).replace(".", '_') + ",")
    f.write("__max };\n")


    f.write("static constexpr std::array __age_strings = { \"unassigned\",")
    for idx, age in enumerate(ages):
        f.write('"{}"{}'.format(age, "," if idx < len(ages) - 1 else ""))
    f.write("};\n")

    f.write("\nstruct __age_data_t { char32_t first; age a;};\n")
    f.write("static constexpr std::array __age_data = {")
    known  = dict([(cp.cp, "v" + str(cp.age).replace(".", '_')) for cp in characters])
    prev  = ""
    size = 0
    for cp in range(0, 0x10FFFF):
        age = known[cp] if cp in known else 'unassigned'
        if prev != age:
            f.write("__age_data_t{{ {}, age::{} }},".format(to_hex(cp, 6), age))
            size = size + 1
            prev = age
    f.write("__age_data_t{0x110000, age::unassigned} };\n")
    print(size)

def write_numeric_data(characters, f):
    f.write("""
    struct __numeric_data_t {
        char32_t c: 24;
        uint8_t p : 8;
        int16_t n : 16;
        int16_t d : 16;
    };
""")
    f.write("static constexpr std::array __numeric_data = { ")
    for cp in characters:
        if cp.nv:
            n = int(cp.nv[0])
            p = 0
            d = cp.nv[1] if len(cp.nv) == 2 else '1'
            while n != 0 and float(n / 10).is_integer():
                p = p + 1
                n = int(n / 10)
            f.write("__numeric_data_t{{ {}, {}, {}, {} }},".format(to_hex(cp.cp, 6), p, n, d))



    f.write("__numeric_data_t{0x110000, 0, 0} };\n")

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
    d = intbitset([c.cp for c in filter(pred, characters)])
    emit_trie_or_table(f, name, d)

if __name__ == "__main__":
    scripts_names = get_scripts_names()
    block_names = get_blocks_names()
    characters, blocks = get_unicode_data()
    categories_name = get_cats_names()


    with open(sys.argv[1], "w") as f:
        f.write("#pragma once\n")
        f.write("#include \"utils.h\"\n")
        f.write("namespace uni {\n")
        f.write("struct __string_with_idx { const char* name; uint32_t value; };")

        print("Script data")
        write_script_data(characters, scripts_names, f)
        print("Block data")
        write_blocks_data(block_names, blocks, f)
        print("Category data")
        write_categories_data(characters, categories_name,f)
        print("Age data")
        write_age_data(characters, f)

        print("Numeric Data")
        write_numeric_data(characters, f)

        emit_binary_data(f, "__prop_other_lower_data", characters, lambda c : c.olower)
        emit_binary_data(f, "__prop_other_upper_data", characters, lambda c : c.oupper)
        emit_binary_data(f, "__prop_alpha_data", characters, lambda c : c.alpha)
        emit_binary_data(f, "__prop_other_ignorable_data", characters, lambda c : c.other_ignorable)
        emit_binary_data(f, "__prop_variation_selector_data", characters, lambda c : c.variation_selector)
        emit_binary_data(f, "__prop_prepend_concatenation_mark_data", characters, lambda c : c.prepend_concat_mark)
        emit_binary_data(f, "__prop_ws_data", characters, lambda c : c.wspace)
        emit_binary_data(f, "__prop_oidstart_data", characters, lambda c : c.oidstart)
        emit_binary_data(f, "__prop_oidcontinue_data", characters, lambda c : c.oidcontinue)
        emit_binary_data(f, "__prop_xidstart_data", characters, lambda c : c.xidstart)
        emit_binary_data(f, "__prop_xidcontinue_data", characters, lambda c : c.xidcontinue)
        emit_binary_data(f, "__prop_pattern_ws_data", characters, lambda c : c.pattern_ws)
        emit_binary_data(f, "__prop_pattern_syntax_data", characters, lambda c : c.pattern_syntax)
        emit_binary_data(f, "__prop_assigned", characters, lambda c : True)
        emit_binary_data(f, "__prop_omath_data", characters, lambda c : c.omath)
        emit_binary_data(f, "__prop_qmark_data", characters, lambda c : c.qmark)
        emit_binary_data(f, "__prop_dash_data", characters, lambda c : c.dash)
        emit_binary_data(f, "__prop_sentence_terminal_data", characters, lambda c : c.sentence_terminal)
        emit_binary_data(f, "__prop_terminal_punctuation_data", characters, lambda c : c.terminal_punctuation)
        emit_binary_data(f, "__prop_extender_data", characters, lambda c : c.extender)
        emit_binary_data(f, "__prop_deprecated_data", characters, lambda c : c.deprecated)
        emit_binary_data(f, "__prop_diatric_data", characters, lambda c : c.diatric)

        write_regex_support(f, characters, categories_name, scripts_names)


        f.write("}//namespace uni")




















