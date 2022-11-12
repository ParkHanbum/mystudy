import hashlib
import os
import string
import random
import subprocess
import time
import binascii
from elftools.elf.elffile import ELFFile


# file list
filelist = []


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
    for i in range(32):
        fn = fname.format(i)
        filelist.append(fn)
        f = open(fn, 'wb')
        appendRandomData(f)


def build():
    """
    build
    build executable file named executable using make
    """
    builder = "make CXXFLAGS=-DDEFINED_FILE_COUNT={:d}".format(len(filelist))
    print(builder)
    subprocess.call(builder, shell=True)
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
    for section in elf.iter_sections():
        if section.name == "myhashes":
            pos = section['sh_offset']

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
    for el in hash_pair:
        name_hash = el[0]
        data_hash = el[1]
        path = el[2]
        f.seek(pos, 0)
        f.write(bytes.fromhex(name_hash))
        pos += 32
        f.seek(pos, 0)
        f.write(bytes.fromhex(data_hash))
        pos += 32

        log.write(path)
        log.write('\t')
        log.write(name_hash)
        log.write('\t')
        log.write(data_hash)
        log.write('\n')

    f.close()


if __name__ == "__main__":
    createFiles()
    build()
    doHashes()

