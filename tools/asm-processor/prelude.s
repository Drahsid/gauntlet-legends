.set noat
.set noreorder
.macro glabel label
    .global \label
    \label:
.endm

