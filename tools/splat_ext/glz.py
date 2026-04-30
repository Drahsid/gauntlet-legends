import zlib
from pathlib import Path
from typing import Tuple

from splat.segtypes.segment import Segment
from splat.util import log, options


def Project_Path(path: Path) -> Path:
    if path.is_absolute():
        return path
    return options.opts.base_path / path


def Raw_Deflate_Decompress(compressed_data: bytes) -> Tuple[bytes, bytes]:
    decompressor = zlib.decompressobj(-15)
    decompressed_data = decompressor.decompress(compressed_data)
    if not decompressor.eof:
        log.error("compressed segment does not contain a complete raw DEFLATE stream")
    return decompressed_data, decompressor.unused_data


class N64SegGlz(Segment):
    @staticmethod
    def is_data() -> bool:
        return True

    def __init__(self, rom_start, rom_end, type, name, vram_start, args, yaml):
        super().__init__(rom_start, rom_end, type, name, vram_start, args=args, yaml=yaml)

        if isinstance(yaml, dict):
            decompressed_path = yaml.get("decompressed_path", f"files/{self.name}")
            compressed_path = yaml.get("compressed_path", f"files/compressed/{self.name}.bin")
        else:
            decompressed_path = f"files/{self.name}"
            compressed_path = f"files/compressed/{self.name}.bin"

        self.decompressed_path = Project_Path(Path(decompressed_path))
        self.compressed_path = Project_Path(Path(compressed_path))

    def out_path(self) -> Path:
        return options.opts.build_path / "compressed" / f"{self.name}.bin"

    def split(self, rom_bytes: bytes):
        if self.rom_end is None:
            log.error(
                f"segment {self.name} needs to know where it ends; add a position marker after it"
            )

        assert isinstance(self.rom_start, int)
        assert isinstance(self.rom_end, int)

        compressed_data = rom_bytes[self.rom_start : self.rom_end]
        decompressed_data, _ = Raw_Deflate_Decompress(compressed_data)

        self.compressed_path.parent.mkdir(parents=True, exist_ok=True)
        self.compressed_path.write_bytes(compressed_data)

        self.decompressed_path.parent.mkdir(parents=True, exist_ok=True)
        self.decompressed_path.write_bytes(decompressed_data)

        self.log(f"Wrote compressed reference {self.compressed_path}")
        self.log(f"Wrote decompressed data {self.decompressed_path}")
