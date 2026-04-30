#!/usr/bin/env python3

import argparse
import subprocess
import sys
import zlib
from pathlib import Path
from typing import Optional, Tuple


def Raw_Deflate_Decompress(compressed_data: bytes) -> Tuple[bytes, bytes]:
    decompressor = zlib.decompressobj(-15)
    decompressed_data = decompressor.decompress(compressed_data)
    if not decompressor.eof:
        raise ValueError("input does not contain a complete raw DEFLATE stream")
    return decompressed_data, decompressor.unused_data


def Decompress(input_file: Path, output_file: Path, compressed_copy: Optional[Path]) -> None:
    compressed_data = input_file.read_bytes()
    decompressed_data, _ = Raw_Deflate_Decompress(compressed_data)

    output_file.parent.mkdir(parents=True, exist_ok=True)
    output_file.write_bytes(decompressed_data)

    if compressed_copy is not None:
        compressed_copy.parent.mkdir(parents=True, exist_ok=True)
        compressed_copy.write_bytes(compressed_data)


def Gzip124_Compress(input_file: Path, gzip_path: Path) -> bytes:
    gzip_data = subprocess.check_output(
        [
            str(gzip_path),
            "-c",
            "--no-name",
            "-6",
            str(input_file),
        ]
    )
    if len(gzip_data) < 18:
        raise ValueError("gzip output is too small to contain a deflate stream")

    # The ROM stores only the raw DEFLATE stream, not the gzip wrapper.
    return gzip_data[10:-8]


def Compress(input_file: Path, output_file: Path, gzip_path: Path, check_file: Optional[Path]) -> None:
    compressed_data = Gzip124_Compress(input_file, gzip_path)

    if check_file is not None:
        expected_data = check_file.read_bytes()
        _, padding = Raw_Deflate_Decompress(expected_data)
        compressed_data += padding
        if compressed_data != expected_data:
            raise SystemExit(f"{output_file} does not byte-match {check_file}")

    output_file.parent.mkdir(parents=True, exist_ok=True)
    output_file.write_bytes(compressed_data)


def Main() -> int:
    parser = argparse.ArgumentParser(description="Gauntlet Legends raw DEFLATE compression helper.")
    parser.add_argument("input_file", type=Path)
    parser.add_argument("output_file", type=Path)
    parser.add_argument("--compress", action="store_true", help="compress input using gzip 1.2.4 raw DEFLATE")
    parser.add_argument("--gzip", type=Path, default=Path("tools/gzip-1.2.4/gzip"), help="path to gzip 1.2.4")
    parser.add_argument("--check", type=Path, help="expected compressed segment to compare against")
    parser.add_argument("--copy-compressed", type=Path, help="write a copy of the compressed input while decompressing")
    args = parser.parse_args()

    if args.compress:
        Compress(args.input_file, args.output_file, args.gzip, args.check)
    else:
        Decompress(args.input_file, args.output_file, args.copy_compressed)

    return 0


if __name__ == "__main__":
    sys.exit(Main())
