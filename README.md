# Gauntlet-Legends
A in-progress decompilation of Gauntlet-Legends (N64) decompression code. This project currently supports the US release.

## Building (Linux)
The build process has the following package requirements:
- git
- build-essential
- binutils-mips-linux-gnu
- python3

Under a Debian based distribution, you can install these with the following commands:
```
sudo apt update
sudo apt install git build-essential binutils-mips-linux-gnu python3
```

### Clone the repository
Clone `https://github.com/Drahsid/gauntlet-legends.git` in whatever directory you wish. Make sure to initialize the submodules!
```
git clone https://github.com/Drahsid/gauntlet-legends.git --recursive
cd gauntlet-legends
git submodule init
```

### Install Python3 requirements
Navigate to `tools/splat/` and run `pip3 install -r requirements.txt`

### Prepare the base rom
Copy over your US version of the gauntlet-legends rom into the root folder of this repository. Rename the rom to baserom.z64.

Next, run `make setup` to extract the assets of the rom.

### Build the rom
Just run `make` to build the rom.

## Contributing
Contributions are welcome. If you would like to reserve a function, open a PR with the function or file name(s).

