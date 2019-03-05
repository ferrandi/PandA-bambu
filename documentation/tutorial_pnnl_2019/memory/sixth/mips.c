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
#include <stdio.h>
int main_result;

#define R 0

#define ADDU 33
#define SUBU 35

#define MULT 24
#define MULTU 25

#define MFHI 16
#define MFLO 18

#define AND 36
#define OR 37
#define XOR 38
#define SLL 0
#define SRL 2
#define SLLV 4
#define SRLV 6

#define SLT 42
#define SLTU 43

#define JR 8

#define J 2
#define JAL 3

#define ADDIU 9
#define ANDI 12
#define ORI 13
#define XORI 14

#define LW 35
#define SW 43
#define LUI 15

#define BEQ 4
#define BNE 5
#define BGEZ 1

#define SLTI 10
#define SLTIU 11

#include "imem.h"
/*
+--------------------------------------------------------------------------+
| * Test Vectors (added for CHStone)                                       |
|     A : input data                                                       |
|     outData : expected output data                                       |
+--------------------------------------------------------------------------+
*/
const int A[8] = { 22, 5, -9, 3, -17, 38, 0, 11 };
const int outData[8] = { -17, -9, 0, 3, 5, 11, 22, 38 };

#define IADDR(x)	(((x)&0x000000ff)>>2)
#define DADDR(x)	(((x)&0x000000ff)>>2)

int
main ()
{
  long long hilo;
  int reg[32];
  int Hi = 0;
  int Lo = 0;
  int pc = 0;
  int dmem[64]={0};
  int j;

  unsigned int ins;
  int op;
  int rs;
  int rt;
  int rd;
  int shamt;
  int funct;
  short address;
  int tgtadr;

    while (1)
    {
      int i;
      int n_inst;

      n_inst = 0;
      main_result = 0;

      for (i = 0; i < 32; i++)
	{
	  reg[i] = 0;
	}
      reg[29] = 0x7fffeffc;

      for (i = 0; i < 8; i++)
	{
	  dmem[i] = A[i];
	}

      pc = 0x00400000;

      do
	{
	  ins = imem[IADDR (pc)];
	  pc = pc + 4;

	  op = ins >> 26;

	  switch (op)
	    {
	    case R:
	      funct = ins & 0x3f;
	      shamt = (ins >> 6) & 0x1f;
	      rd = (ins >> 11) & 0x1f;
	      rt = (ins >> 16) & 0x1f;
	      rs = (ins >> 21) & 0x1f;

	      switch (funct)
		{

		case ADDU:
		  reg[rd] = reg[rs] + reg[rt];
		  break;
		case SUBU:
		  reg[rd] = reg[rs] - reg[rt];
		  break;

		case MULT:
		  hilo = (long long) reg[rs] * (long long) reg[rt];
		  Lo = hilo & 0x00000000ffffffffULL;
		  Hi = ((int) (hilo >> 32)) & 0xffffffffUL;
		  break;
		case MULTU:
		  hilo =
		    (unsigned long long) ((unsigned int) (reg[rs])) *
		    (unsigned long long) ((unsigned int) (reg[rt]));
		  Lo = hilo & 0x00000000ffffffffULL;
		  Hi = ((int) (hilo >> 32)) & 0xffffffffUL;
		  break;

		case MFHI:
		  reg[rd] = Hi;
		  break;
		case MFLO:
		  reg[rd] = Lo;
		  break;

		case AND:
		  reg[rd] = reg[rs] & reg[rt];
		  break;
		case OR:
		  reg[rd] = reg[rs] | reg[rt];
		  break;
		case XOR:
		  reg[rd] = reg[rs] ^ reg[rt];
		  break;
		case SLL:
		  reg[rd] = reg[rt] << shamt;
		  break;
		case SRL:
		  reg[rd] = reg[rt] >> shamt;
		  break;
		case SLLV:
		  reg[rd] = reg[rt] << reg[rs];
		  break;
		case SRLV:
		  reg[rd] = reg[rt] >> reg[rs];
		  break;

		case SLT:
		  reg[rd] = reg[rs] < reg[rt];
		  break;
		case SLTU:
		  reg[rd] = (unsigned int) reg[rs] < (unsigned int) reg[rt];
		  break;

		case JR:
		  pc = reg[rs];
		  break;
		default:
		  pc = 0;	// error
		  break;
		}
	      break;

	    case J:
	      tgtadr = ins & 0x3ffffff;
	      pc = tgtadr << 2;
	      break;
	    case JAL:
	      tgtadr = ins & 0x3ffffff;
	      reg[31] = pc;
	      pc = tgtadr << 2;
	      break;

	    default:

	      address = ins & 0xffff;
	      rt = (ins >> 16) & 0x1f;
	      rs = (ins >> 21) & 0x1f;
	      switch (op)
		{
		case ADDIU:
		  reg[rt] = reg[rs] + address;
		  break;

		case ANDI:
		  reg[rt] = reg[rs] & (unsigned short) address;
		  break;
		case ORI:
		  reg[rt] = reg[rs] | (unsigned short) address;
		  break;
		case XORI:
		  reg[rt] = reg[rs] ^ (unsigned short) address;
		  break;

		case LW:
		  reg[rt] = dmem[DADDR (reg[rs] + address)];
		  break;
		case SW:
		  dmem[DADDR (reg[rs] + address)] = reg[rt];
		  break;

		case LUI:
		  reg[rt] = address << 16;
		  break;

		case BEQ:
		  if (reg[rs] == reg[rt])
		    pc = pc - 4 + (address << 2);
		  break;
		case BNE:
		  if (reg[rs] != reg[rt])
		    pc = pc - 4 + (address << 2);
		  break;
		case BGEZ:
		  if (reg[rs] >= 0)
		    pc = pc - 4 + (address << 2);
		  break;

		case SLTI:
		  reg[rt] = reg[rs] < address;
		  break;

		case SLTIU:
		  reg[rt] = (unsigned int) reg[rs] < (unsigned short) address;
		  break;

		default:
		  pc = 0;	/* error */
		  break;
		}
	      break;
	    }
	  reg[0] = 0;
	  n_inst = n_inst + 1;
	}
      while (pc != 0);

      main_result += (n_inst != 611);
      for (j = 0; j < 8; j++)
	{
	  main_result += (dmem[j] != outData[j]);
	}

      printf ("%d\n", main_result);
      return main_result;
    }
}
