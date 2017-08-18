/*

Copyright 2006-2009 by 

Laboratoire de l'Informatique du Parallelisme, 
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668

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

#ifndef ASSIGNMENT_H
#define ASSIGNMENT_H

#include "expression.h"
#include "chain.h"


typedef struct entryStruct entry;

struct entryStruct 
{
  char *name;
  void *value;
};


chain *addEntry(chain *symTbl, char *name, void *value, void * (*copyValue) (void *));
int containsEntry(chain *symTbl, char *name);
void *getEntry(chain *symTbl, char *name, void * (*copyValue) (void *));
chain *removeEntry(chain *symTbl, char *name, void (*f) (void *));
void freeSymbolTable(chain *symTbl, void (*f) (void *));
void freeDeclaredSymbolTable(chain *declSymTbl, void (*f) (void *));
chain *pushFrame(chain *declSymTbl);
chain *popFrame(chain *declSymTbl, void (*f) (void *));
int containsDeclaredEntry(chain *declSymTbl, char *name);
void *getDeclaredEntry(chain *declSymTbl, char *name, void * (*copyValue) (void *));
chain *assignDeclaredEntry(chain *declSymTbl, char *name, void *value, void * (*copyValue) (void *), void (*freeValue) (void *));
chain *declareNewEntry(chain *declSymTbl, char *name, void *value, void * (*copyValue) (void *));


#endif /* ifdef ASSIGNMENT_H*/
