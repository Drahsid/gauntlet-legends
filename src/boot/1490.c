#include "common.h"

void func_80200490(s32 arg0, s32 arg1);
void func_802016A4(void);

extern u8* D_80201980;
extern s32 D_80201984;
extern u32 D_80201988;
extern s32 D_8020198C;
extern s32 D_80201990;
extern s32 D_80201994;
extern s32 D_80201998;

void func_80200490(s32 arg0, s32 arg1) {
    D_80201990 = arg0;
    D_80201998 = arg1;
    D_80201994 = 0;
}

s32 func_802004AC(s32 arg0) {
    s32 temp_v0 = D_80201994 + arg0;
    D_80201994 = temp_v0;
    return D_80201990 + (temp_v0 - arg0);
}

INCLUDE_ASM("asm/boot/nonmatchings/1490", func_802004D4);

INCLUDE_ASM("asm/boot/nonmatchings/1490", func_80200A3C);

INCLUDE_ASM("asm/boot/nonmatchings/1490", func_80200D68);

INCLUDE_ASM("asm/boot/nonmatchings/1490", func_80200EA0);

INCLUDE_ASM("asm/boot/nonmatchings/1490", func_80201038);

INCLUDE_ASM("asm/boot/nonmatchings/1490", func_802015B0);

INCLUDE_ASM("asm/boot/nonmatchings/1490", func_802016A4);

u32 func_802016E4(void* arg0, s32 arg1) {
    D_80201984 = arg1;
    D_80201980 = arg0;
    func_80200490(0x80220000, 0x5000);
    func_802016A4();
    return (u32)D_80201980 - (u32)arg0;
}

INCLUDE_ASM("asm/boot/nonmatchings/1490", func_80201734);
