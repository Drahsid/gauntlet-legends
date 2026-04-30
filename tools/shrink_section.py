#!/usr/bin/env python3

import shutil
import struct
import sys
from pathlib import Path


SHT_RELA = 4
SHT_REL = 9


def Parse_Size(value: str) -> int:
    return int(value, 0)


def Read_C_String(data: bytes, offset: int) -> str:
    end = data.find(b"\0", offset)
    if end < 0:
        raise SystemExit("unterminated section name")
    return data[offset:end].decode("ascii")


def Main() -> None:
    if len(sys.argv) != 5:
        raise SystemExit("usage: shrink_section.py INPUT OUTPUT SECTION SIZE")

    input_path = Path(sys.argv[1])
    output_path = Path(sys.argv[2])
    section_name = sys.argv[3]
    target_size = Parse_Size(sys.argv[4])

    data = bytearray(input_path.read_bytes())
    if data[:4] != b"\x7FELF" or data[4] != 1:
        raise SystemExit(f"{input_path} is not an ELF32 object")

    endian = ">" if data[5] == 2 else "<"
    section_header_offset = struct.unpack_from(endian + "I", data, 0x20)[0]
    section_header_entry_size = struct.unpack_from(endian + "H", data, 0x2E)[0]
    section_header_count = struct.unpack_from(endian + "H", data, 0x30)[0]
    section_name_index = struct.unpack_from(endian + "H", data, 0x32)[0]

    def Section_Header(index: int) -> int:
        return section_header_offset + (index * section_header_entry_size)

    shstr_header = Section_Header(section_name_index)
    shstr_offset = struct.unpack_from(endian + "I", data, shstr_header + 0x10)[0]

    target_index = -1
    target_header = -1
    target_offset = 0
    old_size = 0
    for index in range(section_header_count):
        header = Section_Header(index)
        name_offset = struct.unpack_from(endian + "I", data, header)[0]
        name = Read_C_String(data, shstr_offset + name_offset)
        if name == section_name:
            target_index = index
            target_header = header
            target_offset = struct.unpack_from(endian + "I", data, header + 0x10)[0]
            old_size = struct.unpack_from(endian + "I", data, header + 0x14)[0]
            break

    if target_index < 0:
        raise SystemExit(f"{section_name} not found in {input_path}")
    if target_size > old_size:
        raise SystemExit(f"{section_name} target size 0x{target_size:X} exceeds current size 0x{old_size:X}")
    if target_size == old_size:
        if input_path != output_path:
            shutil.copyfile(input_path, output_path)
        return

    padding = data[target_offset + target_size:target_offset + old_size]
    if any(value != 0 for value in padding):
        raise SystemExit(f"{section_name} shrink would discard nonzero bytes")

    for index in range(section_header_count):
        header = Section_Header(index)
        section_type = struct.unpack_from(endian + "I", data, header + 0x04)[0]
        section_offset = struct.unpack_from(endian + "I", data, header + 0x10)[0]
        section_size = struct.unpack_from(endian + "I", data, header + 0x14)[0]
        section_entry_size = struct.unpack_from(endian + "I", data, header + 0x24)[0]
        section_info = struct.unpack_from(endian + "I", data, header + 0x1C)[0]
        if section_info != target_index or section_type not in (SHT_REL, SHT_RELA):
            continue

        if section_entry_size == 0:
            raise SystemExit("relocation section has zero entry size")
        for entry_offset in range(section_offset, section_offset + section_size, section_entry_size):
            relocation_offset = struct.unpack_from(endian + "I", data, entry_offset)[0]
            if target_size <= relocation_offset < old_size:
                raise SystemExit(f"{section_name} shrink would discard relocation at 0x{relocation_offset:X}")

    struct.pack_into(endian + "I", data, target_header + 0x14, target_size)
    output_path.write_bytes(data)


if __name__ == "__main__":
    Main()
