#!/usr/bin/env python3
from elftools.elf.elffile import ELFFile
from fnvhash import fnv1a_64
import os
import sys

if __name__ == "__main__":
    binFile = sys.argv[1]

    # read out the .bin file twice
    # once fully and once without the empty hash field at the end
    # firmware only hashes without hash field
    rawDataCropped = 0
    rawDataFull = 0
    fileSize = 0
    HASHFIELD_SIZE = 8
    with open(binFile, 'rb') as f:
        f.seek(0)
        fileSize = os.path.getsize(binFile)
        rawDataCropped = bytes(f.read(fileSize - HASHFIELD_SIZE))

        f.seek(0)
        rawDataFull = bytearray(f.read())

        f.close()

    # hash
    HASH_SEED = 0xcbf29ce484222325
    hash = fnv1a_64(rawDataCropped, HASH_SEED)
    print("  Calculated firmware hash: " + str(hex(hash)))

    # read out raw elf file and look for file offset of .firmwareCRC section for patching
    elfFile = binFile[:-3]
    elfFile += "elf"
    #rawElfFile = 0
    #firmwareCRCOffset = 0
    with open(elfFile, 'rb') as f:
        parsedElf = ELFFile(f)
        section = parsedElf.get_section_by_name(".firmwareHash")
        if section is None:
            raise Exception("Couldn't find firmwareHash section")
        firmwareCRCOffset = section.header.sh_offset
        f.seek(0)
        rawElfFile = bytearray(f.read())

    print("  Found firmwareHash at " + str(hex(firmwareCRCOffset)))

    # write the 8 byte hash in 1 byte pieces to the file buffers
    for i in range(8):
        mask = 0x00000000000000ff << (i*8)
        hashpart = (hash & mask) >> (i*8)

        rawDataFull[fileSize - (7-i) - 1] = hashpart
        # elf files are tricky to parse
        rawElfFile[firmwareCRCOffset + i] = hashpart

    with open(binFile, 'wb+') as f:
        f.seek(0)
        f.write(rawDataFull)
        f.close()
        print("  Patched .bin file successfully")

    with open(elfFile, 'wb+') as f:
        f.seek(0)
        f.write(rawElfFile)
        f.close()
        print("  Patched .elf file successfully")
