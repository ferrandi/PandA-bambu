/*

Copyright 2007-2011 by 

Laboratoire de l'Informatique du Parallelisme, 
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668,

LORIA (CNRS, INPL, INRIA, UHP, U-Nancy 2)

and by

Laboratoire d'Informatique de Paris 6, equipe PEQUAN,
UPMC Universite Paris 06 - CNRS - UMR 7606 - LIP6, Paris, France.

Contributors Ch. Lauter, S. Chevillard

christoph.lauter@ens-lyon.org
sylvain.chevillard@ens-lyon.org

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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>
#include <mpfr.h>
#include "mpfi-compat.h"
#include "expression.h"
#include "general.h"
#include "library.h"
#include "chain.h"
#include "execute.h"

#ifndef RTLD_LOCAL
#define RTLD_LOCAL 0
#endif

#define LIBRARYFUNCTION_CASE 0
#define LIBRARYCONSTANT_CASE 1
#define LIBRARYPROC_CASE 2

chain *openedFunctionLibraries = NULL;
chain *openedConstantLibraries = NULL;
chain *openedProcLibraries = NULL;

libraryHandle *getLibraryHandle(char *libraryName, int type) {
  chain *curr;
  chain *openedLibraries;
  libraryHandle *currHandle;
  void *dlfcnHandle;

  openedLibraries = NULL;
  
  switch(type) {
  case LIBRARYFUNCTION_CASE: openedLibraries = openedFunctionLibraries; break;
  case LIBRARYCONSTANT_CASE: openedLibraries = openedConstantLibraries; break;
  case LIBRARYPROC_CASE: openedLibraries = openedProcLibraries; break;
  }

  curr = openedLibraries;
  while (curr != NULL) {
    currHandle = (libraryHandle *) curr->value;
    if (strcmp(currHandle->libraryName,libraryName) == 0) 
      return currHandle;
    curr = curr->next;
  }

  dlerror(); 
  dlfcnHandle = dlopen(libraryName, RTLD_LOCAL | RTLD_NOW);
  if (dlfcnHandle == NULL) 
    return NULL;

  currHandle = (libraryHandle *) safeMalloc(sizeof(libraryHandle));
  currHandle->libraryName = (char *) safeCalloc(strlen(libraryName)+1,sizeof(char));
  strcpy(currHandle->libraryName,libraryName);
  currHandle->libraryDescriptor = dlfcnHandle;
  currHandle->functionList = NULL;

  openedLibraries = addElement(openedLibraries,currHandle);

  switch(type) {
  case LIBRARYFUNCTION_CASE: openedFunctionLibraries = openedLibraries; break;
  case LIBRARYCONSTANT_CASE: openedConstantLibraries = openedLibraries; break;
  case LIBRARYPROC_CASE: openedProcLibraries = openedLibraries; break;
  }

  return currHandle;
}

libraryHandle *getLibraryFunctionHandle(char *libraryName) {
  return getLibraryHandle(libraryName, LIBRARYFUNCTION_CASE);
}

libraryHandle *getConstantLibraryHandle(char *libraryName) {
  return getLibraryHandle(libraryName, LIBRARYCONSTANT_CASE);
}

libraryHandle *getProcLibraryHandle(char *libraryName) {
  return getLibraryHandle(libraryName, LIBRARYPROC_CASE);
}


/* Functions related to library functions */
libraryFunction *bindFunction(char* libraryName, char *functionName) {
  libraryHandle *libHandle;
  libraryFunction *currFunct;
  int (*myFunction)(sollya_mpfi_t, sollya_mpfi_t, int);
  char *error;
  sollya_mpfi_t op, rop;

  currFunct = getFunction(functionName);
  if (currFunct != NULL) {
    printMessage(1,"Warning: a function named \"%s\" has already been bound.\n",functionName);
    return currFunct;
  }

  libHandle = getLibraryFunctionHandle(libraryName);
  if (libHandle == NULL) {
    changeToWarningMode();
    sollyaFprintf(stderr,"Error: could not open library \"%s\" for binding \"%s\": %s\n",libraryName,functionName,dlerror());
    restoreMode();
    return NULL;
  }
    
  dlerror();
  myFunction = (int (*)(sollya_mpfi_t, sollya_mpfi_t, int)) dlsym(libHandle->libraryDescriptor, functionName);
  if ((error = dlerror()) != NULL) {
    changeToWarningMode();
    sollyaFprintf(stderr, "Error: could not find function \"%s\" in library \"%s\" for binding: %s\n",functionName,libraryName,error);
    restoreMode();
    return NULL;
  }
  
  sollya_mpfi_init2(rop,12);
  sollya_mpfi_init2(op,12);
  sollya_mpfi_set_d(op,1.0);

  myFunction(rop,op,0);

  sollya_mpfi_clear(rop);
  sollya_mpfi_clear(op);

  currFunct = (libraryFunction *) safeMalloc(sizeof(libraryFunction));
  currFunct->functionName = (char *) safeCalloc(strlen(functionName)+1,sizeof(char));
  strcpy(currFunct->functionName,functionName);
  currFunct->code = myFunction;
  
  libHandle->functionList = addElement(libHandle->functionList,currFunct);

  return currFunct;
} 

libraryFunction *getFunction(char *functionName) {
  chain *currLibList, *currFunList;
  libraryFunction *currFunct;
  libraryHandle *currLibHandle;

  currLibList = openedFunctionLibraries;
  while (currLibList != NULL) {
    currLibHandle = (libraryHandle *) currLibList->value;
    currFunList = currLibHandle->functionList;
    while (currFunList != NULL) {
      currFunct = (libraryFunction *) currFunList->value;
      if (strcmp(currFunct->functionName,functionName) == 0)
        return currFunct;
      currFunList = currFunList->next;
    }
    currLibList = currLibList->next;
  }

  return NULL;
}

void freeFunctionLibraries() {
  chain *currLibList, *currFunList, *prevFunList, *prevLibList;
  libraryFunction *currFunct;
  libraryHandle *currLibHandle;

  currLibList = openedFunctionLibraries;
  while (currLibList != NULL) {
    currLibHandle = (libraryHandle *) currLibList->value;
    currFunList = currLibHandle->functionList;
    while (currFunList != NULL) {
      currFunct = (libraryFunction *) currFunList->value;
      free(currFunct->functionName);
      free(currFunList->value);
      prevFunList = currFunList;
      currFunList = currFunList->next;
      free(prevFunList);
    }
    dlerror();
    if (dlclose(currLibHandle->libraryDescriptor) != 0) 
      printMessage(1,"Warning: could not close libary \"%s\": %s\n",currLibHandle->libraryName,dlerror());
    free(currLibHandle->libraryName);
    free(currLibHandle);
    prevLibList = currLibList;
    currLibList = currLibList->next;
    free(prevLibList);
  }
  openedFunctionLibraries = NULL;
}



/* Functions related to library constants */
libraryFunction *bindConstantFunction(char* libraryName, char *functionName) {
  libraryHandle *libHandle;
  libraryFunction *currFunct;
  void (*myFunction)(mpfr_t, mp_prec_t);
  char *error;
  mpfr_t op;

  currFunct = getConstantFunction(functionName);
  if (currFunct != NULL) {
    printMessage(1,"Warning: a function named \"%s\" has already been bound.\n",functionName);
    return currFunct;
  }

  libHandle = getConstantLibraryHandle(libraryName);
  if (libHandle == NULL) {
    changeToWarningMode();
    sollyaFprintf(stderr,"Error: could not open library \"%s\" for binding \"%s\": %s\n",libraryName,functionName,dlerror());
    restoreMode();
    return NULL;
  }
    
  dlerror();
  myFunction = (void (*)(mpfr_t, mp_prec_t)) dlsym(libHandle->libraryDescriptor, functionName);
  if ((error = dlerror()) != NULL) {
    changeToWarningMode();
    sollyaFprintf(stderr, "Error: could not find function \"%s\" in library \"%s\" for binding: %s\n",functionName,libraryName,error);
    restoreMode();
    return NULL;
  }
  
  mpfr_init2(op,20);
  myFunction(op,5);

  mpfr_clear(op);

  currFunct = (libraryFunction *) safeMalloc(sizeof(libraryFunction));
  currFunct->functionName = (char *) safeCalloc(strlen(functionName)+1,sizeof(char));
  strcpy(currFunct->functionName,functionName);
  currFunct->constant_code = myFunction;
  
  libHandle->functionList = addElement(libHandle->functionList,currFunct);

  return currFunct;
} 

libraryFunction *getConstantFunction(char *functionName) { 
  chain *currLibList, *currFunList;
  libraryFunction *currFunct;
  libraryHandle *currLibHandle;

  currLibList = openedConstantLibraries;
  while (currLibList != NULL) {
    currLibHandle = (libraryHandle *) currLibList->value;
    currFunList = currLibHandle->functionList;
    while (currFunList != NULL) {
      currFunct = (libraryFunction *) currFunList->value;
      if (strcmp(currFunct->functionName,functionName) == 0)
        return currFunct;
      currFunList = currFunList->next;
    }
    currLibList = currLibList->next;
  }

  return NULL;
}

void freeConstantLibraries() {
  chain *currLibList, *currFunList, *prevFunList, *prevLibList;
  libraryFunction *currFunct;
  libraryHandle *currLibHandle;

  currLibList = openedConstantLibraries;
  while (currLibList != NULL) {
    currLibHandle = (libraryHandle *) currLibList->value;
    currFunList = currLibHandle->functionList;
    while (currFunList != NULL) {
      currFunct = (libraryFunction *) currFunList->value;
      free(currFunct->functionName);
      free(currFunList->value);
      prevFunList = currFunList;
      currFunList = currFunList->next;
      free(prevFunList);
    }
    dlerror();
    if (dlclose(currLibHandle->libraryDescriptor) != 0) 
      printMessage(1,"Warning: could not close libary \"%s\": %s\n",currLibHandle->libraryName,dlerror());
    free(currLibHandle->libraryName);
    free(currLibHandle);
    prevLibList = currLibList;
    currLibList = currLibList->next;
    free(prevLibList);
  }
  openedConstantLibraries = NULL;
}

/* Functions related to external procedures */
libraryProcedure *bindProcedure(char* libraryName, char *procedureName, chain *signature) {
  libraryHandle *libHandle;
  libraryProcedure *currProc;
  char *error;
  void *myFunction;

  currProc = getProcedure(procedureName);
  if (currProc != NULL) {
    printMessage(1,"Warning: a function named \"%s\" has already been bound.\n",procedureName);
    return currProc;
  }

  libHandle = getProcLibraryHandle(libraryName);
  if (libHandle == NULL) {
    changeToWarningMode();
    sollyaFprintf(stderr,"Error: could not open library \"%s\" for binding \"%s\": %s\n",libraryName,procedureName,dlerror());
    restoreMode();
    return NULL;
  }
    
  dlerror();
  myFunction = dlsym(libHandle->libraryDescriptor, procedureName);
  if ((error = dlerror()) != NULL) {
    changeToWarningMode();
    sollyaFprintf(stderr, "Error: could not find function \"%s\" in library \"%s\" for binding: %s\n",procedureName,libraryName,error);
    restoreMode();
    return NULL;
  }
  
  currProc = (libraryProcedure *) safeMalloc(sizeof(libraryProcedure));
  currProc->procedureName = (char *) safeCalloc(strlen(procedureName)+1,sizeof(char));
  strcpy(currProc->procedureName,procedureName);
  currProc->code = myFunction;
  currProc->signature = copyChainWithoutReversal(signature, copyIntPtrOnVoid);
  
  
  libHandle->functionList = addElement(libHandle->functionList,currProc);

  return currProc;
} 


libraryProcedure *getProcedure(char *procedureName) {
  chain *currLibList, *currProcList;
  libraryProcedure *currProc;
  libraryHandle *currLibHandle;

  currLibList = openedProcLibraries;
  while (currLibList != NULL) {
    currLibHandle = (libraryHandle *) currLibList->value;
    currProcList = currLibHandle->functionList;
    while (currProcList != NULL) {
      currProc = (libraryProcedure *) currProcList->value;
      if (strcmp(currProc->procedureName,procedureName) == 0)
	return currProc;
      currProcList = currProcList->next;
    }
    currLibList = currLibList->next;
  }

  return NULL;
}


void freeProcLibraries() {
  chain *currLibList, *currProcList, *prevProcList, *prevLibList;
  libraryProcedure *currProc;
  libraryHandle *currLibHandle;

  currLibList = openedProcLibraries;
  while (currLibList != NULL) {
    currLibHandle = (libraryHandle *) currLibList->value;
    currProcList = currLibHandle->functionList;
    while (currProcList != NULL) {
      currProc = (libraryProcedure *) currProcList->value;
      free(currProc->procedureName);
      freeChain(currProc->signature,freeIntPtr);
      free(currProcList->value);
      prevProcList = currProcList;
      currProcList = currProcList->next;
      free(prevProcList);
    }
    dlerror();
    if (dlclose(currLibHandle->libraryDescriptor) != 0) 
      printMessage(1,"Warning: could not close libary \"%s\": %s\n",currLibHandle->libraryName,dlerror());
    free(currLibHandle->libraryName);
    free(currLibHandle);
    prevLibList = currLibList;
    currLibList = currLibList->next;
    free(prevLibList);
  }
  openedProcLibraries = NULL;
}

