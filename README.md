# Gauntlet Legends
A in-progress decompilation of Gauntlet Legends

## Building (Linux)

### Install build dependencies
The build process has the following package requirements:
- git
- build-essential
- binutils-mips-linux-gnu
- python3
- python3-pip
- ninja-build
- cargo
- wget

Under a Debian based distribution, you can install these with the following commands:
```
sudo apt update
sudo apt install git build-essential binutils-mips-linux-gnu python3 python3-pip ninja-build cargo wget
```

### Clone the repository
Clone `https://github.com/Drahsid/gauntlet-legends.git` in whatever directory you wish. Make sure to initialize the submodules!
```
git clone https://github.com/Drahsid/gauntlet-legends.git --recursive
cd gauntlet-legends
git submodule update --init --recursive
```

### Install Python3 requirements
Run `python3 -m pip install -r requirements.txt`

### Prepare the tools
Run `./build.py initial`. This builds `n64crc`, downloads the KMC GCC package used by the build, and installs the `slinky-cli` executable if it is not already on your `PATH`.

### Prepare the base rom
Copy over your copy of Gauntlet Legends US into the root folder of this repository. Rename the rom to baserom.z64.

Next, run `./build.py setup` to extract the assets of the rom.

## Build the rom
Run `./build.py` to build the rom. If the build succeeds, a folder will be produced with the name `build`, inside this will be the output rom `gauntlet.z64`.

Addtionally, if the rom matches, you should see this output:
```
BootChip: CIC-NUS-6102
CRC 1: 0x729B5E32  Calculated: 0x729B5E32 (Good)
CRC 2: 0xB728D980  Calculated: 0xB728D980 (Good)
boot: OK!
Tower: OK!
Transitions: OK!
Boss: OK!
game: OK!
```

# TODO
- Recompress files
- Fix data/rodata alignment being 4 instead of 16

