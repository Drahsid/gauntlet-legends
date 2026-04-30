.include "macro.inc"

.set noreorder
.set noat
glabel osMapTLB
    mfc0       $t0, $10
    mtc0       $a0, $0
    mtc0       $a1, $5
    lw         $t1, 0x14($sp)
    addiu      $at, $zero, -1
    beq        $t1, $at, .LbootOsMapTLBGlobal
     addiu     $t4, $zero, 1
    addiu      $t2, $zero, 0x1E
    b          .LbootOsMapTLBSetEntryHi
     or        $a2, $a2, $t1
.LbootOsMapTLBGlobal:
    addiu      $t2, $zero, 0x1F
.LbootOsMapTLBSetEntryHi:
    mtc0       $a2, $10
    addiu      $at, $zero, -1
    beq        $a3, $at, .LbootOsMapTLBEvenInvalid
     nop
    srl        $t3, $a3, 6
    or         $t3, $t3, $t2
    mtc0       $t3, $2
    b          .LbootOsMapTLBOdd
     nop
.LbootOsMapTLBEvenInvalid:
    mtc0       $t4, $2
.LbootOsMapTLBOdd:
    lw         $t3, 0x10($sp)
    addiu      $at, $zero, -1
    beq        $t3, $at, .LbootOsMapTLBOddInvalid
     nop
    srl        $t3, $t3, 6
    or         $t3, $t3, $t2
    mtc0       $t3, $3
    b          .LbootOsMapTLBWrite
     nop
.LbootOsMapTLBOddInvalid:
    mtc0       $t4, $3
    addiu      $at, $zero, -1
    bne        $a3, $at, .LbootOsMapTLBWrite
     nop
    lui        $t3, 0x8000
    mtc0       $t3, $10
.LbootOsMapTLBWrite:
    nop
    tlbwi
    nop
    nop
    nop
    nop
    mtc0       $t0, $10
    jr         $ra
     nop
    nop
    nop
    nop
endlabel osMapTLB
.set at
.set reorder
