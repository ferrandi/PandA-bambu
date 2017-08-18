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
#include "sollya.h"

/* Example of an external functionnality linked to a function symbol in sollya

   Compile with 

     gcc -fPIC -Wall -c externalfun.c 
     gcc -fPIC -shared -o externalfun externalfun.o 

   Function foo will be linked by

   foo = library("./externalfun");

   The C function foo is supposed to affect rop with an enclosure of 
   the image of the n-th derivative of the function on interval op.
   It is supposed to be sensitive to the precision of rop.
   
   Remark that most sollya functionalities rely on the fact that
   at least the second derivative can be finitely enclosed. The C function
   may return [-inf; +inf] as an enclosure for the higher derivatives.

   The C function foo can use all functionalities inside sollya that
   have been exported in sollya.h. Some care must be taken with regard
   to the memory management inside the C function if sollya functionalities
   are used: be sure that all memory is freed before returning.

   The C function foo is supposed to return an integer. Its meaning is to be defined.

*/


int foo(mpfi_t rop, mpfi_t op, int n) {


  return 0;
}
