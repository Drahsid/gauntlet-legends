import argparse
import struct
import zlib
import subprocess

def decompress_lz(input_file: str, output_file: str) -> None:
    with open(input_file, 'rb') as f:
        compressed_data = f.read()

    decompressed_data = zlib.decompress(compressed_data, -15)

    # Convert the decompressed data to a bytearray so we can byteswap it
    decompressed_data = bytearray(decompressed_data)

    # Byteswap the data if the target file is in Big Endian
    if struct.pack('@i', 1) == b'\x00\x00\x00\x01':
        # Target file is Big Endian, byteswap the data
        decompressed_data = bytearray(struct.unpack('>{}H'.format(len(decompressed_data) // 2), decompressed_data))

    with open(output_file, 'wb') as f:
        f.write(decompressed_data) 

def compress_lz(input_file: str, output_file: str) -> None:
    with open(input_file, 'rb') as f:
        decompressed_data = f.read()
    # Compress the decompressed data using zlib
    p = subprocess.Popen(["gzip-1.2.4/gzip", "-c", "--no-name", "-9"], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    compressed_data, _ = p.communicate(decompressed_data)

    # Byteswap the compressed data if the target file is in Big Endian
    if struct.pack('@i', 1) == b'\x00\x00\x00\x01':
        # Target file is Big Endian, byteswap the data
        compressed_data = bytearray(struct.pack('>{}H'.format(len(compressed_data) // 2), *struct.unpack('{}H'.format(len(compressed_data) // 2), compressed_data)))

    with open(output_file, 'wb') as f:
        f.write(compressed_data)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Decompress and optionally recompress a LZ file.')
    parser.add_argument('input_file', help='path to the input file')
    parser.add_argument('output_file', help='path to the output file')
    parser.add_argument('--compress', action='store_true', help='set to recompress the decompressed data')
    args = parser.parse_args()

    if args.compress:
        compress_lz(args.input_file, args.output_file)
    else:
        decompress_lz(args.input_file, args.output_file)
