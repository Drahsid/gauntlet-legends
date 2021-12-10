#include "common.h"

GLOBAL_ASM("asm/nonmatchings/1050/func_80200050.s")

void MapTLB(void) {
    int32_t index;

    for (index = 0; index < 32; index++) {
        osMapTLB(index, OS_PM_4K, (void*)0x80000000, -1, -1, -1);
    }

    osMapTLB(30, OS_PM_4M, (void*)0xE0000000, 0, 0x400000, -1);
}

// just here for pseudocode, might be handwritten
#ifdef NON_MATCHING
void func_8020010C(s32 arg0) {
    s32 temp_a0;
    s32 temp_s0_2;
    s32 temp_s0_3;
    s32 temp_v1;
    s32 temp_v1_2;
    u32 temp_s0;
    u8* temp_a1;
    s32* phi_v1;
    u32 phi_s0;
    s32* phi_s1;
    s32 phi_v1_2;
    s8* phi_a0;
    s32 phi_s0_2;
    u8* phi_a1;
    u8* phi_a0_2;
    s32 phi_v1_3;

    phi_s0 = 1U;
    phi_s1 = saved_reg_s1;
    if (D_802002C4 != 0) {
        phi_v1 = &D_802002C4 + 0x1C;
loop_2:
        phi_s1 = phi_v1;
        if (func_80200050(phi_v1, &D_80201970) != 0) {
            temp_s0 = phi_s0 + 1;
            phi_v1 += 0x30;
            phi_s0 = temp_s0;
            if ((u32) (&D_802002C4 - 0x14)->unk14 >= temp_s0) {
                goto loop_2;
            }
        }
    }

    do {

    } while ((*(s32* )0xA4600010 & 3) != 0);

    *(void* )0xA4600010 = 2;
    *(s32* )0xA4600000 = 0x300000;
    *(s32* )0xA4600004 = phi_s1->unk10 + 0x10000000;
    temp_v1 = (phi_s1->unk14 + 7) & ~7;
    temp_a0 = temp_v1 > 0xFFFF8;
    phi_v1_2 = temp_v1;
    if (temp_a0 != 0) {
        phi_v1_2 = 0xFFFF8;
    }

    *(s32* )0xA460000C = phi_v1_2;
    if ((*(void* )0xA4600010 & 3) != 0) {
        do {

        } while ((*(void* )0xA4600010 & 3) != 0);
    }

    *(void* )0xA4600010 = 2;
    MapTLB(temp_a0, (void* )0xA4600010);
    func_802016E4((u8* )0x80300000, phi_s1->unk18); // decompress
    temp_s0_2 = phi_s1->unk24 - 1;
    phi_a0 = phi_s1->unk20;
    phi_s0_2 = temp_s0_2;
    if (temp_s0_2 != -1) {
        do {
            *phi_a0 = 0;
            temp_s0_3 = phi_s0_2 - 1;
            phi_a0 += 1;
            phi_s0_2 = temp_s0_3;
        } while (temp_s0_3 != -1);
    }

    phi_a1 = &D_802002B0;
    phi_a0_2 = &D_E01301C0;
    phi_v1_3 = 0x1DF;
    do {
        temp_a1 = phi_a1 + 1;
        temp_v1_2 = phi_v1_3 - 1;
        *phi_a0_2 = *phi_a1;
        phi_a1 = temp_a1;
        phi_a0_2 += 1;
        phi_v1_3 = temp_v1_2;
    } while (temp_v1_2 != -1);

    func_80002850(arg0, temp_a1, -1); // main
}
#else
GLOBAL_ASM("asm/nonmatchings/1050/func_8020010C.s")
#endif
