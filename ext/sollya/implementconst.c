/*

Copyright 2010-2011 by

LORIA (CNRS, INPL, INRIA, UHP, U-Nancy 2),

Centre de recherche INRIA Sophia-Antipolis Mediterranee, equipe APICS,
Sophia Antipolis, France,

and by

Laboratoire d'Informatique de Paris 6, equipe PEQUAN,
UPMC Universite Paris 06 - CNRS - UMR 7606 - LIP6, Paris, France.

Contributors S. Chevillard, Ch. Lauter

sylvain.chevillard@ens-lyon.org
christoph.lauter@ens-lyon.org

This software is a computer program whose purpose is to provide an
environment for safe floating-point code development. It is
particularily targeted to the automatized implementation of
mathematical floating-point libraries (libm). Amongst other features,
it offers a certified infinity norm, an automatic polynomial
implementer and a fast Remez algorithm.

This software is governed by the CeCILL-C license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL-C
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL-C license and that you accept its terms.

This program is distributed WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "expression.h"
#include "infnorm.h"
#include "general.h"

#define BUFFERSIZE 64

/* A program is given by a list of instructions, the index number of the first
   unused temporary variable in the program, the number of temporary variables
   used by the program and a list giving, for each temporary variable, the
   maximum of the precision that it takes.      
*/
struct implementCsteProgram {
  chain *instructions;
  int counter;
  int maxcounter;
  chain *precisions;
};

typedef struct implementCsteCouple {
  int var;
  long int prec;
} couple;

couple *makeCouple(int var, long int prec) {
  couple *res = safeMalloc(sizeof(couple));
  res->var=var;
  res->prec=prec;
  return res;
}

void *copyCouple(void *c) {
  couple *ptr, *newCouple;
  ptr = (couple *)c;
  newCouple = safeMalloc(sizeof(couple));
  newCouple->var = ptr->var;
  newCouple->prec = ptr->prec;
  return (void *)ptr;
}

/* This functions looks for the variable var in program. If it is absent, it
   adds the couple (var, prec) to program->precisions. If there already is an
   occurence (var, prec2) in program->precisions, it compares prec and prec2
   and keeps the largest.
*/
void appendPrecisionProg(int var, long int prec, struct implementCsteProgram *program) {
  chain *curr;
  int test = 0;
  curr = program->precisions;
  while ( (curr != NULL) && (!test) ) {
    if ( ((couple *)(curr->value))->var == var) {
      test = 1;
      if (prec > ((couple *)(curr->value))->prec) ((couple *)(curr->value))->prec = prec;
    }
    else curr = curr->next;
  }
  if (!test) program->precisions = addElement(program->precisions, makeCouple(var, prec));
  return;
}

void incrementProgramCounter(struct implementCsteProgram *program) {
  program->counter++;
  if (program->counter >= program->maxcounter) program->maxcounter = program->counter;
  return;
}

/* Possible instructions:
   - mpfr_init2(var1, prec)
   - mpfr_set_prec(var1, prec)
   - 0ary function: name(var1, MPFR_RNDN)
   - unary function: name(var1, var2, MPFR_RNDN)
   - binary function: name(var1, var2, var3, MPFR_RNDN)
   - mpfr_set_ui(var1, valui, MPFR_RNDN)
   - mpfr_set_si(var1, valsi, MPFR_RNDN)
   - mpfr_set_str(var1, valstr, 2, MPFR_RNDN)
   - mpfr_ui_pow_ui(var1, valui, valui2, MPFR_RNDN);
   - mpfr_pow_ui(var1, var2, valui, MPFR_RNDN);
   - mpfr_root(var1, var2, valui, MPFR_RNDN);
   - valstr(var1, prec); for a library constant
   - if(valstr) {prog1} else {prog2}
*/
#define INIT2 0
#define SETPREC 1
#define CONSTANTFUNC 2
#define UNARYFUNC 3
#define BINARYFUNC 4
#define SETUI 5
#define SETSI 6
#define SETSTR 7
#define UIPOWUI 8
#define POWUI 9
#define ROOT 10
#define LIBRARYCONST 11
#define IFTHENELSE 12

/* Good practice: an instruction should always be created with a
   function appendSomeInstructionProg. Moreover strval, prog1.instructions,
   prog1.precisions, prog2.instructions and prog2.precisions must
   be initialized with NULL: hence this value can be tested 
   when freeing memory, in order to know if the char * has been
   malloced or not.
   This implies modifying all the append*Prog, when a new kind of
   instruction is added with such a malloced parameter.
   WARNING: in the implementation of if-then-else, the main program
   and the branch programs share the same precisions list. In order
   to avoid double freeing, one must explicitely set the precisions
   list of prog1 and prog2 to NULL after the construction of the
   if-then-else statement.
*/
struct implementCsteInstruction {
  int type;
  char var1[BUFFERSIZE];
  char var2[BUFFERSIZE];
  char var3[BUFFERSIZE];
  char name[BUFFERSIZE];
  long int prec;
  unsigned long int uival;
  unsigned long int uival2;
  long sival;
  char *strval;
  struct implementCsteProgram prog1;
  struct implementCsteProgram prog2;
};

void free_implementCsteInstruction(void *instr) {
  struct implementCsteInstruction *ptr;

  ptr = (struct implementCsteInstruction *)instr;
  if (ptr->strval != NULL )  free(ptr->strval);

  freeChain(ptr->prog1.instructions, free_implementCsteInstruction);
  freeChain(ptr->prog1.precisions, free);
  freeChain(ptr->prog2.instructions, free_implementCsteInstruction);
  freeChain(ptr->prog2.precisions, free);

  free(ptr);
  return;
}

void *copy_implementCsteInstructions(void *instr) {
  struct implementCsteInstruction *ptr, *newInstr;
  ptr = (struct implementCsteInstruction *)instr;
  newInstr = safeMalloc(sizeof(struct implementCsteInstruction));
  newInstr->type = ptr->type;
  strcpy(newInstr->var1, ptr->var1);
  strcpy(newInstr->var2, ptr->var2);
  strcpy(newInstr->var3, ptr->var3);
  strcpy(newInstr->name, ptr->name);
  newInstr->prec = ptr->prec;
  newInstr->uival = ptr->uival;
  newInstr->uival2 = ptr->uival2;
  newInstr->sival = ptr->sival;
  if(ptr->strval != NULL) {
    newInstr->strval = safeCalloc(1+strlen(ptr->strval) , sizeof(char));
    strcpy(newInstr->strval, ptr->strval);
  }
  else newInstr->strval = NULL;

  newInstr->prog1.instructions = copyChainWithoutReversal(ptr->prog1.instructions, copy_implementCsteInstructions);
  newInstr->prog1.counter = ptr->prog1.counter;
  newInstr->prog1.maxcounter = ptr->prog1.maxcounter;
  newInstr->prog1.precisions = copyChainWithoutReversal(ptr->prog1.precisions, copyCouple);

  newInstr->prog2.instructions = copyChainWithoutReversal(ptr->prog2.instructions, copy_implementCsteInstructions);
  newInstr->prog2.counter = ptr->prog2.counter;
  newInstr->prog2.maxcounter = ptr->prog2.maxcounter;
  newInstr->prog2.precisions = copyChainWithoutReversal(ptr->prog2.precisions, copyCouple);

  return (void *)newInstr;
}

void fprintInstruction(FILE *output, struct implementCsteInstruction instr, int indentlevel) {
  const char init_string[]="mpfr_init2";
  const char setprec_string[]="mpfr_set_prec";
  const char *ptr;
  chain * curr;
  char *indent;
  int i;

  indent = safeCalloc(indentlevel*2+1, sizeof(char));
  for(i=0;i<indentlevel*2;i++) indent[i] = ' ';
  indent[i] = '\0';

  switch (instr.type) {
  case INIT2:
  case SETPREC:
  case LIBRARYCONST:
    if (instr.type == INIT2) ptr=init_string;
    else if (instr.type == SETPREC) ptr=setprec_string;
    else ptr = instr.strval;

    if (instr.prec > 0)
      sollyaFprintf(output, "%s%s (%s, prec+%d);\n", indent, ptr, instr.var1, instr.prec);
    else if (instr.prec == 0)
      sollyaFprintf(output, "%s%s (%s, prec);\n", indent, ptr, instr.var1);
    else {
      sollyaFprintf(output, "%sif (prec >= %d+MPFR_PREC_MIN)\n", indent, -instr.prec);
      sollyaFprintf(output, "%s{\n", indent);
      sollyaFprintf(output, "%s  %s (%s, prec-%d);\n", indent, ptr, instr.var1, -instr.prec);
      sollyaFprintf(output, "%s}\n", indent);
      sollyaFprintf(output, "%selse\n", indent);
      sollyaFprintf(output, "%s{\n", indent);
      sollyaFprintf(output, "%s  %s (%s, MPFR_PREC_MIN);\n", indent, ptr, instr.var1);
      sollyaFprintf(output, "%s}\n", indent);
    }
    break;
  case CONSTANTFUNC:
    sollyaFprintf(output, "%s%s (%s, MPFR_RNDN);\n", indent, instr.name, instr.var1);
    break;
  case UNARYFUNC:
    sollyaFprintf(output, "%s%s (%s, %s, MPFR_RNDN);\n", indent, instr.name, instr.var1, instr.var2);
    break;
  case BINARYFUNC:
    sollyaFprintf(output, "%s%s (%s, %s, %s, MPFR_RNDN);\n", indent, instr.name, instr.var1, instr.var2, instr.var3);
    break;
  case SETUI:
    sollyaFprintf(output, "%smpfr_set_ui (%s, %lu, MPFR_RNDN);\n", indent, instr.var1, instr.uival);
    break;
  case SETSI:
    sollyaFprintf(output, "%smpfr_set_si (%s, %ld, MPFR_RNDN);\n", indent, instr.var1, instr.sival);
    break;
  case SETSTR:
    sollyaFprintf(output, "%smpfr_set_str (%s, \"%s\", 2, MPFR_RNDN);\n", indent, instr.var1, instr.strval);
    break;
  case UIPOWUI:
    sollyaFprintf(output, "%smpfr_ui_pow_ui (%s, %lu, %lu, MPFR_RNDN);\n", indent, instr.var1, instr.uival, instr.uival2);
    break;
  case POWUI:
    sollyaFprintf(output, "%smpfr_pow_ui (%s, %s, %lu, MPFR_RNDN);\n", indent, instr.var1, instr.var2, instr.uival);
    break;
  case ROOT:
    sollyaFprintf(output, "%smpfr_root (%s, %s, %lu, MPFR_RNDN);\n", indent, instr.var1, instr.var2, instr.uival);
    break;
  case IFTHENELSE:
    sollyaFprintf(output, "%sif (%s)\n", indent, instr.strval);
    sollyaFprintf(output, "%s{\n", indent);
    curr = instr.prog1.instructions;
    while(curr!=NULL) {
      fprintInstruction(output, *(struct implementCsteInstruction *)(curr->value), indentlevel+1);
      curr = curr->next;
    }
    sollyaFprintf(output, "%s}\n", indent);
    sollyaFprintf(output, "%selse\n", indent);
    sollyaFprintf(output, "%s{\n", indent);
    curr = instr.prog2.instructions;
    while(curr!=NULL) {
      fprintInstruction(output, *(struct implementCsteInstruction *)(curr->value), indentlevel+1);
      curr = curr->next;
    }
    sollyaFprintf(output, "%s}\n", indent);
    break;
  default: 
    sollyaFprintf(stderr, "Unknown instruction %d\n", instr.type);
  }

  free(indent);
  return;
}

void constructName(char *res, int counter) {
  if (counter==0) strcpy(res, "y");
  else sprintf(res, "tmp%d", counter);
  return;
}


/* These constructors allow one for adding an instruction to the end of a */
/* give program.                                                          */
void appendInit2Prog(int var1, long int prec, struct implementCsteProgram *program) {
  struct implementCsteInstruction *instr;
  instr = safeMalloc(sizeof(struct implementCsteInstruction));
  instr->type = INIT2;
  constructName(instr->var1, var1);
  strcpy(instr->var2, "");
  strcpy(instr->var3, "");
  strcpy(instr->name, "");
  instr->prec = prec;
  instr->strval = NULL;
  instr->prog1.instructions = NULL;
  instr->prog1.precisions = NULL;
  instr->prog2.instructions = NULL;
  instr->prog2.precisions = NULL;
  program->instructions = addElement(program->instructions, instr);
  return;
}

void appendSetprecProg(int var1, long int prec, struct implementCsteProgram *program) {
  struct implementCsteInstruction *instr;
  instr = safeMalloc(sizeof(struct implementCsteInstruction));
  instr->type = SETPREC;
  constructName(instr->var1, var1);
  strcpy(instr->var2, "");
  strcpy(instr->var3, "");
  strcpy(instr->name, "");
  instr->prec = prec;
  instr->strval = NULL;
  instr->prog1.instructions = NULL;
  instr->prog1.precisions = NULL;
  instr->prog2.instructions = NULL;
  instr->prog2.precisions = NULL;
  program->instructions = addElement(program->instructions, instr);
  appendPrecisionProg(var1, prec, program);
  return;
}

void appendConstantfuncProg(char *name, int var1, struct implementCsteProgram *program) {
  struct implementCsteInstruction *instr;
  instr = safeMalloc(sizeof(struct implementCsteInstruction));
  instr->type = CONSTANTFUNC;
  constructName(instr->var1, var1);
  strcpy(instr->var2, "");
  strcpy(instr->var3, "");
  strcpy(instr->name, name);
  instr->strval = NULL;
  instr->prog1.instructions = NULL;
  instr->prog1.precisions = NULL;
  instr->prog2.instructions = NULL;
  instr->prog2.precisions = NULL;
  program->instructions = addElement(program->instructions, instr);
  return;
}

void appendUnaryfuncProg(char *name, int var1, int var2, struct implementCsteProgram *program) {
  struct implementCsteInstruction *instr;
  instr = safeMalloc(sizeof(struct implementCsteInstruction));
  instr->type = UNARYFUNC;
  constructName(instr->var1, var1);
  constructName(instr->var2, var2);
  strcpy(instr->var3, "");
  strcpy(instr->name, name);
  instr->strval = NULL;
  instr->prog1.instructions = NULL;
  instr->prog1.precisions = NULL;
  instr->prog2.instructions = NULL;
  instr->prog2.precisions = NULL;
  program->instructions = addElement(program->instructions, instr);
  return;
}

void appendBinaryfuncProg(char *name, int var1, int var2, int var3, struct implementCsteProgram *program) {
  struct implementCsteInstruction *instr;
  instr = safeMalloc(sizeof(struct implementCsteInstruction));
  instr->type = BINARYFUNC;
  constructName(instr->var1, var1);
  constructName(instr->var2, var2);
  constructName(instr->var3, var3);
  strcpy(instr->name, name);
  instr->strval = NULL;
  instr->prog1.instructions = NULL;
  instr->prog1.precisions = NULL;
  instr->prog2.instructions = NULL;
  instr->prog2.precisions = NULL;
  program->instructions = addElement(program->instructions, instr);
  return;
}

void appendSetuiProg(int var1, unsigned long int val, struct implementCsteProgram *program) {
  struct implementCsteInstruction *instr;
  instr = safeMalloc(sizeof(struct implementCsteInstruction));
  instr->type = SETUI;
  constructName(instr->var1, var1);
  strcpy(instr->var2, "");
  strcpy(instr->var3, "");
  strcpy(instr->name, "");
  instr->uival = val;
  instr->strval = NULL;
  instr->prog1.instructions = NULL;
  instr->prog1.precisions = NULL;
  instr->prog2.instructions = NULL;
  instr->prog2.precisions = NULL;
  program->instructions = addElement(program->instructions, instr);
  return;
}

void appendSetsiProg(int var1, long int val, struct implementCsteProgram *program) {
  struct implementCsteInstruction *instr;
  instr = safeMalloc(sizeof(struct implementCsteInstruction));
  instr->type = SETSI;
  constructName(instr->var1, var1);
  strcpy(instr->var2, "");
  strcpy(instr->var3, "");
  strcpy(instr->name, "");
  instr->sival = val;
  instr->strval = NULL;
  instr->prog1.instructions = NULL;
  instr->prog1.precisions = NULL;
  instr->prog2.instructions = NULL;
  instr->prog2.precisions = NULL;
  program->instructions = addElement(program->instructions, instr);
  return;
}

void appendSetstrProg(int var1, mpfr_t val, struct implementCsteProgram *program) {
  struct implementCsteInstruction *instr;
  instr = safeMalloc(sizeof(struct implementCsteInstruction));
  instr->type = SETSTR;
  constructName(instr->var1, var1);
  strcpy(instr->var2, "");
  strcpy(instr->var3, "");
  strcpy(instr->name, "");
  instr->prog1.instructions = NULL;
  instr->prog1.precisions = NULL;
  instr->prog2.instructions = NULL;
  instr->prog2.precisions = NULL;
  instr->strval = mpfr_to_binary_str(val);
  program->instructions = addElement(program->instructions, instr);
  return;
}

void appendUipowui(int var1, unsigned long int val1, unsigned long int val2, struct implementCsteProgram *program) {
  struct implementCsteInstruction *instr;
  instr = safeMalloc(sizeof(struct implementCsteInstruction));
  instr->type = UIPOWUI;
  constructName(instr->var1, var1);
  strcpy(instr->var2, "");
  strcpy(instr->var3, "");
  strcpy(instr->name, "");
  instr->uival = val1;
  instr->uival2 = val2;
  instr->strval = NULL;
  instr->prog1.instructions = NULL;
  instr->prog1.precisions = NULL;
  instr->prog2.instructions = NULL;
  instr->prog2.precisions = NULL;
  program->instructions = addElement(program->instructions, instr);
  return;  
}

void appendPowuiProg(int var1, int var2, unsigned long int val, struct implementCsteProgram *program) {
  struct implementCsteInstruction *instr;
  instr = safeMalloc(sizeof(struct implementCsteInstruction));
  instr->type = POWUI;
  constructName(instr->var1, var1);
  constructName(instr->var2, var2);
  strcpy(instr->var3, "");
  strcpy(instr->name, "");
  instr->uival = val;
  instr->strval = NULL;
  instr->prog1.instructions = NULL;
  instr->prog1.precisions = NULL;
  instr->prog2.instructions = NULL;
  instr->prog2.precisions = NULL;
  program->instructions = addElement(program->instructions, instr);
  return;  
}

void appendRootProg(int var1, int var2, unsigned long int val, struct implementCsteProgram *program) {
  struct implementCsteInstruction *instr;
  instr = safeMalloc(sizeof(struct implementCsteInstruction));
  instr->type = ROOT;
  constructName(instr->var1, var1);
  constructName(instr->var2, var2);
  strcpy(instr->var3, "");
  strcpy(instr->name, "");
  instr->uival = val;
  instr->strval = NULL;
  instr->prog1.instructions = NULL;
  instr->prog1.precisions = NULL;
  instr->prog2.instructions = NULL;
  instr->prog2.precisions = NULL;
  program->instructions = addElement(program->instructions, instr);
  return;  
}

void appendLibraryConstantProg(node *c, int gamma0, struct implementCsteProgram *program) {
  struct implementCsteInstruction *instr;
  instr = safeMalloc(sizeof(struct implementCsteInstruction));
  instr->type = LIBRARYCONST;
  constructName(instr->var1, program->counter);
  instr->prec = gamma0;
  strcpy(instr->var2, "");
  strcpy(instr->var3, "");
  strcpy(instr->name, "");
  instr->strval = safeCalloc(strlen(c->libFun->functionName)+1, sizeof(char));
  strcpy(instr->strval, c->libFun->functionName);
  instr->prog1.instructions = NULL;
  instr->prog1.precisions = NULL;
  instr->prog2.instructions = NULL;
  instr->prog2.precisions = NULL;

  program->instructions = addElement(program->instructions, instr);
  return;  
}

void appendIfThenElseProg(char *cond, struct implementCsteProgram prog1, struct implementCsteProgram prog2, struct implementCsteProgram *program) {
  struct implementCsteInstruction *instr;
  instr = safeMalloc(sizeof(struct implementCsteInstruction));
  instr->type = IFTHENELSE;
  strcpy(instr->var1, "");
  strcpy(instr->var2, "");
  strcpy(instr->var3, "");
  strcpy(instr->name, "");
  instr->strval = NULL;

  instr->prog1.counter = -1;
  instr->prog1.maxcounter = -1;
  instr->prog1.precisions = NULL;
  instr->prog1.instructions = copyChain(prog1.instructions, copy_implementCsteInstructions); /* Note: it reverses the list, but it is what we want since the programs are created with instructions in reversed order */
  instr->prog2.counter = -1;
  instr->prog2.maxcounter = -1;
  instr->prog2.precisions = NULL;
  instr->prog2.instructions = copyChain(prog2.instructions, copy_implementCsteInstructions); /* Note: it reverses the list, but it is what we want since the programs are created with instructions in reversed order */
  instr->strval = safeCalloc(strlen(cond)+1, sizeof(char));
  strcpy(instr->strval, cond);

  program->counter = (prog1.counter > prog2.counter)?prog1.counter:prog2.counter;
  program->maxcounter = (prog1.maxcounter > prog2.maxcounter)?prog1.maxcounter:prog2.maxcounter; 
  if(prog1.precisions != prog2.precisions) {
    sollyaFprintf(stderr, "Unexpected error: in an if-then-else statement, both branches must share the same pointer of precisions\n");
    exit(1);
  }
  program->precisions = copyChainWithoutReversal(prog2.precisions, copyCouple);

  program->instructions = addElement(program->instructions, instr);
  return;
}

/* Prototype of the recursive function */
/* Returns 0 in case of success and non-zero otherwise */
int constantImplementer(node *c, int gamma0, struct implementCsteProgram *program);

/* Main function */
/* Returns 0 in case of success and non-zero otherwise */
int implementconst(node *c, FILE *fd, char *name) {
  int i, test, res;
  FILE *output = fd;
  struct implementCsteProgram program;
  chain *curr;

  program.instructions = NULL;
  program.counter = 0;
  program.maxcounter = 0;
  program.precisions = NULL;

  res = constantImplementer(c, 0, &program);
  if (res) {   /* Something went wrong */
    freeChain(program.instructions, free_implementCsteInstruction);
    freeChain(program.precisions, free);
    return res;
  }

  /* reverse the chain */
  curr = copyChain(program.instructions, copy_implementCsteInstructions);
  freeChain(program.instructions, free_implementCsteInstruction);
  program.instructions = curr;

  curr = program.precisions;
  while(curr != NULL) {
    if( ((couple *)(curr->value))->var != 0 )
      appendInit2Prog( ((couple *)(curr->value))->var, ((couple *)(curr->value))->prec, &program);
    curr = curr->next;
  }

  sollyaFprintf(output, "#include <mpfr.h>\n\n");
  sollyaFprintf(output, "void\n");
  sollyaFprintf(output, "%s (mpfr_ptr y, mp_prec_t prec)\n", name);
  sollyaFprintf(output, "{\n");
  if(program.maxcounter>=2) sollyaFprintf(output, "  /* Declarations */\n");
  for(i=1; i<=program.maxcounter-1; i++) sollyaFprintf(output, "  mpfr_t tmp%d;\n", i);
  if(program.maxcounter>=2) sollyaFprintf(output, "\n");
  sollyaFprintf(output, "  /* Initializations */\n");

  test = 1;
  curr=program.instructions;
  while(curr!=NULL) {
    if (test 
        && ( ((struct implementCsteInstruction *)(curr->value))->type != INIT2 )
        ) {
      sollyaFprintf(output, "\n");
      sollyaFprintf(output, "  /* Core */\n");
      test = 0;
    }
    fprintInstruction(output, *(struct implementCsteInstruction *)(curr->value), 1);
    curr = curr->next;
  }

  if(program.maxcounter>=2) {
    sollyaFprintf(output, "\n");
    sollyaFprintf(output, "  /* Cleaning stuff */\n");
  }
  for(i=1; i<=program.maxcounter-1; i++) sollyaFprintf(output, "  mpfr_clear(tmp%d);\n", i);
  sollyaFprintf(output, "}\n");

  freeChain(program.instructions, free_implementCsteInstruction);
  freeChain(program.precisions, free);
  return 0;
}

int ceil_log2n(int p) {
  int n,log2p, test;
  n = p;
  log2p = 0;
  test = 1;
  /* Compute log2p such that 2^(log2p-1) <= p < 2^(log2p) */
  while (n>=1) { log2p++; if ((n%2)!=0) test=0; n = n/2;} 
  /* Adjust log2p in order to have 2^(log2p-1) < n <= 2^(log2p) */
  if(test) log2p--;

  return log2p;
}

/* Returns the maximal exponent of a number among those contained in x */
mp_exp_t sollya_mpfi_max_exp(sollya_mpfi_t x) {
  mpfr_t u,v;
  mp_exp_t Eu, Ev, E;
  mp_prec_t prec;

  prec = sollya_mpfi_get_prec(x);
  mpfr_init2(u, prec);
  mpfr_init2(v, prec);

  sollya_mpfi_get_left(u, x);
  sollya_mpfi_get_right(v, x);
  
  if (mpfr_zero_p(u)) E = mpfr_get_exp(v);
  else {
    if (mpfr_zero_p(v)) E = mpfr_get_exp(u);
    else {
      Eu = mpfr_get_exp(u);
      Ev = mpfr_get_exp(v);
      E = (Eu<=Ev) ? Ev : Eu;
    }
  }
  mpfr_clear(u);
  mpfr_clear(v);
  return E;
}

/* Return the smallest exponent among the exponents of the numbers contained
   in x. If 0 \in x, it returns NULL, else it returns a valid pointer E such
   that *E is the minimal exponent. */
mp_exp_t *sollya_mpfi_min_exp(sollya_mpfi_t x) {
  mpfr_t u,v;
  mp_exp_t Eu, Ev;
  mp_exp_t *E = NULL;
  mp_prec_t prec;

  prec = sollya_mpfi_get_prec(x);
  mpfr_init2(u, prec);
  mpfr_init2(v, prec);

  sollya_mpfi_get_left(u, x);
  sollya_mpfi_get_right(v, x);
  
  if (mpfr_sgn(u)*mpfr_sgn(v)>0) {
    E = safeMalloc(sizeof(mp_exp_t));
    Eu = mpfr_get_exp(u);
    Ev = mpfr_get_exp(v);
    *E = (Eu<=Ev) ? Eu : Ev;
  }

  mpfr_clear(u);
  mpfr_clear(v);
  return E;
}
 
/* Let a be the constant given by the expression cste and f the function with */
/* node type nodeType. This functions generates code for the implementation   */
/* of f(a) in precision prec+gamma0, the result being stored in resName.      */
int unaryFunctionCase(int nodeType, node *cste, char *functionName, int gamma0, struct implementCsteProgram *program) {
  sollya_mpfi_t a, b, u, v, tmp;
  mpfr_t alpha, beta;
  mp_prec_t prec = getToolPrecision();
  node *func, *deriv;
  int gamma;
  int counter;
  int res;

  sollya_mpfi_init2(a, prec);
  sollya_mpfi_init2(b, prec);
  sollya_mpfi_init2(u, prec);
  sollya_mpfi_init2(v, prec);
  sollya_mpfi_init2(tmp, prec);
  mpfr_init2(alpha, prec);
  mpfr_init2(beta, prec);

  func = makeUnary(makeVariable(), nodeType);
  deriv = differentiate(func);
  
  evaluateInterval(a, cste, NULL, a);
  evaluateInterval(b, func, deriv, a);
  if (sollya_mpfi_has_zero(b)) {
    sollya_mpfi_clear(a); sollya_mpfi_clear(b);
    sollya_mpfi_clear(u); sollya_mpfi_clear(v);
    sollya_mpfi_clear(tmp);
    mpfr_clear(alpha); mpfr_clear(beta);
    free_memory(func); free_memory(deriv);

    changeToWarningMode();
    sollyaPrintf("Error in implementconstant: the following expression seems to be exactly zero:\n");
    func = makeUnary(copyTree(cste), nodeType);
    printTree(func);
    free_memory(func);
    sollyaPrintf("\nIf it is not exactly zero, increasing prec should solve the issue.\nAbort.\n");
    restoreMode();
    return 2;
  }

  sollya_mpfi_div(u, a, b);
  evaluateInterval(tmp, deriv, NULL, a); 
  sollya_mpfi_mul(v, u, tmp);

  gamma = 2+sollya_mpfi_max_exp(v)-1;
  do {
    gamma++;
    mpfr_set_ui(beta, 1, GMP_RNDU);
    mpfr_div_2si(beta, beta, gamma+gamma0, GMP_RNDU);
    mpfr_ui_sub(alpha, 1, beta, GMP_RNDD);
    mpfr_add_ui(beta, beta, 1, GMP_RNDU);
    sollya_mpfi_interv_fr(tmp, alpha, beta);

    sollya_mpfi_mul(tmp, a, tmp);
    evaluateInterval(tmp, deriv, NULL, tmp);
    sollya_mpfi_mul(v, u, tmp);
  } while (gamma < 2+sollya_mpfi_max_exp(v));
  
  counter = program->counter;
  incrementProgramCounter(program);
  res = constantImplementer(cste, gamma0+gamma, program);
  program->counter = counter;
  appendSetprecProg(counter, gamma0+2, program);
  appendUnaryfuncProg(functionName, counter, counter+1, program);

  sollya_mpfi_clear(a);
  sollya_mpfi_clear(b);
  sollya_mpfi_clear(u);
  sollya_mpfi_clear(v);
  sollya_mpfi_clear(tmp);
  mpfr_clear(alpha);
  mpfr_clear(beta);
  free_memory(func);
  free_memory(deriv);
  return res;
}

void normalizeDivMul(node *c, chain **numerator, chain **denominator) {
  chain *num1 = NULL;
  chain *denom1 = NULL;
  chain *num2 = NULL;
  chain *denom2 = NULL;

  if (c->nodeType == MUL) {
    normalizeDivMul(c->child1, &num1, &denom1);
    normalizeDivMul(c->child2, &num2, &denom2);
    *numerator = concatChains(num1, num2);
    *denominator = concatChains(denom1, denom2);
  }
  else if (c->nodeType == DIV) {
    normalizeDivMul(c->child1, &num1, &denom1);
    normalizeDivMul(c->child2, &denom2, &num2);
    *numerator = concatChains(num1, num2);
    *denominator = concatChains(denom1, denom2);
  }
  else *numerator = addElement(*numerator, copyTree(c));
}

int implementDivMul(node *c, int gamma0, struct implementCsteProgram *program) {
  chain *numerator = NULL;
  chain *denominator = NULL;
  chain *curr;
  chain *bufferNum, *bufferDenom;
  int log2n, n;
  int *tmp;
  int counter;
  int res = 0;

  normalizeDivMul(c, &numerator, &denominator);

  n = lengthChain(numerator) + lengthChain(denominator);
  log2n = ceil_log2n(n);
  counter = program->counter;
  incrementProgramCounter(program);

  curr = numerator;
  bufferNum = NULL;
  while ( (curr!=NULL) && (!res) ) {
    tmp = safeMalloc(sizeof(int));
    *tmp = program->counter;
    bufferNum = addElement(bufferNum, tmp);
    res = constantImplementer(curr->value, gamma0+2+log2n, program);
    curr = curr->next;
  }
  if (res) { /* Something went wrong */
    freeChain(bufferNum, freeIntPtr);
    freeChain(numerator, (void (*)(void *))free_memory);
    freeChain(denominator, (void (*)(void *))free_memory);  
    return res;
  }

  curr = denominator;
  bufferDenom = NULL;
  while ( (curr!=NULL) && (!res) ) {
    tmp = safeMalloc(sizeof(int));
    *tmp = program->counter;
    bufferDenom = addElement(bufferDenom, tmp);
    res = constantImplementer(curr->value, gamma0+2+log2n, program);
    curr = curr->next;
  }
  if (res) { /* Something went wrong */
    freeChain(bufferNum, freeIntPtr);
    freeChain(bufferDenom, freeIntPtr);
    freeChain(numerator, (void (*)(void *))free_memory);
    freeChain(denominator, (void (*)(void *))free_memory);  
    return res;
  }


  program->counter = counter;
  if ( (lengthChain(numerator)==1) && (lengthChain(denominator)==1) ) {
    appendSetprecProg(counter, gamma0+2+log2n, program);
    appendBinaryfuncProg("mpfr_div", counter, *((int *)(bufferNum->value)), *((int *)(bufferDenom->value)), program);
  }
  else if (lengthChain(numerator)==1) {
    appendSetprecProg(counter, gamma0+2+log2n, program);
    appendBinaryfuncProg("mpfr_mul", counter, *((int *)(bufferDenom->value)), *((int *)(bufferDenom->next->value)), program);
    curr = bufferDenom->next->next;
    while(curr!=NULL) {
      appendBinaryfuncProg("mpfr_mul", counter, counter, *((int *)(curr->value)), program);
      curr = curr->next;
    }
    appendBinaryfuncProg("mpfr_div", counter, *((int *)(bufferNum->value)), counter, program);
  }
  else if (lengthChain(denominator)<=1) {
    appendSetprecProg(counter, gamma0+2+log2n, program);
    appendBinaryfuncProg("mpfr_mul", counter, *((int *)(bufferNum->value)), *((int *)(bufferNum->next->value)), program);
    curr = bufferNum->next->next;
    while(curr!=NULL) {
      appendBinaryfuncProg("mpfr_mul", counter, counter, *((int *)(curr->value)), program);
      curr = curr->next;
    }
    if (lengthChain(denominator)==1)
      appendBinaryfuncProg("mpfr_div", counter, counter, *((int *)(bufferDenom->value)), program);
  }
  else {
    incrementProgramCounter(program);
    appendSetprecProg(counter, gamma0+2+log2n, program);
    appendBinaryfuncProg("mpfr_mul", counter, *((int *)(bufferNum->value)), *((int *)(bufferNum->next->value)), program);
    curr = bufferNum->next->next;
    while(curr!=NULL) {
      appendBinaryfuncProg("mpfr_mul", counter, counter, *((int *)(curr->value)), program);
      curr = curr->next;
    }
    appendSetprecProg(program->counter, gamma0+2+log2n, program);
    appendBinaryfuncProg("mpfr_mul", program->counter,  *((int *)(bufferDenom->value)), *((int *)(bufferDenom->next->value)), program);
    curr = bufferDenom->next->next;
    while(curr!=NULL) {
      appendBinaryfuncProg("mpfr_mul", program->counter, program->counter, *((int *)(curr->value)), program);
      curr = curr->next;
    }
    appendBinaryfuncProg("mpfr_div", counter, counter, program->counter, program);
  }
 
  program->counter = counter;
  freeChain(bufferNum, freeIntPtr);
  freeChain(bufferDenom, freeIntPtr);
  freeChain(numerator, (void (*)(void *))free_memory);
  freeChain(denominator, (void (*)(void *))free_memory);  
  return res;
}

int summation_weight(node *c) {
  if ( (c->nodeType == ADD) || (c->nodeType == SUB) )
    return (summation_weight(c->child1) + summation_weight(c->child2) + 1);
  else return 1;
}

int implementAddSub(node *c, int gamma0, struct implementCsteProgram *program) {
  sollya_mpfi_t y, a, b, tmp, tmp2;
  mp_exp_t *Ea, *Eb, *Ey;
  int na, nb, n;
  int tmpa, tmpb;
  mp_prec_t prec;
  int counter;
  int res;
  struct implementCsteProgram prog1, prog2;
  char *str;

  prec = getToolPrecision();
  sollya_mpfi_init2(y, prec);
  sollya_mpfi_init2(a, prec);
  sollya_mpfi_init2(b, prec);
  sollya_mpfi_init2(tmp, prec);
  sollya_mpfi_init2(tmp2, prec);

  
  evaluateInterval(y, c, NULL, y);
  if (sollya_mpfi_has_zero(y)) {
    sollya_mpfi_clear(y); sollya_mpfi_clear(a); sollya_mpfi_clear(b);
    sollya_mpfi_clear(tmp); sollya_mpfi_clear(tmp2);

    changeToWarningMode();
    sollyaPrintf("Error in implementconstant: the following expression seems to be exactly zero:\n");
    printTree(c);
    sollyaPrintf("\nIf it is not exactly zero, increasing prec should solve the issue.\nAbort.\n");
    restoreMode();
    return 2;
  }
  evaluateInterval(a, c->child1, NULL, a);
  evaluateInterval(b, c->child2, NULL, b);

  na = summation_weight(c->child1);
  nb = summation_weight(c->child2);
  n = na+nb+1;

  sollya_mpfi_div(tmp, y, a); sollya_mpfi_mul_ui(tmp, tmp, na); sollya_mpfi_div_ui(tmp, tmp, n);
  Ea = sollya_mpfi_min_exp(tmp);
  if (Ea==NULL) {
    sollyaFprintf(stderr, "Unexpected error. Aborting\n");
    exit(1);
  }

  sollya_mpfi_div(tmp, y, b); sollya_mpfi_mul_ui(tmp, tmp, nb); sollya_mpfi_div_ui(tmp, tmp, n);
  Eb = sollya_mpfi_min_exp(tmp);
  if (Eb==NULL) {
    sollyaFprintf(stderr, "Unexpected error. Aborting\n");
    exit(1);
  }

  sollya_mpfi_abs(tmp, a);
  sollya_mpfi_abs(tmp2, b);
  sollya_mpfi_add(tmp, tmp, tmp2);
  sollya_mpfi_div(tmp, y, tmp);
  sollya_mpfi_div_ui(tmp, tmp, n);
  Ey = sollya_mpfi_min_exp(tmp);
  if (Ey==NULL) {
    sollyaFprintf(stderr, "Unexpected error. Aborting\n");
    exit(1);
  }

  counter = program->counter;
  incrementProgramCounter(program);

  if( gamma0+1-*Ea>=0 ) { /* No need to perform a test inside 
                             the generated code */
    tmpa = program->counter;
    res = constantImplementer(c->child1, gamma0+1-*Ea, program);
  }
  else {
    prog1.instructions = NULL;
    prog1.counter = program->counter;
    prog1.maxcounter = program->maxcounter;
    prog1.precisions = program->precisions;
    
    prog2.instructions = NULL;
    prog2.counter = program->counter;
    prog2.maxcounter = program->maxcounter;
    
    tmpa = program->counter;
    appendSetuiProg(tmpa, 0, &prog1);
    incrementProgramCounter(&prog1);
    
    prog2.precisions = prog1.precisions;
    res = constantImplementer(c->child1, gamma0+1-*Ea, &prog2);
    prog1.precisions = prog2.precisions;
    
    str = safeCalloc(32 , sizeof(char));
    sprintf(str, "prec <= %d", (int)(*Ea-gamma0));
    appendIfThenElseProg(str, prog1, prog2, program);
    free(str);

    /* No need to free progi.precisions: program.precisions now on it. */
    freeChain(prog1.instructions, free_implementCsteInstruction);
    freeChain(prog2.instructions, free_implementCsteInstruction);
  }
  if (res) { /* Something went wrong */
    free(Ea); free(Eb); free(Ey);
    sollya_mpfi_clear(y); sollya_mpfi_clear(a);
    sollya_mpfi_clear(b); sollya_mpfi_clear(tmp);
    sollya_mpfi_clear(tmp2);
    return res;
  }

  if( gamma0+1-*Eb>=0 ) { /* No need to perform a test inside 
                             the generated code */
    tmpb = program->counter;
    res = constantImplementer(c->child2, gamma0+1-*Eb, program);
  }
  else {
    prog1.instructions = NULL;
    prog1.counter = program->counter;
    prog1.maxcounter = program->maxcounter;
    prog1.precisions = program->precisions;
 
    prog2.instructions = NULL;
    prog2.counter = program->counter;
    prog2.maxcounter = program->maxcounter;

    tmpb = program->counter;
    appendSetuiProg(tmpb, 0, &prog1);
    incrementProgramCounter(&prog1);

    prog2.precisions = prog1.precisions;
    res = constantImplementer(c->child2, gamma0+1-*Eb, &prog2);
    prog1.precisions = prog2.precisions;

    str = safeCalloc(32 , sizeof(char));
    sprintf(str, "prec <= %d", (int)(*Eb-gamma0));
    appendIfThenElseProg(str, prog1, prog2, program);
    free(str);
    
    /* No need to free progi.precisions: program.precisions now on it. */
    freeChain(prog1.instructions, free_implementCsteInstruction);
    freeChain(prog2.instructions, free_implementCsteInstruction);
  }
  if (res) { /* Something went wrong */
    free(Ea); free(Eb); free(Ey);
    sollya_mpfi_clear(y); sollya_mpfi_clear(a);
    sollya_mpfi_clear(b); sollya_mpfi_clear(tmp);
    sollya_mpfi_clear(tmp2);
    return res;
  }

  appendSetprecProg(counter, gamma0+2-*Ey, program);
  if (c->nodeType==ADD)
    appendBinaryfuncProg("mpfr_add", counter, tmpa, tmpb, program);
  else if (c->nodeType==SUB)
    appendBinaryfuncProg("mpfr_sub", counter, tmpa, tmpb, program);
  else {
    sollyaFprintf(stderr, "Unexpected error: an addition/subtraction must have nodeType=ADD or nodeType=SUB\n");
    exit(1);
  }

  program->counter = counter;
  free(Ea);
  free(Eb);
  free(Ey);
  sollya_mpfi_clear(y);
  sollya_mpfi_clear(a);
  sollya_mpfi_clear(b);
  sollya_mpfi_clear(tmp);
  sollya_mpfi_clear(tmp2);
  return res;
}

int implementPow(node *c, int gamma0, struct implementCsteProgram *program) {
  int log2p, p, counter;
  node *tmpNode;
  mpfr_t tmp;
  int res;

  counter = program->counter;
  if ( (c->child1->nodeType==CONSTANT) 
       && mpfr_integer_p(*(c->child1->value))
       && mpfr_fits_ulong_p(*(c->child1->value), GMP_RNDN)
       && (c->child2->nodeType==CONSTANT) 
       && mpfr_integer_p(*(c->child2->value))
       && mpfr_fits_ulong_p(*(c->child2->value), GMP_RNDN)) { /* Case n^p */
    appendSetprecProg(counter, gamma0, program);
    appendUipowui(counter, mpfr_get_ui(*(c->child1->value), GMP_RNDN), mpfr_get_ui(*(c->child2->value), GMP_RNDN), program);
    program->counter = counter;
    return 0;
  }

  if ( (c->child2->nodeType==CONSTANT) 
       && mpfr_integer_p(*(c->child2->value))
       && mpfr_fits_ulong_p(*(c->child2->value), GMP_RNDN) ) { /* Case x^p */
    p = mpfr_get_ui(*(c->child2->value), GMP_RNDN);
    log2p = ceil_log2n(p);
    incrementProgramCounter(program);
    res = constantImplementer(c->child1, gamma0+log2p+3, program);
    appendSetprecProg(counter, gamma0+2, program);
    appendPowuiProg(counter, counter+1, p, program);
    program->counter = counter;
    return res;
  }

  if ( (c->child2->nodeType==DIV)
       && (c->child2->child1->nodeType==CONSTANT)
       && (mpfr_cmp_ui(*(c->child2->child1->value), 1)==0)
       && (c->child2->child2->nodeType==CONSTANT)
       && mpfr_integer_p(*(c->child2->child2->value))
       && mpfr_fits_ulong_p(*(c->child2->child2->value), GMP_RNDN)
       ) { /* Case x^(1/p) (note that this does not handle the case when p=2^k */
    p = mpfr_get_ui(*(c->child2->child2->value), GMP_RNDN);
    log2p = ceil_log2n(p);
    incrementProgramCounter(program);
    res = constantImplementer(c->child1, gamma0-log2p+3, program);
    appendSetprecProg(counter, gamma0+2, program);
    appendRootProg(counter, counter+1, p, program);
    program->counter = counter;
    return res; 
  }

  if (c->child2->nodeType==CONSTANT) {
    mpfr_init2(tmp, 64);
    if ( (mpfr_ui_div(tmp, 1, *(c->child2->value), GMP_RNDN) == 0)
         && mpfr_integer_p(tmp)
         && mpfr_fits_ulong_p(tmp, GMP_RNDN)
         ) { /* Case x^(1/p) where p is a power of 2 */
      p = mpfr_get_ui(tmp, GMP_RNDN);
      log2p = ceil_log2n(p);
      incrementProgramCounter(program);
      res = constantImplementer(c->child1, gamma0-log2p+3, program);
      appendSetprecProg(counter, gamma0+2, program);
      appendRootProg(counter, counter+1, p, program);
      program->counter = counter;
      mpfr_clear(tmp);
      return res; 
    }
    mpfr_clear(tmp);
  }

  /* else... case x^y with x possibly integer. Handled as exp(y*ln(x)) */
  tmpNode = makeExp(makeMul(copyTree(c->child2), makeLog(copyTree(c->child1))));
  res = constantImplementer(tmpNode, gamma0, program);
  free_memory(tmpNode);
  program->counter = counter;
  return res;
}

int implementCsteCase(node *c, int gamma0, struct implementCsteProgram *program) {
  appendSetprecProg(program->counter, gamma0, program);
  if (mpfr_integer_p(*(c->value)) && mpfr_fits_ulong_p(*(c->value), GMP_RNDN)) {
    appendSetuiProg(program->counter, mpfr_get_ui(*(c->value), GMP_RNDN), program);
  }
  else if (mpfr_integer_p(*(c->value)) && mpfr_fits_slong_p(*(c->value), GMP_RNDN)) {
    appendSetsiProg(program->counter, mpfr_get_si(*(c->value), GMP_RNDN), program);
  }
  else {
    appendSetstrProg(program->counter, *(c->value), program);
  }
  return 0;
}

int constantImplementer(node *c, int gamma0, struct implementCsteProgram *program) {
  int res;

  switch (c->nodeType) {
  case ADD:
  case SUB:
    res = implementAddSub(c, gamma0, program);
    break;
  case MUL:
  case DIV:
    res = implementDivMul(c, gamma0, program);
    break;
  case POW:
    res = implementPow(c, gamma0, program);
    break;
  case CONSTANT:
    res = implementCsteCase(c, gamma0, program);
    break;
  case NEG:
    res = constantImplementer(c->child1, gamma0, program);
    appendUnaryfuncProg("mpfr_neg", program->counter, program->counter, program);
    break;
  case ABS:
    res = constantImplementer(c->child1, gamma0, program);
    appendUnaryfuncProg("mpfr_abs", program->counter, program->counter, program);
    break;
  case DOUBLE:
    changeToWarningMode();
    sollyaPrintf("implementconstant: error: the double function is not supported by this command.\nNo code will be produced.\n");
    restoreMode();
    res = 1;
    break;
  case DOUBLEDOUBLE:
    changeToWarningMode();
    sollyaPrintf("implementconstant: error: the doubledouble function is not supported by this command.\nNo code will be produced.\n");
    restoreMode();
    res = 1;
    break;
  case TRIPLEDOUBLE:
    changeToWarningMode();
    sollyaPrintf("implementconstant: error: the tripledouble function is not supported by this command.\nNo code will be produced.\n");
    restoreMode();
    res = 1;
    break;
  case DOUBLEEXTENDED:
    changeToWarningMode();
    sollyaPrintf("implementconstant: error: the doubleextended function is not supported by this command.\nNo code will be produced.\n");
    restoreMode();
    res = 1;
    break;
  case SINGLE:
    changeToWarningMode();
    sollyaPrintf("implementconstant: error: the single function is not supported by this command.\nNo code will be produced.\n");
    restoreMode();
    res = 1;
    break;
  case HALFPRECISION:
    changeToWarningMode();
    sollyaPrintf("implementconstant: error: the half-precision function is not supported by this command.\nNo code will be produced.\n");
    restoreMode();
    res = 1;
    break;
  case QUAD:
    changeToWarningMode();
    sollyaPrintf("implementconstant: error: the quad function is not supported by this command.\nNo code will be produced.\n");
    restoreMode();
    res = 1;
    break;
  case NEARESTINT:
    changeToWarningMode();
    sollyaPrintf("implementconstant: error: the nearestint function is not supported by this command.\nNo code will be produced.\n");
    restoreMode();
    res = 1;
    break;
  case CEIL:
    changeToWarningMode();
    sollyaPrintf("implementconstant: error: the ceil function is not supported by this command.\nNo code will be produced.\n");
    restoreMode();
    res = 1;
    break;
  case FLOOR:
    changeToWarningMode();
    sollyaPrintf("implementconstant: error: the floor function is not supported by this command.\nNo code will be produced.\n");
    restoreMode();
    res = 1;
    break;
  case PI_CONST:
    appendSetprecProg(program->counter, gamma0, program);
    appendConstantfuncProg("mpfr_const_pi", program->counter, program);
    res = 0;
    break;
  case LIBRARYCONSTANT:
    appendPrecisionProg(program->counter, gamma0, program);
    appendLibraryConstantProg(c, gamma0, program);
    res = 0;
    break;

  case SQRT:
    res = unaryFunctionCase(c->nodeType, c->child1, "mpfr_sqrt", gamma0, program);
    break;
  case EXP:
    res = unaryFunctionCase(c->nodeType, c->child1, "mpfr_exp", gamma0, program);
    break;
  case LOG:
    res = unaryFunctionCase(c->nodeType, c->child1, "mpfr_log", gamma0, program);
    break;
  case LOG_2:
    res = unaryFunctionCase(c->nodeType, c->child1, "mpfr_log2", gamma0, program);
    break;
  case LOG_10:
    res = unaryFunctionCase(c->nodeType, c->child1, "mpfr_log10", gamma0, program);
    break;
  case SIN:
    res = unaryFunctionCase(c->nodeType, c->child1, "mpfr_sin", gamma0, program);
    break;
  case COS:
    res = unaryFunctionCase(c->nodeType, c->child1, "mpfr_cos", gamma0, program);
    break;
  case TAN:
    res = unaryFunctionCase(c->nodeType, c->child1, "mpfr_tan", gamma0, program);
    break;
  case ASIN:
    res = unaryFunctionCase(c->nodeType, c->child1, "mpfr_asin", gamma0, program);
    break;
  case ACOS:
    res = unaryFunctionCase(c->nodeType, c->child1, "mpfr_acos", gamma0, program);
    break;
  case ATAN:
    res = unaryFunctionCase(c->nodeType, c->child1, "mpfr_atan", gamma0, program);
    break;
  case SINH:
    res = unaryFunctionCase(c->nodeType, c->child1, "mpfr_sinh", gamma0, program);
    break;
  case COSH:
    res = unaryFunctionCase(c->nodeType, c->child1, "mpfr_cosh", gamma0, program);
    break;
  case TANH:
    res = unaryFunctionCase(c->nodeType, c->child1, "mpfr_tanh", gamma0, program);
    break;
  case ASINH:
    res = unaryFunctionCase(c->nodeType, c->child1, "mpfr_asinh", gamma0, program);
    break;
  case ACOSH:
    res = unaryFunctionCase(c->nodeType, c->child1, "mpfr_acosh", gamma0, program);
    break;
  case ATANH:
    res = unaryFunctionCase(c->nodeType, c->child1, "mpfr_atanh", gamma0, program);
    break;
  case ERF:
    res = unaryFunctionCase(c->nodeType, c->child1, "mpfr_erf", gamma0, program);
    break;
  case ERFC:
    res = unaryFunctionCase(c->nodeType, c->child1, "mpfr_erfc", gamma0, program);
    break;
  case LOG_1P:
    res = unaryFunctionCase(c->nodeType, c->child1, "mpfr_log1p", gamma0, program);
    break;
  case EXP_M1:
    res = unaryFunctionCase(c->nodeType, c->child1, "mpfr_expm1", gamma0, program);
    break;
  case LIBRARYFUNCTION:
    changeToWarningMode();
    sollyaPrintf("implementconstant: error: library functions are not supported by this command.\nNo code will be produced.\n");
    restoreMode();
    res = 1;
    break;
  case PROCEDUREFUNCTION:
    changeToWarningMode();
    sollyaPrintf("implementconstant: error: procedure functions are not supported by this command.\nNo code will be produced.\n");
    restoreMode();
    res = 1;
    break;

  default:
    printMessage(1, "Unknown identifier (%d) in the tree\n", c->nodeType);
    res = 1;
  }

  incrementProgramCounter(program);
  return res;
}
