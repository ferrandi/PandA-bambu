/*
+--------------------------------------------------------------------------+
| CHStone : a suite of benchmark programs for C-based High-Level Synthesis |
| ======================================================================== |
|                                                                          |
| * Collected and Modified : Y. Hara, H. Tomiyama, S. Honda,               |
|                            H. Takada and K. Ishii                        |
|                            Nagoya University, Japan                      |
|                                                                          |
| * Remark :                                                               |
|    1. This source code is modified to unify the formats of the benchmark |
|       programs in CHStone.                                               |
|    2. Test vectors are added for CHStone.                                |
|    3. If "main_result" is 0 at the end of the program, the program is    |
|       correctly executed.                                                |
|    4. Please follow the copyright of each benchmark program.             |
+--------------------------------------------------------------------------+
*/
/*
 * Copyright (C) 2008
 * Y. Hara, H. Tomiyama, S. Honda, H. Takada and K. Ishii
 * Nagoya University, Japan
 * All rights reserved.
 *
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis. The authors disclaims any and all warranties, 
 * whether express, implied, or statuary, including any implied warranties or 
 * merchantability or of fitness for a particular purpose. In no event shall the
 * copyright-holder be liable for any incidental, punitive, or consequential damages
 * of any kind whatsoever arising from the use of these programs. This disclaimer
 * of warranty extends to the user of these programs and user's customers, employees,
 * agents, transferees, successors, and assigns.
 *
 */
const unsigned long imem[44] = {
  0x8fa40000,			// [0x00400000]  lw $4, 0($29)                   ; 175: lw $a0 0($sp)               # argc
  0x27a50004,			// [0x00400004]  addiu $5, $29, 4                ; 176: addiu $a1 $sp 4             # argv
  0x24a60004,			// [0x00400008]  addiu $6, $5, 4                 ; 177: addiu $a2 $a1 4             # envp
  0x00041080,			// [0x0040000c]  sll $2, $4, 2                   ; 178: sll $v0 $a0 2
  0x00c23021,			// [0x00400010]  addu $6, $6, $2                 ; 179: addu $a2 $a2 $v0
  0x0c100016,			// [0x00400014]  jal 0x00400058 [main]           ; 180: jal main
  0x00000000,			// [0x00400018]  nop                             ; 181: nop
  0x3402000a,			// [0x0040001c]  ori $2, $0, 10                  ; 183: li $v0 10
  0x0000000c,			// [0x00400020]  syscall                         ; 184: syscall                     # syscall 10 (exit)
  0x3c011001,			// [0x00400024]  lui $1, 4097 [A]                ; 4: la   $t0,A           ; C&S
  0x34280000,			// [0x00400028]  ori $8, $1, 0 [A]
  0x00044880,			// [0x0040002c]  sll $9, $4, 2                   ; 5: sll  $t1,$a0,2
  0x01094821,			// [0x00400030]  addu $9, $8, $9                 ; 6: addu $t1,$t0,$t1
  0x8d2a0000,			// [0x00400034]  lw $10, 0($9)                   ; 7: lw   $t2,($t1)
  0x00055880,			// [0x00400038]  sll $11, $5, 2                  ; 8: sll  $t3,$a1,2
  0x010b5821,			// [0x0040003c]  addu $11, $8, $11               ; 9: addu $t3,$t0,$t3
  0x8d6c0000,			// [0x00400040]  lw $12, 0($11)                  ; 10: lw   $t4,($t3)
  0x018a682a,			// [0x00400044]  slt $13, $12, $10               ; 11: slt  $t5,$t4,$t2
  0x11a00003,			// [0x00400048]  beq $13, $0, 12 [L1-0x00400048] ; 12: beq  $t5,$zero,L1
  0xad2c0000,			// [0x0040004c]  sw $12, 0($9)                   ; 13: sw   $t4,($t1)
  0xad6a0000,			// [0x00400050]  sw $10, 0($11)                  ; 14: sw   $t2,($t3)
  0x03e00008,			// [0x00400054]  jr $31                          ; 15: jr   $ra            ; L1
  0x27bdfff4,			// [0x00400058]  addiu $29, $29, -12             ; 17: addiu $sp,$sp,-12   ; main
  0xafbf0008,			// [0x0040005c]  sw $31, 8($29)                  ; 18: sw   $ra,8($sp)
  0xafb10004,			// [0x00400060]  sw $17, 4($29)                  ; 19: sw   $s1,4($sp)
  0xafb00000,			// [0x00400064]  sw $16, 0($29)                  ; 20: sw   $s0,0($sp)
  0x24100000,			// [0x00400068]  addiu $16, $0, 0                ; 21: addiu $s0,$zero,0
  0x2a080008,			// [0x0040006c]  slti $8, $16, 8                 ; 22: slti $t0,$s0,8      ; L5
  0x1100000b,			// [0x00400070]  beq $8, $0, 44 [L2-0x00400070]  ; 23: beq  $t0,$zero,L2
  0x26110001,			// [0x00400074]  addiu $17, $16, 1               ; 24: addiu $s1,$s0,1
  0x2a280008,			// [0x00400078]  slti $8, $17, 8                 ; 25: slti $t0,$s1,8      ; L4
  0x11000006,			// [0x0040007c]  beq $8, $0, 24 [L3-0x0040007c]  ; 26: beq  $t0,$zero,L3
  0x26040000,			// [0x00400080]  addiu $4, $16, 0                ; 27: addiu $a0,$s0,0
  0x26250000,			// [0x00400084]  addiu $5, $17, 0                ; 28: addiu $a1,$s1,0
  0x0c100009,			// [0x00400088]  jal 0x00400024 [compare_swap]   ; 29: jal  compare_swap
  0x26310001,			// [0x0040008c]  addiu $17, $17, 1               ; 30: addiu $s1,$s1,1
  0x0810001e,			// [0x00400090]  j 0x00400078 [L4]               ; 31: j    L4
  0x26100001,			// [0x00400094]  addiu $16, $16, 1               ; 32: addiu $s0,$s0,1     ; L3
  0x0810001b,			// [0x00400098]  j 0x0040006c [L5]               ; 33: j    L5
  0x8fbf0008,			// [0x0040009c]  lw $31, 8($29)                  ; 34: lw   $ra,8($sp)     ; L2
  0x8fb10004,			// [0x004000a0]  lw $17, 4($29)                  ; 35: lw   $s1,4($sp)
  0x8fb00000,			// [0x004000a4]  lw $16, 0($29)                  ; 36: lw   $s0,0($sp)
  0x27bd000c,			// [0x004000a8]  addiu $29, $29, 12              ; 37: addiu $sp,$sp,12
  0x03e00008,			// [0x004000ac]  jr $31                          ; 38: jr   $ra
};
