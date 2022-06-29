def compute_trie(rawdata, chunksize):
    root = []
    childmap = {}
    child_data = []
    for i in range(len(rawdata) // chunksize):
        data = rawdata[i * chunksize : (i + 1) * chunksize]
        child = '|'.join(map(str, data))
        if child not in childmap:
            childmap[child] = len(childmap)
            child_data.extend(data)
        root.append(childmap[child])
    return (root, child_data)


class Point:
    level1data   = None
    level2index  = None
    level2data   = None
    level2first  = None
    level2size   = None
    level2index  = None
    level2data   = None
    level2first  = None
    level2size   = None


    r2data = None

def construct_trie_data(data, bits_per_cp):
    CHUNK = 64
    rawdata = [0] * 0x110000
    if(isinstance(data, dict)):
        for cp, value in data.items():
            rawdata[cp] = value
    else:
        for cp in data:
            rawdata[cp] = 1

    def trim(arr):
        import numpy
        a = numpy.trim_zeros(arr, 'f')
        f = len(arr) - len(a)
        c = numpy.trim_zeros(a, 'b')
        b = len(a) - len(c)
        return (c, f, b)


    # convert to bitmap chunks of 128 bits each
    chunks = []
    for i in range(0x110000 // CHUNK):
        chunk = 0
        for j in range(CHUNK):
            if rawdata[i * CHUNK + j]:
                chunk |= rawdata[i * CHUNK + j] << j + (bits_per_cp -1)
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

def emit_trie(f, name, trie_data, bits_per_cp):

    format_bytes = lambda bytes :\
        ','.join("0x%0{}x".format(16 * bits_per_cp) % chunk for chunk in bytes)

    r1data = format_bytes(trie_data[0])
    r2data = ','.join(str(node) for node in trie_data[1][0])
    r3data = format_bytes(trie_data[2])
    r4data = ','.join(str(node) for node in trie_data[3][0])
    r5data = ','.join(str(node) for node in trie_data[4][0])
    r6data = format_bytes(trie_data[5])
    f.write("""
static constexpr
basic_trie<{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}> {} {{"""
.format(
        bits_per_cp,
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
    f.write("{{ {} }}, {{ {} }}, {{ {} }}, {{ {} }}, {{ {} }}, {{ {} }}"
        .format(r1data, r2data, r3data, r4data, r5data, r6data))
    f.write("};")
