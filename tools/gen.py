#!/usr/bin/python3

import textwrap
import xml.etree.cElementTree as etree
import sys
import os
import re
import collections
import copy
from io import StringIO

DIR_WITH_UCD = os.path.realpath(sys.argv[2])
LAST_VERSION = "15.0"

def cp_code(cp):
    try:
        return int(cp, 16)
    except:
        return 0
def to_hex(cp, n = 6):
    return "{0:#0{fill}X}".format(int(cp), fill=n).replace('X', 'x')

def to_hex_unotation(cp, n = 5):
    return "U+{0:0{fill}X}".format(int(cp), fill=n)

class ucd_cp(object):
    def __init__(self, cp, char):
        try:
            self.age = float(char.get("age"))
        except:
            self.age = None
            pass
        self.cp  = cp
        self.gc  = char.get("gc")
        self.ea  = char.get("ea")
        self.name = char.get("na")
        self.reserved = False
        self.props = {}
        self.is_pictogram = False
        self.is_emoji = False
        if self.gc in ['Co', 'Cn', 'Cs']:
            self.reserved = True
            return
        self.is_emoji = char.get("Emoji") == "Y"
        self.is_pictogram = char.get("ExtPict") == "Y"
        for p in ["XIDC", "XIDS"]:
            if  char.get(p) == 'Y':
                self.props[p.lower()] =  True


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
    return (characters, blocks)



def write_binary_properties(file, property_name, codepoints):
    file.write("""


    // Property {}
    """.format(property_name))

    start   = None
    current = None
    for idx in range(0, len(codepoints)):
        if codepoints[idx]:
            if start == None:
                start = idx
                current = idx
                continue
            if idx == current+1:
                current = idx
                continue
        if start:
            f.write("{{ {}, {} }},".format(to_hex(start), to_hex(current)))
            start = current = None

def write_text_ranges(file, property_name, codepoints, names):
    file.write("""
// {} (count: {})\n
""".format(property_name, sum(codepoints)))

    start   = None
    current = None
    for idx in range(0, len(codepoints)):
        if codepoints[idx]:
            if start == None:
                start = idx
                current = idx
                continue
            if idx == current+1:
                current = idx
                continue
        if start:
            if start == current:
                f.write("{} {}\n".format(to_hex_unotation(start), names[start]))
            else:
                f.write("{} {} ..{} {} \n".
                    format(to_hex_unotation(start), names[start],
                    to_hex_unotation(current), names[current]))
            start = current = None

def is_printable_cat(cat):
    return cat == 'Zs' or cat[0] in ['L', 'M', 'N', 'P', 'S']

def is_printable(cp):
    return is_printable_cat(cp.gc)

def write_xid_start(file, characters):
    arr = [False] * 0x10FFFF
    for c in [c for c in characters if "xids" in c.props]:
        arr[c.cp] = True
    write_binary_properties(file, "xid_start", arr)

def write_xid_continue(file, characters):
    arr = [False] * 0x10FFFF
    for c in [c for c in characters if "xidc" in c.props and not "xids" in c.props]:
        arr[c.cp] = True
    write_binary_properties(file, "xid_continue", arr)

def write_is_printable(file, characters):
    arr = [False] * 0x10FFFF
    for c in [c for c in characters if is_printable(c)]:
        arr[c.cp] = True
    write_binary_properties(file, "printable", arr)

def write_is_formatable(file, characters):
    arr = [False] * 0x10FFFF
    for c in [c for c in characters if c.gc == 'Cf']:
        arr[c.cp] = True
    write_binary_properties(file, "formatable", arr)

def write_is_non_spacing(file, characters):
    arr = [False] * 0x10FFFF
    for c in [c for c in characters if c.gc in ['Me', 'Mn']]:
        arr[c.cp] = True
    write_binary_properties(file, "enclosing + non spacing", arr)

def write_is_wide(file, characters):
    arr = [False] * 0x10FFFF
    for c in [c for c in characters if c.ea in ['F', 'W']]:
        arr[c.cp] = True
    write_binary_properties(file, "wide", arr)

def write_list_of_codepoints(file, name, characters, names):
    arr = [False] * 0x10FFFF
    for c in characters:
        arr[c] = True
    write_binary_properties(file, name, arr)

standard_wide_ranges = [
        (0x1100, 0x115F),
        (0x2329, 0x232A),
        (0x2E80, 0x303E),
        (0x3040, 0xA4CF),
        (0xAC00, 0xD7A3),
        (0xF900, 0xFAFF),
        (0xFE10, 0xFE19),
        (0xFE30, 0xFE6F),
        (0xFF00, 0xFF60),
        (0xFFE0, 0xFFE6),
        (0x1F300, 0x1F64F),
        (0x1F900, 0x1F9FF),
        (0x20000, 0x2FFFD),
        (0x30000, 0x3FFFD)
]

def is_standard(cp):
    for start, end in standard_wide_ranges:
        if cp >= start and cp <= end:
            return True
        if cp < start:
            break
    return False

def is_emoji_or_symbols(c):
    ranges = [
        (0x04DC0, 0x04DFF),
        (0x1F300, 0x1F5FF),
        (0x1F900, 0x1F9FF),
    ]
    if c.reserved:
        return False
    for r in ranges:
        if c.cp >= r[0] and c.cp <= r[1]:
            return True
    return False

def is_wide(c):
    return (c.ea in ['F', 'W'] or is_emoji_or_symbols(c))

if __name__ == "__main__":

    all_characters, blocks = get_unicode_data()
    characters = list(filter(lambda c : c != None and c.age and c.age < 13, all_characters))
    names = {}
    for c in characters:
        names[c.cp] = c.name
    ea = [c for c in characters if is_wide(c)]
    missing_from_standard = [c for c in ea if not is_standard(c.cp)]
    reserved = [c for c in characters if c.reserved and is_standard(c.cp) and not is_wide(c)]
    only_in_standard = [c for c in characters if is_standard(c.cp) and not c.reserved and not is_wide(c)]

    with open(sys.argv[1], "w") as f:
        pass
        #write_list_of_codepoints(f, "Missing from standard", missing_from_standard, names)
        #write_text_ranges(f, "Reserved", reserved_but_specified, names)
        #write_text_ranges(f, "Only in standard", only_in_standard, names)
        #write_list_of_codepoints(f, "EA", ea, names)

    print("U+XXXXX: |_|\n")
    print("## Reserved:")
    print("\n".join(["{}: |_|{}|_|".format(to_hex_unotation(c.cp), chr(c.cp)) for c in reserved]))
    print("## Assigned, wide in the standard only:")
    print("\n".join(["{}: |_|{}|_|".format(to_hex_unotation(c.cp), chr(c.cp)) for c in only_in_standard]))
    print("## Assigned, wide in Unicode only:")
    print("\n".join(["{}: |_|{}|_|".format(to_hex_unotation(c.cp), chr(c.cp)) for c in missing_from_standard]))

