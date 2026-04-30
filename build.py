#!/usr/bin/env python3

import hashlib
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple

import splat.scripts.split as split
import yaml
from splat.util import conf


BINARIES: List[str] = ["boot", "Tower", "Transitions", "Boss", "game"]
COMPRESSED_BINARIES: List[str] = ["Tower", "Transitions", "Boss", "game"]
BUILD_DIR = Path("build")
LINKER_DIR = BUILD_DIR / "ld"
TOOLS_DIR = Path("tools")
BASENAME = "gauntlet"

M2CTX = TOOLS_DIR / "m2ctx.py"
COMPRESS = TOOLS_DIR / "compress.py"
GZIP124 = TOOLS_DIR / "gzip-1.2.4" / "gzip"
N64CRC = TOOLS_DIR / "n64crc"
SHRINK_SECTION = TOOLS_DIR / "shrink_section.py"
FIRSTDIFF = Path("first_diff.py")
BUILD_TEMPLATE = Path("build.ninja_")
BUILD_NINJA = Path("build.ninja")

SLINKY = os.environ.get("SLINKY", "slinky-cli")
PYTHON = sys.executable


def Ninja_Path(path: Path) -> str:
    return path.as_posix()


def Host_Platform() -> str:
    system_name = platform.system().lower()
    if system_name == "darwin":
        return "mac"
    if system_name.startswith("linux"):
        return "linux"
    if system_name.startswith("windows"):
        return "windows"
    return system_name


def Run(command: List[str]) -> None:
    subprocess.run(command, check=True)


def Verify_Checksum(bin_file: Path, sha1sum_file: Path) -> bool:
    sha1 = hashlib.sha1()
    with bin_file.open("rb") as file:
        while True:
            data = file.read(65536)
            if not data:
                break
            sha1.update(data)

    with sha1sum_file.open("r", encoding="utf-8") as file:
        expected_checksum = file.readline().split()[0]

    return sha1.hexdigest() == expected_checksum


def Read_Build_Template() -> List[str]:
    lines = BUILD_TEMPLATE.read_text(encoding="utf-8").splitlines(keepends=True)
    for index, line in enumerate(lines):
        line = line.replace("__BASENAME__", BASENAME)
        line = line.replace("__HOST_PLATFORM__", Host_Platform())
        if line.startswith("PYTHON = PYTHON_VALUE"):
            line = f"PYTHON = {Ninja_Path(Path(PYTHON))}\n"
        lines[index] = line
    return lines


def Object_Path(binary_name: str, source_path: Path) -> Path:
    return BUILD_DIR / binary_name / f"{Ninja_Path(source_path)}.o"


def Dependency_Path(binary_name: str, source_path: Path) -> Path:
    return BUILD_DIR / binary_name / f"{Ninja_Path(source_path)}.d"


def Ensure_Parent(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)


def Existing_Files(paths: Iterable[Path], pattern: str) -> List[Path]:
    output: List[Path] = []
    for path in paths:
        if path.exists():
            output.extend(path.rglob(pattern))
    return sorted(output, key=lambda value: Ninja_Path(value))


def Source_Files(binary_name: str) -> Tuple[List[Path], List[Path], List[Path]]:
    c_files = Existing_Files([Path("src") / binary_name], "*.c")
    asm_files = Existing_Files(
        [Path("asm") / binary_name, Path("src") / binary_name],
        "*.s",
    )
    asm_files = [
        path
        for path in asm_files
        if "nonmatching" not in {part.lower() for part in path.parts}
        and "nonmatchings" not in {part.lower() for part in path.parts}
        and "matching" not in {part.lower() for part in path.parts}
        and "matchings" not in {part.lower() for part in path.parts}
    ]
    bin_files = Existing_Files([Path("assets") / binary_name], "*.bin")
    if binary_name == "boot":
        compressed_assets = {
            Path("assets") / "boot" / f"{compressed_binary}.bin"
            for compressed_binary in COMPRESSED_BINARIES
        }
        bin_files = [path for path in bin_files if path not in compressed_assets]
    return c_files, asm_files, bin_files


def Linker_Script_Path(binary_name: str) -> Path:
    if binary_name == "boot":
        return LINKER_DIR / f"{BASENAME}.ld"
    return LINKER_DIR / f"{binary_name}.ld"


def Slinky_Config_Path(binary_name: str) -> Path:
    if binary_name == "boot":
        return LINKER_DIR / f"{BASENAME}.slinky.yaml"
    return LINKER_DIR / f"{binary_name}.slinky.yaml"


def Link_Rule(binary_name: str) -> str:
    if binary_name == "boot":
        return "link_boot"
    return f"link_{binary_name}"


def Compressed_Binary_Path(binary_name: str) -> Path:
    return BUILD_DIR / "boot" / "compressed" / f"{binary_name}.bin"


def Compressed_Binary_Object_Path(binary_name: str) -> Path:
    output_path = Compressed_Binary_Path(binary_name)
    return output_path.with_suffix(output_path.suffix + ".o")


def Compressed_Binary_Reference_Path(binary_name: str) -> Path:
    return Path("files") / "compressed" / f"{binary_name}.bin"


def Boot_Romfile_Binary_Paths() -> List[Tuple[str, Path]]:
    return [
        ("Tower", Compressed_Binary_Path("Tower")),
        ("Transitions", Compressed_Binary_Path("Transitions")),
        ("Boss", Compressed_Binary_Path("Boss")),
        ("LevelInit", Path("assets") / "boot" / "LevelInit.bin"),
        ("game", Compressed_Binary_Path("game")),
        ("ROMDISK", Path("assets") / "boot" / "ROMDISK.bin"),
    ]


def Boot_Romfile_Decompressed_Metadata() -> Dict[str, Tuple[int, int, int, int]]:
    output: Dict[str, Tuple[int, int, int, int]] = {}

    for binary_name in COMPRESSED_BINARIES:
        config_path = Path(f"{binary_name}.yaml")
        code_segments = [
            segment
            for segment in Splat_Segments(config_path)
            if getattr(segment, "vram_start", None) is not None
            and getattr(segment, "rom_start", None) == 0
            and getattr(segment, "rom_end", None) is not None
        ]
        if len(code_segments) != 1:
            raise SystemExit(f"Expected exactly one loadable code segment in {config_path}")

        segment = code_segments[0]
        vram_start = int(segment.vram_start)
        loaded_size = int(segment.rom_end)
        bss_size = int(getattr(segment, "bss_size", 0) or 0)
        output[binary_name] = (
            vram_start,
            loaded_size + bss_size,
            vram_start + loaded_size,
            bss_size,
        )

    output["LevelInit"] = (0xE0840000, 0, 0xE0840000, 0)
    output["ROMDISK"] = (0, 0, 0, 0)
    return output


def Binary_Symbol_Stem(input_path: Path) -> str:
    return "_binary_" + "".join(
        character if character.isalnum() or character == "_" else "_"
        for character in Ninja_Path(input_path)
    )


def Boot_Romfile_Linker_Symbols() -> List[str]:
    output = [
        "",
        "    /* Boot ROM file table symbols */",
    ]
    decompressed_metadata = Boot_Romfile_Decompressed_Metadata()

    for name, input_path in Boot_Romfile_Binary_Paths():
        symbol_stem = Binary_Symbol_Stem(input_path)
        vram_start, decompressed_size, bss_start, bss_size = decompressed_metadata[name]
        output.extend(
            [
                f"    _{name}_ROM_START = ABSOLUTE(romfile_ROM_START + ({symbol_stem}_start - romfile_DATA_START));",
                f"    _{name}_ROM_END = ABSOLUTE(romfile_ROM_START + ({symbol_stem}_end - romfile_DATA_START));",
                f"    _{name}_ROM_SIZE = ABSOLUTE({symbol_stem}_end - {symbol_stem}_start);",
                f"    _{name}_DECOMPRESSED_VRAM_START = ABSOLUTE(0x{vram_start:X});",
                f"    _{name}_DECOMPRESSED_SIZE = ABSOLUTE(0x{decompressed_size:X});",
                f"    _{name}_BSS_START = ABSOLUTE(0x{bss_start:X});",
                f"    _{name}_BSS_SIZE = ABSOLUTE(0x{bss_size:X});",
            ]
        )

    output.append("")
    return [f"{line}\n" for line in output]


def Inject_Boot_Romfile_Linker_Symbols(linker_script_path: Path) -> None:
    lines = linker_script_path.read_text(encoding="utf-8").splitlines(keepends=True)
    for index, line in enumerate(lines):
        if line.strip() == "/DISCARD/ :":
            lines[index:index] = Boot_Romfile_Linker_Symbols()
            linker_script_path.write_text("".join(lines), encoding="utf-8", newline="\n")
            return

    raise SystemExit(f"Could not find /DISCARD/ marker in {linker_script_path}")


def Linker_Object_Files(binary_name: str) -> List[Path]:
    slinky_config = yaml.safe_load(Slinky_Config_Path(binary_name).read_text(encoding="utf-8"))
    output: List[Path] = []
    seen = set()

    for segment in slinky_config["segments"]:
        for file_entry in segment["files"]:
            object_file = Path(file_entry["path"])
            if object_file in seen:
                continue

            seen.add(object_file)
            output.append(object_file)

    return output


def Expected_Section_Sizes(binary_name: str) -> Dict[Tuple[str, str], int]:
    config_path = Path(f"{BASENAME}.yaml") if binary_name == "boot" else Path(f"{binary_name}.yaml")
    output: Dict[Tuple[str, str], int] = {}

    for segment in Splat_Segments(config_path):
        for entry in segment.get_linker_entries():
            if entry.section_order_type != ".text":
                continue

            entry_segment = getattr(entry, "segment", None)
            rom_start = getattr(entry_segment, "rom_start", None)
            rom_end = getattr(entry_segment, "rom_end", None)
            if rom_start is None or rom_end is None:
                continue

            output[(Ninja_Path(Path(entry.object_path)), entry.section_order_type)] = rom_end - rom_start

    return output


def Generate_Build_Ninja(permuter: bool, binary_name: str, write_mode: str) -> None:
    c_files, asm_files, bin_files = Source_Files(binary_name)
    expected_section_sizes = Expected_Section_Sizes(binary_name)

    output_lines: List[str] = []
    if write_mode == "w":
        output_lines = Read_Build_Template()

    object_files: List[Path] = []
    output_lines.append(f"\n\n# Output for {binary_name}\n")

    for c_file in c_files:
        object_file = Object_Path(binary_name, c_file)
        dependency_file = Dependency_Path(binary_name, c_file)
        Ensure_Parent(object_file)
        Ensure_Parent(dependency_file)

        if not permuter:
            stripped_input = Path(f"{Ninja_Path(object_file)}_")
            stripped_output = object_file
            text_section_size = expected_section_sizes.get((Ninja_Path(object_file), ".text"))
            if text_section_size is not None:
                stripped_output = Path(f"{Ninja_Path(object_file)}.strip")

            output_lines.append(
                f"build {Ninja_Path(dependency_file)}: compile_c_cpp {Ninja_Path(c_file)}\n"
            )
            output_lines.append(f"    object = {Ninja_Path(object_file)}\n")
            output_lines.append(
                f"build {Ninja_Path(stripped_input)}: compile_c {Ninja_Path(c_file)} | {Ninja_Path(dependency_file)}\n"
            )
            output_lines.append(
                f"build {Ninja_Path(stripped_output)}: compile_c_strip {Ninja_Path(stripped_input)}\n"
            )
            if text_section_size is not None:
                output_lines.append(
                    f"build {Ninja_Path(object_file)}: shrink_section {Ninja_Path(stripped_output)} | {Ninja_Path(SHRINK_SECTION)}\n"
                )
                output_lines.append("    section = .text\n")
                output_lines.append(f"    size = 0x{text_section_size:X}\n")
            output_lines.append("\n")
        else:
            output_lines.append(
                f"build {Ninja_Path(object_file)}: compile_c {Ninja_Path(c_file)}\n"
            )

        object_files.append(object_file)

    for asm_file in asm_files:
        object_file = Object_Path(binary_name, asm_file)
        Ensure_Parent(object_file)
        output_lines.append(
            f"build {Ninja_Path(object_file)}: assemble_s {Ninja_Path(asm_file)}\n"
        )
        object_files.append(object_file)

    for bin_file in bin_files:
        object_file = Object_Path(binary_name, bin_file)
        Ensure_Parent(object_file)
        output_lines.append(
            f"build {Ninja_Path(object_file)}: compile_bin {Ninja_Path(bin_file)}\n"
        )
        object_files.append(object_file)

    if binary_name == "boot":
        for compressed_binary in COMPRESSED_BINARIES:
            compressed_path = Compressed_Binary_Path(compressed_binary)
            compressed_object_path = Compressed_Binary_Object_Path(compressed_binary)
            source_path = BUILD_DIR / f"{compressed_binary}.bin"
            reference_path = Compressed_Binary_Reference_Path(compressed_binary)
            if not reference_path.exists():
                raise SystemExit(
                    f"{reference_path} was not found. Run './build.py setup' to extract "
                    f"compressed references with the splat extension."
                )

            Ensure_Parent(compressed_path)
            Ensure_Parent(compressed_object_path)
            output_lines.append(
                f"build {Ninja_Path(compressed_path)}: recompress_bin {Ninja_Path(source_path)} | "
                f"{Ninja_Path(reference_path)} {Ninja_Path(COMPRESS)} {Ninja_Path(GZIP124)}\n"
            )
            output_lines.append(f"    check = {Ninja_Path(reference_path)}\n")
            output_lines.append(
                f"build {Ninja_Path(compressed_object_path)}: compile_bin {Ninja_Path(compressed_path)}\n"
            )
            object_files.append(compressed_object_path)

    object_files_string = " ".join(Ninja_Path(path) for path in Linker_Object_Files(binary_name))
    linker_script = Linker_Script_Path(binary_name)
    elf_path = BUILD_DIR / f"{binary_name}.elf"
    output_lines.append(
        f"\nbuild {Ninja_Path(elf_path)}: {Link_Rule(binary_name)} {object_files_string} | {Ninja_Path(linker_script)}\n"
    )

    if binary_name == "boot":
        requirements = " ".join(
            Ninja_Path(BUILD_DIR / f"{requirement}.bin")
            for requirement in BINARIES
            if requirement != "boot"
        )
        rom_path = BUILD_DIR / f"{BASENAME}.z64"
        output_lines.append(
            f"build {Ninja_Path(rom_path)}: objcopy {Ninja_Path(elf_path)} | {requirements}\n"
        )
    else:
        bin_path = BUILD_DIR / f"{binary_name}.bin"
        output_lines.append(
            f"build {Ninja_Path(bin_path)}: objcopy {Ninja_Path(elf_path)}\n"
        )

    with BUILD_NINJA.open(write_mode, encoding="utf-8", newline="\n") as build_file:
        build_file.writelines(output_lines)


def Invoke_Ninja() -> None:
    Run(["ninja"])


def Segment_Name(base_name: str, section_name: str, is_noload: bool, is_first: bool, count: int) -> str:
    if is_noload:
        return f"{base_name}_bss"
    if is_first:
        return base_name
    suffix = section_name.lstrip(".").replace(".", "_")
    if count <= 1:
        return f"{base_name}_{suffix}"
    return f"{base_name}_{suffix}_{count}"


def Section_Runs(segment) -> List[Tuple[Tuple[bool, str], List]]:
    section_runs: List[Tuple[Tuple[bool, str], List]] = []
    for entry in segment.get_linker_entries():
        key = (entry.noload, entry.section_order_type)
        if len(section_runs) > 0 and section_runs[-1][0] == key:
            section_runs[-1][1].append(entry)
        else:
            section_runs.append((key, [entry]))

    return section_runs


def Slinky_File(entry) -> Dict[str, object]:
    file_entry: Dict[str, object] = {
        "path": Ninja_Path(Path(entry.object_path)),
    }
    if entry.section_link_type != entry.section_order_type:
        file_entry["section_order"] = {
            entry.section_link_type: entry.section_order_type,
        }
    return file_entry


def Noload_Sections(section_name: str) -> List[str]:
    if section_name == ".bss":
        return [".bss", ".sbss", ".scommon", "COMMON"]

    return [section_name]


def Slinky_Segment(segment, segment_name: str, key: Tuple[bool, str], entries: List) -> Dict[str, object]:
    is_noload, section_name = key
    slinky_segment: Dict[str, object] = {
        "name": segment_name,
        "subalign": segment.subalign,
        "files": [Slinky_File(entry) for entry in entries],
    }

    if is_noload:
        slinky_segment["alloc_sections"] = None
        slinky_segment["noload_sections"] = Noload_Sections(section_name)
    else:
        slinky_segment["alloc_sections"] = [section_name]
        slinky_segment["noload_sections"] = None

    if segment.align is not None:
        slinky_segment["section_end_align"] = segment.align
        slinky_segment["segment_end_align"] = segment.align

    for entry in entries:
        entry_segment = getattr(entry, "segment", None)
        entry_align = getattr(entry_segment, "align", None)
        if entry_align is not None:
            slinky_segment["section_end_align"] = entry_align
            slinky_segment["segment_end_align"] = entry_align

    return slinky_segment


def Splat_Segments(config_path: Path):
    config = conf.load([config_path], ["ld"], False, False, False)
    return split.initialize_segments(config["segments"])


def Generate_Slinky_Config(binary_name: str) -> None:
    config_path = Path(f"{BASENAME}.yaml") if binary_name == "boot" else Path(f"{binary_name}.yaml")
    slinky_config_path = Slinky_Config_Path(binary_name)
    slinky_script_path = Linker_Script_Path(binary_name)
    slinky_config_path.parent.mkdir(parents=True, exist_ok=True)

    document: Dict[str, object] = {
        "settings": {
            "target_path": Ninja_Path(BUILD_DIR / f"{binary_name}.elf"),
            "d_path": Ninja_Path(slinky_script_path.with_suffix(".d")),
            "linker_symbols_style": "splat",
            "wildcard_sections": False,
            "sections_allowlist_extra": [],
            "sections_denylist": [],
            "discard_wildcard_section": True,
        },
        "segments": [],
    }

    final_segment_names: Dict[str, str] = {}
    for segment in Splat_Segments(config_path):
        entries = segment.get_linker_entries()
        if len(entries) == 0:
            continue

        section_runs = Section_Runs(segment)
        previous_slinky_segment: Optional[str] = None
        first_group = True
        base_name = segment.get_cname()
        section_counts: Dict[str, int] = {}

        for key, group_entries in section_runs:
            if key[0] and previous_slinky_segment is not None:
                merged_segment = document["segments"][-1]
                merged_segment["noload_sections"] = Noload_Sections(key[1])

                existing_paths = {
                    file_entry["path"]
                    for file_entry in merged_segment["files"]
                }
                for entry in group_entries:
                    file_entry = Slinky_File(entry)
                    if file_entry["path"] not in existing_paths:
                        merged_segment["files"].append(file_entry)
                        existing_paths.add(file_entry["path"])
                continue

            section_counts[key[1]] = section_counts.get(key[1], 0) + 1
            slinky_segment_name = Segment_Name(
                base_name,
                key[1],
                key[0],
                first_group,
                section_counts[key[1]],
            )
            slinky_segment = Slinky_Segment(segment, slinky_segment_name, key, group_entries)

            if first_group:
                follows_segment = getattr(segment, "given_follows_vram", None)
                if follows_segment is not None:
                    slinky_segment["follows_segment"] = final_segment_names[follows_segment]
                elif getattr(segment, "vram_start", None) is not None:
                    slinky_segment["fixed_vram"] = segment.vram_start
            elif previous_slinky_segment is not None:
                slinky_segment["follows_segment"] = previous_slinky_segment

            document["segments"].append(slinky_segment)
            previous_slinky_segment = slinky_segment_name
            first_group = False

        if previous_slinky_segment is not None:
            final_segment_names[base_name] = previous_slinky_segment
            final_segment_names[segment.name] = previous_slinky_segment

    slinky_config_path.write_text(
        yaml.safe_dump(document, sort_keys=False, width=120),
        encoding="utf-8",
        newline="\n",
    )


def Generate_Linker_Script(binary_name: str) -> None:
    slinky_config_path = Slinky_Config_Path(binary_name)
    linker_script_path = Linker_Script_Path(binary_name)
    Generate_Slinky_Config(binary_name)
    if shutil.which(SLINKY) is None and not Path(SLINKY).exists():
        raise SystemExit(
            "slinky-cli was not found. Run './build.py initial' or install it with "
            "'cargo install slinky-cli --version 0.5.0 --locked'."
        )
    Run([SLINKY, Ninja_Path(slinky_config_path), "-o", Ninja_Path(linker_script_path)])
    if binary_name == "boot":
        Inject_Boot_Romfile_Linker_Symbols(linker_script_path)


def Generate_Linker_Scripts() -> None:
    for binary_name in BINARIES:
        Generate_Linker_Script(binary_name)


def Command_Initial() -> None:
    Run(["make", "-C", Ninja_Path(TOOLS_DIR)])


def Command_Clean() -> None:
    for file_path in [Path(".splache"), BUILD_NINJA]:
        if file_path.exists():
            file_path.unlink()
    shutil.rmtree(BUILD_DIR, ignore_errors=True)


def Command_Nuke() -> None:
    Command_Clean()
    shutil.rmtree("asm", ignore_errors=True)
    shutil.rmtree("assets", ignore_errors=True)
    for autofile in Path(".").glob("*auto.txt"):
        try:
            autofile.unlink()
        except OSError:
            print(f"Failed to remove {autofile}")


def Command_Context() -> None:
    for file_path in [Path("ctx.c"), Path("ctx_includes.c")]:
        if file_path.exists():
            file_path.unlink()

    include_files: List[Path] = []
    for root_path in [Path("include"), Path("src")]:
        if root_path.exists():
            include_files.extend(sorted(root_path.rglob("*.h")))

    ctx_includes = Path("ctx_includes.c")
    with ctx_includes.open("w", encoding="utf-8", newline="\n") as file:
        for include_file in include_files:
            file.write(f'#include "{Ninja_Path(include_file)}"\n')
    Run([PYTHON, Ninja_Path(M2CTX), Ninja_Path(ctx_includes)])


def Command_Setup() -> None:
    split.main([Path(f"{BASENAME}.yaml")], modes=["all"], verbose=False, disassemble_all=True)

    for binary_name in BINARIES:
        if binary_name == "boot":
            continue
        config_path = Path(f"{binary_name}.yaml")
        if config_path.exists():
            split.main([config_path], modes=["all"], verbose=False, disassemble_all=True)

def Build(permuter: bool) -> None:
    Generate_Linker_Scripts()

    write_mode = "w"
    for binary_name in BINARIES:
        Generate_Build_Ninja(permuter, binary_name, write_mode)
        write_mode = "a"

    Invoke_Ninja()
    print("================")
    for binary_name in BINARIES:
        target_name = f"{binary_name}.bin"
        if binary_name == "boot":
            target_name = f"{BASENAME}.z64"
            Run([Ninja_Path(N64CRC), Ninja_Path(BUILD_DIR / target_name)])

        if Verify_Checksum(BUILD_DIR / target_name, Path(f"{binary_name}_checksum.sha1")):
            print(f"{binary_name}: OK!")
        else:
            print(f"{binary_name}: NOT OK!!")


def Command_Diff() -> None:
    if not FIRSTDIFF.exists():
        raise SystemExit(f"{FIRSTDIFF} was not found")
    Run([PYTHON, Ninja_Path(FIRSTDIFF)])


def Main() -> None:
    args = sys.argv[1:]
    if len(args) == 0:
        args = ["build"]

    for value in args:
        command = value.split("=", 1)[0]
        if command == "initial":
            Command_Initial()
        elif command == "nuke":
            Command_Nuke()
        elif command == "clean":
            Command_Clean()
        elif command == "setup":
            Command_Setup()
        elif command == "context":
            Command_Context()
        elif command == "build":
            Build(False)
        elif command == "permuter":
            Build(True)
        elif command == "diff":
            Command_Diff()
        else:
            raise SystemExit(f"Unknown command: {command}")


if __name__ == "__main__":
    Main()
