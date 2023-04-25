#ifndef INCLUDE_ASM_H
#define INCLUDE_ASM_H

#if !defined(SPLAT) && !defined(M2CTX) && !defined(__CTX__) && !defined(PERMUTER)
__asm__(".include \"include/macro.inc\"\n");
#ifndef INCLUDE_ASM
#define INCLUDE_ASM(FOLDER, NAME) \
    __asm__( \
        ".section .text\n" \
        "    .set noat\n" \
        "    .set noreorder\n" \
        "    .align    2\n" \
        "    .globl    "#NAME"\n" \
        "    .type "#NAME", @function\n" \
        "    .ent    "#NAME"\n" \
        #NAME ":\n" \
        "    .include \""FOLDER"/"#NAME".s\"\n" \
        "    .set reorder\n" \
        "    .set at\n" \
        "    .end    "#NAME"\n" \
        ".end"#NAME":\n" \
        "    .size    "#NAME",.end"#NAME"-"#NAME "\n" \
        "    .globl    " #NAME ".NON_MATCHING\n" \
        "    " #NAME ".NON_MATCHING" " = " #NAME "\n" \
    )
#endif
#ifndef INCLUDE_RODATA
#define INCLUDE_RODATA(FOLDER, NAME) \
    __asm__( \
        ".section .rodata\n" \
        "    .include \""FOLDER"/"#NAME".s\"\n" \
        ".section .text" \
    )
#endif
#else
#ifndef INCLUDE_ASM
#define INCLUDE_ASM(FOLDER, NAME)
#endif
#ifndef INCLUDE_RODATA
#define INCLUDE_RODATA(FOLDER, NAME)
#endif
#endif

#endif
