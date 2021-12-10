#include "common.h"

#ifdef NON_MATCHING
// are these even right?
uint8_t* D_80201980 = 0xE47D7D70;
uint8_t* D_80201984 = 0x54D795E7;
uint32_t D_80201988 = 0x79AF3F68;
uint32_t D_8020198C = 0x93B67988;
uint8_t* D_80201990 = 0x366E7B55;
uint32_t D_80201994 = 0x763FF124;
uint32_t D_80201998 = 0xDA84A41A;
uint32_t D_8020199C = 0xFC122B49;
#endif

void func_80200490(void* arg0, uint32_t arg1) {
    D_80201990 = (uint32_t*)arg0;
    D_80201998 = arg1;
    D_80201994 = 0;
}

void* func_802004AC(int32_t arg0) {
    D_80201994 += arg0;
    return D_80201990 + (D_80201994 - arg0);
}

GLOBAL_ASM("asm/nonmatchings/deflate/func_802004D4.s")

// TODO further fold, fix control flow
#ifdef NON_MATCHING
int32_t func_80200A3C(int32_t arg0, int32_t arg1, int32_t arg2, int32_t arg3) {
    s32 phi_v0;
    u8* phi_t2;
    u32 phi_t0;
    u32 phi_t1;
    u8* phi_a0;
    s32 phi_t3;

    phi_t0 = D_8020198C;
    phi_t1 = D_80201988;

    while (phi_t0 < (u32)arg2) {
        if (phi_v0 != 0) {
            phi_t0 += 8;
            phi_t1 |= *D_80201980++ << phi_t0;
            continue;
        }

        phi_t2 = arg0 + ((phi_t1 & *(&D_80201878 + (arg2 * 2))) * 8);

        while ((u32)phi_t2->unk0 < 0x11U) {
            if ((u32)phi_t2->unk0 < 0x11U) {
                phi_t0 -= phi_t2->unk1;
                phi_t1 = phi_t1 >> phi_t2->unk1;

                if (phi_t2->unk0 == 0x10) {
                    *D_80201984++ = (u8)(u16)phi_t2->unk4;
                    continue;
                }

                if (phi_t2->unk0 != 0xF) {
                    phi_v0 = phi_t2->unk0 * 2;
                    if (phi_t0 - phi_t2->unk1 < (u32)phi_t2->unk0) {
                        do {
                            phi_t0 += +8;
                            phi_t1 |= | (*D_80201980++ << phi_t0);
                            phi_v0 = phi_t2->unk0 * 2;
                        } while (phi_t0 < (u32)phi_t2->unk0);
                    }

                    phi_t0 -= phi_t2->unk0;
                    phi_t1 = phi_t1 >> phi_t2->unk0;
                    if (phi_t0 - phi_t2->unk0 < (u32)arg3) {
                        do {
                            phi_t0 += 8;
                            phi_t1 |= (*D_80201980++ << phi_t0);
                        } while (phi_t0 < (u32)arg3);
                    }

                    phi_t2 = arg1 + ((phi_t1 & *(&D_80201878 + (arg3 * 2))) * 8);

                    while ((u32)phi_t2->unk0 < 0x11U) {
                        if ((u32)phi_t2->unk0 < 0x11U) {
                            phi_t0 -= phi_t2->unk1;
                            phi_t1 = phi_t1 >> phi_t2->unk1;
                            if (phi_t0 - phi_t2->unk1 < (u32)phi_t2->unk0) {
                                do {
                                    phi_t0 += 8;
                                    phi_t1 |= (*D_80201980++ << phi_t0);
                                } while (phi_t0 < (u32)phi_t2->unk0);
                            }

                            phi_a0 = &D_80201984[-(s32)(u16)phi_t2->unk4 -
                                (phi_t1 & *((phi_t2->unk0 * 2) + &D_80201878))];
                            phi_t3 =
                                ((u16)phi_t2->unk4 + (phi_t1 & *(phi_v0_2 + &D_80201878))) - 1;
                            ;
                            phi_t0 -= phi_t2->unk0;
                            phi_t1 = phi_t1 >> phi_t2->unk0;
                            if (((u16)phi_t2->unk4 + (phi_t1 & *(phi_v0_2 + &D_80201878))) - 1;
                                != -1) {
                                do {
                                    *D_80201984++ = *phi_a0;
                                    phi_a0 += 1;
                                    phi_t3 = phi_t3 - 1;
                                } while (phi_t3 - 1 != -1);
                                phi_v0 = phi_t0 - phi_t2->unk0 < (u32)arg2;
                            }
                            else {
                                continue;
                            }
                            continue;
                        }

                        if (phi_t2->unk0 == 0x63) {
                            return 1;
                        }

                        phi_t0 -= phi_t2->unk1;
                        phi_t1 = phi_t1 >> phi_t2->unk1;
                        phi_v0 = phi_t2->unk0 - 0x10 * 2;
                        if (phi_t0 - phi_t2->unk1 < phi_t2->unk0 - 0x10) {
                            do {
                                phi_t0 += 8;
                                phi_t1 |= *D_80201980++ << phi_t0;
                                phi_v0 = phi_t2->unk0 - 0x10 * 2;
                            } while (phi_t0 < phi_t2->unk0 - 0x10);
                        }
                        phi_t2 = phi_t2->unk4 + ((phi_t1 & *(phi_v0 + &D_80201878)) * 8);
                        phi_t1 = phi_t1;
                    }
                }

                D_80201988 = phi_t1 >> phi_t2->unk1;
                D_8020198C = phi_t0 - phi_t2->unk1;
                return 0;
            }

            if (temp_a0 != 0x63) {
                phi_t0 = phi_t0 - phi_t2->unk1;
                phi_t1 = phi_t1 >> phi_t2->unk1;
                phi_v0 = temp_a0 - 0x10 * 2;
                if (phi_t0 - phi_t2->unk1 < temp_a0 - 0x10) {
                    do {
                        phi_t0 = phi_t0 + 8;
                        phi_t1 = phi_t1 | (*D_80201980++ << phi_t0);
                        phi_v0 = temp_a0 - 0x10 * 2;
                    } while (phi_t0 < temp_a0 - 0x10);
                }

                phi_t2 = phi_t2->unk4 + ((phi_t1 & *(phi_v0 + &D_80201878)) * 8);
                phi_t1 = phi_t1;
            }
        }
    }

    return 1;
}
#else
GLOBAL_ASM("asm/nonmatchings/deflate/func_80200A3C.s")
#endif

uint32_t func_80200D68(void) {
    uint32_t phi_a0 = D_8020198C;
    uint32_t phi_a1 = D_80201988;
    uint32_t phi_a2;

    phi_a2 = phi_a0 & 7;
    phi_a0 -= phi_a2;
    phi_a1 = phi_a1 >> phi_a2;

    while (phi_a0 < 0x10) {
        phi_a1 |= *((uint8_t*)D_80201980++) << phi_a0;
        phi_a0 += 8;
    }

    phi_a2 = (uint16_t)phi_a1;
    phi_a0 -= 0x10;
    phi_a1 = phi_a1 >> 0x10;
    while (phi_a0 < 0x10) {
        phi_a1 |= *((uint8_t*)D_80201980++) << phi_a0;
        phi_a0 += 8;
    }

    if (phi_a2 != (uint16_t)(~phi_a1)) {
        return 1;
    }

    phi_a1 = phi_a1 >> 0x10;

    phi_a0 -= 0x10;
    while (phi_a2--) {
        while (phi_a0 < 8) {
            phi_a1 |= *((uint8_t*)D_80201980++) << phi_a0;
            phi_a0 += 8;
        }
        phi_a0 -= 8;

        *((uint8_t*)D_80201984++) = phi_a1;

        phi_a1 = phi_a1 >> 8;
    }

    D_80201988 = phi_a1;
    D_8020198C = phi_a0;

    return 0;
}

// OK!ish One instruction off, just regalloc
#ifdef NON_MATCHING
int32_t func_80200EA0(void) {
    uint32_t sp20[0x120];
    int32_t sp4A0;
    int32_t sp4A4;
    int32_t sp4A8;
    int32_t sp4AC;
    int32_t index;

    for (index = 0; index < 0x90; index++) {
        sp20[index] = 8;
    }

    for (index = 0x90; index < 0x100; index++) {
        sp20[index] = 9;
    }

    while (index < 0x118) {
        sp20[index++] = 7;
    }

    while (index < 0x120) {
        sp20[index++] = 8;
    }

    sp4A4 = 7;
    index = func_802004D4(sp20, 0x120, 0x101, &D_80201780, &D_802017C0, &sp4A0, &sp4A4);
    if (index != 0) {
        return index;
    }
    while (index < 0x1E) {
        sp20[index++] = 5;
    }

    sp4AC = 5;
    index = func_802004D4(sp20, 0x1E, 0, &D_80201800, &D_8020183C, &sp4A8, &sp4AC);
    if (index >= 2) {
        D_80201994 = 0;
        return index;
    }

    index = func_80200A3C(sp4A0, sp4A8, sp4A4, sp4AC);
    if (index == 0) {
        index = 0;
        D_80201994 = 0;
    }
    else if (index >= 0) {
        return 1;
    }
    else {
        return index;
    }

    return index;
}
#else
GLOBAL_ASM("asm/nonmatchings/deflate/func_80200EA0.s")
#endif

GLOBAL_ASM("asm/nonmatchings/deflate/func_80201038.s")

uint32_t func_802015B0(uint32_t* arg0) {
    uint32_t phi_a1 = D_8020198C;
    uint32_t phi_a2 = D_80201988;
    uint32_t temp;

    while (phi_a1 == 0) {
        phi_a2 |= *((uint8_t*)D_80201980++) << phi_a1;
        phi_a1 += 8;
    }

    *arg0 = phi_a2 & 1;
    phi_a2 = phi_a2 >> 1;

    phi_a1 -= 1;
    while (phi_a1 < 2) {
        phi_a2 |= *((uint8_t*)D_80201980++) << phi_a1;
        phi_a1 += 8;
    }

    temp = phi_a2 & 3;

    phi_a2 = phi_a2 >> 2;
    D_80201988 = phi_a2;

    phi_a1 = phi_a1 - 2;
    D_8020198C = phi_a1;

    if (temp == 2) {
        return func_80201038();
    }
    else if (temp == 0) {
        return func_80200D68();
    }
    else if (temp == 1) {
        return func_80200EA0();
    }

    return 2;
}

uint32_t func_802016A4(void) {
    int32_t sp10;
    int32_t phi_v0;

    D_8020198C = 0;
    D_80201988 = 0;

    while (1) {
        phi_v0 = func_802015B0(&sp10);
        if (phi_v0 == 0) {
            phi_v0 = 0;
            if (sp10 != 0) {
                return phi_v0;
            }
        }
        else {
            return phi_v0;
        }
    }
}

uint8_t* func_802016E4(uint8_t* arg0, uint8_t* arg1) {
    D_80201984 = arg1;
    D_80201980 = arg0;
    func_80200490((void*)0x80220000, 0x5000);
    func_802016A4();

    return D_80201980 - arg0;
}

