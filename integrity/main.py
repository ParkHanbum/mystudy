import hashlib
import os
import string
import random
import subprocess
import time
import binascii
import sys
from elftools.elf.elffile import ELFFile


# file list
filelist = []

# bucketcount
bucketcount = 1

# FILE
file_amount = 32

# BUILD
build_type = "bench"


def appendRandomData(f: object) -> None:
    """
    appendRandomData
    add randomized data to file which given as argument
    """
    f.write(bytes(random.getrandbits(8) for _ in range(16)))


def createFiles():
    """
    createFiles
    create temporary files used for test
    """
    from pathlib import Path
    Path("temp").mkdir(parents=True, exist_ok=True)
    fname = os.path.join("temp", "temporary-{:d}")
    for i in range(file_amount):
        fn = fname.format(i)
        filelist.append(fn)
        f = open(fn, 'wb')
        appendRandomData(f)


def build():
    """
    build
    build executable file named executable using make
    """
    global bucketcount
    # calc bucket count
    filecount = len(filelist)
    opts = []
    # bucket count must be power of 2
    while bucketcount < filecount:
        bucketcount <<= 1

    build_script = "make"
    build_script = build_script + " " + build_type

    opts.append("-DDEFINED_FILE_COUNT={:d}".format(filecount))
    opts.append("-DBUCKET_COUNT={:d}".format(bucketcount))
    print(opts)
    build_script = build_script + " CXXFLAGS='{}'".format(' '.join(opts))
    print(build_script)
    subprocess.call(build_script, shell=True)
    time.sleep(1)


def doHashes():
    """
    doHashes
    write hashes to file integrity.
    write its index array also.
    """
    log = open("hashes.txt", "w+", newline="")
    f = open("integrity", "rb+")
    elf = ELFFile(f)
    pos = 0
    idx_pos = 0
    for section in elf.iter_sections():
        if section.name == "myhashes":
            pos = section['sh_offset']
        if section.name == "myhashes_index":
            idx_pos = section['sh_offset']

    hash_pair = []
    for path in filelist:
        pair = []
        # filename hash
        sha = hashlib.new('sha256')
        sha.update(path.encode('utf-8'))
        hex = sha.hexdigest()
        pair.append(hex)

        # filedata hash
        data = open(path, "rb+").read()
        sha = hashlib.new('sha256')
        sha.update(data)
        hex = sha.hexdigest()
        pair.append(hex)

        # keep the filename for logging
        pair.append(path)

        hash_pair.append(pair)

    # sort is required for use like hashmap
    hash_pair = sorted(hash_pair)
    for idx, el in enumerate(hash_pair):
        name_hash = el[0]
        data_hash = el[1]
        path = el[2]
        f.seek(pos, 0)
        f.write(bytes.fromhex(name_hash))
        pos += 32
        f.seek(pos, 0)
        f.write(bytes.fromhex(data_hash))
        pos += 32

        log.write(str(idx) + "]")
        log.write(path)
        log.write('\t')
        log.write(name_hash)
        log.write('\t')
        log.write(data_hash)
        log.write('\n')

    # hash index generate
    hashbit = len(bin(bucketcount - 1)[2:])
    hashbook = {}
    pos = 0
    for idx, el in enumerate(hash_pair):
        name_hash = el[0]
        data_hash = el[1]
        path = el[2]

        # calculate hash index
        v = int(name_hash, 16)
        # key index
        key_idx = v >> (256 - hashbit)
        if key_idx in hashbook:
            el = hashbook[key_idx]
            el["count"] = el["count"] + 1
        else:
            hashbook[key_idx] = {"hash_idx":idx, "count": 1}

        log.write(path)
        log.write('\t')
        log.write(str(key_idx))
        log.write('\t')
        log.write(name_hash)
        log.write('\n')

    for key_idx, item in hashbook.items():
        # since it has sorted it before, this work can sequentially
        pos = idx_pos + (8 * key_idx)
        f.seek(pos, 0)
        f.write((item["hash_idx"]).to_bytes(4, byteorder="little"))
        pos = pos + 4
        f.seek(pos, 0)
        f.write((item["count"]).to_bytes(4, byteorder="little"))

        log.write(str(key_idx))
        log.write('\t')
        log.write(str(item["hash_idx"]))
        log.write('\t')
        log.write(str(item["count"]))
        log.write('\t')
        log.write(hash_pair[item["hash_idx"]][0])
        log.write('\n')

    f.close()


if __name__ == "__main__":
    print("USAGE : python main.py [file amount] [bench|integrity]")
    file_amount = int(sys.argv[1], 10)
    if len(sys.argv) > 2:
        build_type = sys.argv[2]

    createFiles()
    build()
    doHashes()

