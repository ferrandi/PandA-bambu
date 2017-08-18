/*

Copyright 2007-2009 by 

Laboratoire de l'Informatique du Parallelisme, 
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668

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

#include <mpfi.h>
#include <mpfr.h>
#include "sollya.h"

/* Example of an external procedure linked to an identifier in sollya

   Compile with 

     gcc -fPIC -Wall -c externalproc.c 
     gcc -fPIC -shared -o externalproc externalproc.o 

   Procedure foo will be linked by

   externalproc(foo, "./externalproc", signature);

   where signature is the signature of the function as type -> type or (type, type, ...) -> type
   where type is one of

   void, constant, function, range, integer, string, boolean, list of constant, 
   list of function, list of range, list of integer, list of string, list of boolean.
   
   The C function foo is supposed to return an integer indicating success. 
   It returns its result depending on its sollya result type as follows:

   * void :        the C function has no pointer argument for the result
   * constant:     the first argument of the C function is of C type mpfr_t *, 
                   the result is returned by affecting the MPFR variable
   * function:     the first argument of the C function is of C type node **,
                   the result is returned by the node * pointed with a new node *
   * range:        the first argument of the C function is of C type mpfi_t *,
                   the result is returned by affecting the MPFI variable
   * integer:      the first argument of the C function is of C type int *,
                   the result is returned by affecting the int variable
   * string:       the first argument of the C function is of C type char **,
                   the result is returned by the char * pointed with a new char *
   * boolean:      the first argument of the C function is of C type int *,
                   the result is returned by affecting the int variable with a boolean value
   * list of type: the first argument of the C function is of C type chain **,
                   the result is returned by the chain * pointed with a new chain *
		   containing for sollya type 
		   - constant: pointers mpfr_t * to new MPFR variables
                   - function: pointers node * to new nodes
                   - range:    pointers mpfi_t * to new MPFI variables
                   - integer:  pointers int * to new int variables
		   - string:   pointers char * to new char * variables
		   - boolean:  pointers int * to new int variables representing boolean values.
	       
   The C function affects its possible pointer argument if and only if it succeeds.
   This means, if the function returns an integer indicating failure, it does not 
   leak any memory to the encompassing environment.
 
   The C function foo receives its arguments as follows: 
   If the sollya argument type is void, no argument array is given. 
   Otherwise the C function receives a C void ** argument representing an array 
   of size equal to the arity of the function where each entry (of C type void *) represents
   a value with a C type depending on the corresponding sollya type:

   * constant:     the C type the void * is to be casted to is mpfr_t *
   * function:     the C type the void * is to be casted to is node *
   * range:        the C type the void * is to be casted to is mpfi_t *
   * integer:      the C type the void * is to be casted to is int *
   * string:       the C type the void * is to be casted to is char *
   * boolean:      the C type the void * is to be casted to is int *
   * list of type: the C type the void * is to be casted to is chain *
                   where depending on sollya type, the values in the chain are
		   to be casted to 
		   - constant: mpfr_t *
                   - function: node *
                   - range:    mpfi_t *
                   - integer:  int *
		   - string:   char *
		   - boolean:  int *.

   The C function is not supposed to alter the memory pointed by its array argument void **.

   In both directions (argument and result values), empty lists are represented by chain * NULL pointers.

*/


/* Signature (integer, integer) -> integer */

int foo(int *res, void **args) {

  *res = *((int *) args[0]) + *((int *) args[1]);

  return 1;
}
