#!/usr/bin/python3

import glob
import sys
import os
import subprocess
import shutil
import hashlib
import colorama
from typing import List

BINARIES: List[str] = ['boot', 'Tower', 'Transitions', 'Boss', 'game']
BUILD_DIR: str = 'build'
TOOLS_DIR: str = 'tools'
BASENAME: str = 'gauntlet'
SPLAT: str = f'{TOOLS_DIR}/splat/split.py'
M2CTX: str = f'{TOOLS_DIR}/m2ctx.py'
COMPRESS: str = f'{TOOLS_DIR}/compress.py'
ASMPROCESSOR: str = f'{TOOLS_DIR}/asm-processor/asm_processor.py'
REMCOMS: str = f'{TOOLS_DIR}/clearcomments.py'
N64CRC: str = f'{TOOLS_DIR}/n64crc'
FIRSTDIFF: str = 'first_diff.py'
PYTHON: str = 'python3'

def verify_checksum(bin_file: str, sha1sum_file: str) -> bool:
    # Read the SHA1 checksum from the binary file
    with open(bin_file, 'rb') as f:
        sha1 = hashlib.sha1()
        while True:
            data = f.read(65536)
            if not data:
                break
            sha1.update(data)
        bin_checksum = sha1.hexdigest()

    # Read the SHA1 checksum from the file that contains the checksum
    with open(sha1sum_file, 'r') as f:
        file_checksum = f.readline().split()[0]

    # Compare the checksums and return the result
    if bin_checksum == file_checksum:
        return True
    else:
        return False

# I decided that I hate Ninja
def read_build_ninja_original() -> List[str]:
    with open('build.ninja_', 'r') as original_file:
        lines = original_file.readlines()
        for i, line in enumerate(lines):
            lines[i] = line.replace('__BASENAME__', BASENAME)
            if line.startswith('PYTHON = PYTHON_VALUE'):
                lines[i] = f'PYTHON = {PYTHON}\n'
        return lines

def generate_build_ninja(permuter: bool, binary_name: str, write_mode: str) -> None:
    c_files = glob.glob(f'src/{binary_name}/**/*.c', recursive=True)
    asm_files = [file for file in glob.glob(f'asm/{binary_name}/**/*.s', recursive=True) + glob.glob(f'src/{binary_name}/**/*.s', recursive=True) if 'nonmatching' not in file]
    bin_files = glob.glob(f'assets/{binary_name}/**/*.bin', recursive=True)

    output_lines = []
    if write_mode == 'w':
        output_lines = read_build_ninja_original()
    o_files = []

    output_lines.append(f'\n\n# Output for {binary_name}\n')

    for c_file in c_files:
        obj_file = os.path.join(f'{BUILD_DIR}/{binary_name}', c_file + '.o')
        if not permuter:
            output_lines.append(f'build {BUILD_DIR}/{binary_name}/{c_file}.d: compile_c_cpp {c_file}\n')
            output_lines.append(f'build {obj_file}_: compile_c {c_file} | {BUILD_DIR}/{binary_name}/{c_file}.d\n')
            output_lines.append(f'build {obj_file}: compile_c_strip {obj_file}_\n\n')
        else:
            output_lines.append(f'build {obj_file}: compile_c {c_file}\n')
        o_files.append(obj_file)
        os.makedirs(os.path.join('build', binary_name, os.path.dirname(c_file)), exist_ok=True)

    for asm_file in asm_files:
        obj_file = os.path.join(f'{BUILD_DIR}/{binary_name}', asm_file + '.o')
        output_lines.append(f'build {obj_file}: assemble_s {asm_file}\n')
        o_files.append(obj_file)
        os.makedirs(os.path.join('build', binary_name, os.path.dirname(asm_file)), exist_ok=True)

    for bin_file in bin_files:
        obj_file = os.path.join(f'{BUILD_DIR}/{binary_name}', bin_file + '.o')
        output_lines.append(f'build {obj_file}: compile_bin {bin_file}\n')
        o_files.append(obj_file)
        os.makedirs(os.path.join('build', binary_name, os.path.dirname(bin_file)), exist_ok=True)

    o_files_str = ' '.join(o_files)
    output_lines.append(f'\nbuild {BUILD_DIR}/{binary_name}.elf: link_{binary_name} {o_files_str}\n')

    if binary_name == 'boot':
        requirements = " | "
        for requirement in BINARIES:
            if requirement is not 'boot':
                requirements = f'{requirements} {BUILD_DIR}/{requirement}.bin'
        output_lines.append(f'build {BUILD_DIR}/{BASENAME}.z64: objcopy {BUILD_DIR}/{binary_name}.elf {requirements}\n')
    else:
        output_lines.append(f'build {BUILD_DIR}/{binary_name}.bin: objcopy {BUILD_DIR}/{binary_name}.elf\n')

    with open('build.ninja', write_mode) as build_file:
        build_file.writelines(output_lines)

def invoke_ninja() -> None:
    subprocess.run(['ninja'], check = True)

# Commands

def command_initial() -> None:
    subprocess.run(['make', '-C', TOOLS_DIR], check = True)

def command_clean() -> None:
    if os.path.exists('build'):
        shutil.rmtree('build')

def command_nuke() -> None:
    command_clean()
    if os.path.exists('asm'):
        shutil.rmtree('asm')
    if os.path.exists('assets'):
        shutil.rmtree('assets')
    autos = glob.glob('*auto.txt')
    for autofile in autos:
        try:
            os.remove(autofile)
        except:
            print(f'Failed to remove {autofile}')

def command_context() -> None:
    try:
        os.remove('ctx.c')
        os.remove('ctx_includes.c')
    except OSError:
        pass

    include_files = []
    for root, dirs, files in os.walk('include/'):
        for file in files:
            if file.endswith('.h'):
                include_files.append(os.path.join(root, file))
    for root, dirs, files in os.walk('src/'):
        for file in files:
            if file.endswith('.h'):
                include_files.append(os.path.join(root, file))

    ctxinc = 'ctx_includes.c'
    with open(ctxinc, 'w') as f:
        for include_file in include_files:
            f.write(f'#include "{include_file}"\n')
    subprocess.run([PYTHON, M2CTX, ctxinc], check=True)

def build(permuter: bool) -> None:
    write_mode = 'w'
    for binary_name in BINARIES:
        generate_build_ninja(permuter, binary_name, write_mode)
        write_mode = 'a'
    invoke_ninja()
    print('================')
    for binary_name in BINARIES:
        tname = f'{binary_name}.bin'
        if binary_name == 'boot':
            tname = f'{BASENAME}.z64'
            subprocess.run([N64CRC, f'{BUILD_DIR}/{tname}'])
        if verify_checksum(f'{BUILD_DIR}/{tname}', f'{binary_name}_checksum.sha1'):
            print(f'{binary_name}: OK!')
        else:
            print(f'{binary_name}: NOT OK!!')

if __name__ == '__main__':
    colorama.init()
    PYTHON = sys.executable

    build_used = False

    args = sys.argv[1:]
    for value in args:
        trunc = value.split('=')
        arg = trunc[0]
        if arg == 'initial':
            command_initial()
        if arg == 'nuke':
            command_nuke()
        if arg == 'clean':
            command_clean()
        if arg == 'setup':
            try:
                os.remove('files/syms/symbol_addrs.txt')
            except:
                pass
            for binary_name in BINARIES:
                subprocess.run(['cpp', '-CC', '-P', f'files/syms/symbol_addrs_original.{binary_name}.txt'], stdout = open(f'files/syms/symbol_addrs.{binary_name}.txt', 'w'))
                subprocess.run([PYTHON, REMCOMS, f'files/syms/symbol_addrs.{binary_name}.txt'])
            subprocess.run([PYTHON, SPLAT, f'{BASENAME}.yaml'], check = True)
            subprocess.run([PYTHON, COMPRESS, 'assets/boot/Tower.bin', 'files/Tower'], check = True)
            subprocess.run([PYTHON, COMPRESS, 'assets/boot/Transitions.bin', 'files/Transitions'], check = True)
            subprocess.run([PYTHON, COMPRESS, 'assets/boot/Boss.bin', 'files/Boss'], check = True)
            subprocess.run([PYTHON, COMPRESS, 'assets/boot/game.bin', 'files/game'], check = True)

            for binary_name in BINARIES:
                if binary_name == 'boot': continue
                if os.path.exists(f'{binary_name}.yaml'):
                    subprocess.run([PYTHON, SPLAT, f'{binary_name}.yaml'], check = True)
        if arg == 'context':
            command_context()
        if arg == 'build':
            build_used = True
            if len(trunc) > 1:
                if int(trunc[1]) == 1:
                    build(False)
            else:
                build(False)
        if arg == 'permuter':
            build_used = True
            build(True)
        if arg == 'diff':
            subprocess.run([PYTHON, FIRSTDIFF], check = True)

    if build_used == False:
        build(False)

