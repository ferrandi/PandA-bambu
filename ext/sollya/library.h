/*

Copyright 2007-2011 by 

Laboratoire de l'Informatique du Parallelisme, 
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668,

LORIA (CNRS, INPL, INRIA, UHP, U-Nancy 2)

and by

Laboratoire d'Informatique de Paris 6, equipe PEQUAN,
UPMC Universite Paris 06 - CNRS - UMR 7606 - LIP6, Paris, France.

Contributor Ch. Lauter

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

#ifndef LIBRARY_H
#define LIBRARY_H

#include <mpfr.h>
#include "mpfi-compat.h"
#include <dlfcn.h>
#include "expression.h"
#include "chain.h"

typedef struct libraryHandleStruct libraryHandle;
struct libraryHandleStruct 
{
  char *libraryName;
  void *libraryDescriptor;
  chain *functionList;
};

typedef struct libraryFunctionStruct libraryFunction;
struct libraryFunctionStruct 
{
  char *functionName;
  int (*code)(sollya_mpfi_t, sollya_mpfi_t, int); /* used for LIBRARYFUNCTION */
  void (*constant_code)(mpfr_t, mp_prec_t); /* used for LIBRARYCONSTANT */
};

typedef struct libraryProcedureStruct libraryProcedure;
struct libraryProcedureStruct 
{
  char *procedureName;
  void *code;
  chain *signature;
};


#define VOID_TYPE 0
#define CONSTANT_TYPE 1
#define FUNCTION_TYPE 2
#define RANGE_TYPE 3
#define INTEGER_TYPE 4
#define STRING_TYPE 5
#define BOOLEAN_TYPE 6
#define CONSTANT_LIST_TYPE 7
#define FUNCTION_LIST_TYPE 8
#define RANGE_LIST_TYPE 9
#define INTEGER_LIST_TYPE 10
#define STRING_LIST_TYPE 11
#define BOOLEAN_LIST_TYPE 12


libraryFunction *bindFunction(char* libraryName, char *functionName);
libraryFunction *bindConstantFunction(char* libraryName, char *functionName);
libraryProcedure *bindProcedure(char* libraryName, char *procedureName, chain *signature);
libraryFunction *getFunction(char *functionName);
libraryFunction *getConstantFunction(char *functionName);
libraryProcedure *getProcedure(char *procedureName);
void freeFunctionLibraries();
void freeConstantLibraries();
void freeProcLibraries();

#endif /* ifdef LIBRARY_H*/
