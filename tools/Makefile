UNAME_S := $(shell uname -s)
ifeq ($(OS),Windows_NT)
	DETECTED_OS := windows
	HOST_TRIPLE := x86_64-pc-windows-msvc
else ifeq ($(UNAME_S),Linux)
	DETECTED_OS := linux
	HOST_TRIPLE := x86_64-unknown-linux-musl
else ifeq ($(UNAME_S),Darwin)
	DETECTED_OS := mac
	MAKE := gmake
	CPPFLAGS += -xc++
	HOST_TRIPLE := x86_64-apple-darwin
endif

ifeq ($(DETECTED_OS),windows)
$(error This repo does not support compiling on native Windows yet, please use WSL instead.)
endif

KMC_DIR         := gcc_kmc/$(DETECTED_OS)/2.7.2
KMC_GCC         := $(KMC_DIR)/gcc
KMC_AS          := $(KMC_DIR)/as
SLINKY_VERSION  := 0.5.0
GZIP_VERSION    := 1.2.4
GZIP_DIR        := gzip-$(GZIP_VERSION)
GZIP            := $(GZIP_DIR)/gzip

# Tools
CC := gcc
CFLAGS := -I. -Wall -Wextra -Wno-unused-parameter -pedantic -Ofast -s

# Not sure if n64crc has a repo somewhere that I should pull as a submodule
PROGRAMS := n64crc $(KMC_GCC) $(KMC_AS) $(GZIP) slinky

default: all

.PHONY: default all slinky

all: $(PROGRAMS)

n64crc:
	$(CC) $(CFLAGS) -o $@ $@.c

$(KMC_GCC): | $(KMC_DIR)
	wget https://github.com/decompals/mips-gcc-2.7.2/releases/latest/download/gcc-2.7.2-$(DETECTED_OS).tar.gz
	tar xf gcc-2.7.2-$(DETECTED_OS).tar.gz -C $(KMC_DIR)
	$(RM) gcc-2.7.2-$(DETECTED_OS).tar.gz

$(KMC_AS): | $(KMC_DIR)
	wget https://github.com/decompals/mips-binutils-2.6/releases/latest/download/binutils-2.6-$(DETECTED_OS).tar.gz
	tar xf binutils-2.6-$(DETECTED_OS).tar.gz -C $(KMC_DIR)
	$(RM) binutils-2.6-$(DETECTED_OS).tar.gz

$(GZIP):
	wget https://ftp.gnu.org/pub/gnu/gzip/$(GZIP_DIR).tar.gz
	tar xf $(GZIP_DIR).tar.gz
	cd $(GZIP_DIR) && ./configure
	$(MAKE) -C $(GZIP_DIR) CFLAGS='-O -Wno-error=incompatible-pointer-types'
	$(RM) $(GZIP_DIR).tar.gz

slinky:
	@if ! command -v slinky-cli >/dev/null 2>&1; then \
		if ! command -v cargo >/dev/null 2>&1; then \
			echo "cargo is required to install slinky-cli $(SLINKY_VERSION)"; \
			exit 1; \
		fi; \
		cargo install slinky-cli --version $(SLINKY_VERSION) --locked; \
	fi

$(KMC_DIR):
	mkdir -p $@
