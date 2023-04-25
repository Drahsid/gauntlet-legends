#include "common.h"
#include <PR/os_tlb.h>

extern unk_struct_D_802002C4 D_802002C4;
extern char D_80201970[]; // "game"

extern u8 D_802002B0[];
extern u8 D_E01301C0[];

s32 strcmp(u8* arg0, u8* arg1) {
    s32 temp_a2;
    u8 temp_v0;
    u8 temp_v1;

    while (1) {
        temp_v0 = *arg0;
        temp_v1 = *arg1;
        temp_a2 = temp_v0 - temp_v1;

        if (!((temp_v0 != 0) && (((temp_v1 == 0) | (temp_a2 != 0)) == 0))) {
            return temp_a2;
        }

        arg0++;
        arg1++;
    }
    return temp_a2;
}

void MapTLB(void) {
    s32 index;

    for (index = 0; index < 32; index++) {
        osMapTLB(index, OS_PM_4K, (void*)0x80000000, -1, -1, -1);
    }

    osMapTLB(30, OS_PM_4M, (void*)0xE0000000, 0, 0x400000, -1);
}

#define DRAM_OFFSET (0x300000)
void func_8020010C(s32 arg0) {
    s32 pi_write_length;
    s32 var_v1_3;
    RomFileEntry* file;
    s8* var_a0;
    s32 var_s0;
    u8* var_a1;

    for (var_s0 = 1; var_s0 <= D_802002C4.entryCount; var_s0++) {
        file = &D_802002C4.entries[var_s0 - 1];
        // search for "game" entry
        if (strcmp(file->name, D_80201970) == 0) {
            break;
        }
    }

    while (IO_READ(PI_STATUS_REG) & (PI_STATUS_DMA_BUSY | PI_STATUS_IO_BUSY)); // DMA Busy / IO Busy?
    IO_WRITE(PI_STATUS_REG, PI_STATUS_IO_BUSY); // IO Busy?
    IO_WRITE(PI_DRAM_ADDR_REG, DRAM_OFFSET);
    IO_WRITE(PI_CART_ADDR_REG, file->rom + 0x10000000);

    // align size to 8 bytes
    pi_write_length = (file->size + 7) & ~7;
    if (pi_write_length > 0xFFFF8) {
        pi_write_length = 0xFFFF8;
    }
    
    IO_WRITE(PI_WR_LEN_REG, pi_write_length);
    while (IO_READ(PI_STATUS_REG) & (PI_STATUS_DMA_BUSY | PI_STATUS_IO_BUSY)) {} // DMA Busy / IO Busy?
    IO_WRITE(PI_STATUS_REG, PI_STATUS_IO_BUSY); // IO Busy?
    
    MapTLB();
    func_802016E4(PHYS_TO_K0(DRAM_OFFSET), file->vram0);

    var_a0 = file->vram1;
    var_s0 = file->size_zero;
    for(var_s0--; var_s0 != -1; var_s0--, var_a0++) {
        *var_a0 = 0;
    }
    
    var_a1 = D_802002B0;
    var_a0 = D_E01301C0;
    var_v1_3 = 0x1E0;
    for (var_v1_3--; var_v1_3 != -1; var_v1_3--) {
        *var_a0++ = *var_a1++;
    }
    func_80002850(arg0, var_a1);
}

