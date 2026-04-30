#ifndef COMMON_H
#define COMMON_H

#include <ultra64.h>
#include "include_asm.h"

typedef struct {
    /* 0x00 */ char headerTag[32]; // "-==ROMDISK Header - HERE I AM==-"
    /* 0x20 */ u32 unk_0x20; // size ?
    /* 0x24 */ u32 unk_0x24; // offset ?
    /* 0x28 */ u32 unk_0x28; // entry count ?
    /* 0x2C */ u32 unk_0x2C;
} RomDiskHeader; /* sizeof = 0x30 */

typedef struct {
    /* 0x00 */ char name[16];
    /* 0x10 */ u32 romOffset; // for the "$ROMDISK" entry, this points to "-==ROMDISK Header - HERE I AM==-"
    /* 0x14 */ u32 compressedSize; // correlates to entry.rom - last_entry.rom
    /* 0x18 */ u32 decompressedVramStart;
    /* 0x1C */ u32 decompressedSize; // size ?
    /* 0x20 */ u32 bssStart; // seems to be the end of the decompressed file in vram
    /* 0x24 */ u32 bssSize;
    /* 0x28 */ u32 unk_0x28;
    /* 0x2C */ u32 unk_0x2C;
} RomFileEntry; /* sizeof = 0x30 */

typedef struct {
    /* 0x00 */ u32 entryCount;
    /* 0x04 */ u32 unk_0x04;
    /* 0x08 */ u32 unk_0x08;
    /* 0x0C */ u32 unk_0x0C;
    /* 0x10 */ u32 unk_0x10;
    /* 0x14 */ u32 unk_0x14;
    /* 0x18 */ u32 unk_0x18;
    /* 0x1C */ RomFileEntry entries[0];
} RomFileTableHeader; /* sizeof = 0x1C + (entryCount * sizeof(RomFileEntry)) */

extern void func_80002850(s32, u8*);
extern u32 inflate_decompress(u8* input, u8* output);

#endif
