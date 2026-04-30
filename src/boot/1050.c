#include "common.h"
#include <PR/os_tlb.h>

typedef struct {
    /* 0x000 */ u32 entryCount;
    /* 0x004 */ u32 unk_0x004;
    /* 0x008 */ u32 unk_0x008;
    /* 0x00C */ u32 unk_0x00C;
    /* 0x010 */ u32 unk_0x010;
    /* 0x014 */ u32 unk_0x014;
    /* 0x018 */ u32 unk_0x018;
    /* 0x01C */ RomFileEntry entries[6];
    /* 0x13C */ u8 unk_0x13C[0x90];
} BootRomFileTable; /* sizeof = 0x1CC */

extern u8 D_E01301C0[];

static u8 sBootMetadataCopyStart[0x14] = { 0 };

static BootRomFileTable sBootRomFileTable = {
    6,
    0,
    0,
    0,
    0,
    0,
    0,
    {
        { "Tower", 0x2980, 0x4600, 0xE0860000, 0xB120, 0xE086AC50, 0x4D0, 0, 0 },
        { "Transitions", 0x6F80, 0x8860, 0xE0800000, 0x251D0, 0xE081CF50, 0x8280, 0, 0 },
        { "Boss", 0xF7E0, 0x60E0, 0xE0880000, 0xFC70, 0xE088E4F0, 0x1780, 0, 0 },
        { "LevelInit", 0x158C0, 0x10, 0xE0840000, 0, 0xE0840000, 0, 0, 0 },
        { "game", 0x158D0, 0x4DE10, 0xE0000400, 0x142B90, 0xE00ACEF0, 0x960A0, 0, 0 },
        { "$ROMDISK", 0x636E0, 0x30, 0, 0, 0, 0, 0, 0 },
    },
    { 0 },
};

s32 strcmp(u8* left, u8* right) {
    s32 difference;
    u8 left_char;
    u8 right_char;

    while (1) {
        left_char = *left;
        right_char = *right;
        difference = left_char - right_char;

        if (!((left_char != 0) && (((right_char == 0) | (difference != 0)) == 0))) {
            return difference;
        }

        left++;
        right++;
    }
    return difference;
}

void MapTLB(void) {
    s32 index;

    for (index = 0; index < 32; index++) {
        osMapTLB(index, OS_PM_4K, (void*)0x80000000, -1, -1, -1);
    }

    osMapTLB(30, OS_PM_4M, (void*)0xE0000000, 0, 0x400000, -1);
}

#define GAME_COMPRESSED_DRAM_OFFSET (0x300000)
#define PI_CART_DOM1_ADDR_BASE (0x10000000)
#define MAX_PI_TRANSFER_LENGTH (0xFFFF8)
#define BOOT_METADATA_COPY_SIZE (0x1E0)

void func_8020010C(s32 boot_arg) {
    s32 dma_write_length;
    s32 metadata_copy_count;
    RomFileEntry* game_file;
    u8* write_ptr;
    s32 loop_counter;
    u8* metadata_source;

    // Locate the compressed 'game' binary in the boot ROM file table
    for (loop_counter = 1; (u32)loop_counter <= sBootRomFileTable.entryCount; loop_counter++) {
        game_file = &sBootRomFileTable.entries[loop_counter - 1];
        // search for "game" entry
        if (strcmp(game_file->name, "game") == 0) {
            break;
        }
    }

    while (IO_READ(PI_STATUS_REG) & (PI_STATUS_DMA_BUSY | PI_STATUS_IO_BUSY)) {}
    IO_WRITE(PI_STATUS_REG, PI_STATUS_IO_BUSY);
    IO_WRITE(PI_DRAM_ADDR_REG, GAME_COMPRESSED_DRAM_OFFSET);
    IO_WRITE(PI_CART_ADDR_REG, game_file->romOffset + PI_CART_DOM1_ADDR_BASE);

    // PI write length is 8-byte aligned and capped to MAX_PI_TRANSFER_LENGTH
    dma_write_length = (game_file->compressedSize + 7) & ~7;
    if (dma_write_length > MAX_PI_TRANSFER_LENGTH) {
        dma_write_length = MAX_PI_TRANSFER_LENGTH;
    }

    IO_WRITE(PI_WR_LEN_REG, dma_write_length);
    while (IO_READ(PI_STATUS_REG) & (PI_STATUS_DMA_BUSY | PI_STATUS_IO_BUSY)) {}
    IO_WRITE(PI_STATUS_REG, PI_STATUS_IO_BUSY);

    MapTLB();
    inflate_decompress((u8*)PHYS_TO_K0(GAME_COMPRESSED_DRAM_OFFSET), (u8*)game_file->decompressedVramStart);

    // Clear the game BSS
    write_ptr = (u8*)game_file->bssStart;
    loop_counter = game_file->bssSize;
    for (loop_counter--; loop_counter != -1; loop_counter--, write_ptr++) {
        *write_ptr = 0;
    }

    // Copy the boot ROM file table to the freshly inflated game image
    metadata_source = sBootMetadataCopyStart;
    write_ptr = D_E01301C0;
    metadata_copy_count = BOOT_METADATA_COPY_SIZE;
    for (metadata_copy_count--; metadata_copy_count != -1; metadata_copy_count--) {
        *write_ptr++ = *metadata_source++;
    }

    func_80002850(boot_arg, metadata_source);
}

