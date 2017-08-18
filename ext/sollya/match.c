/*

Copyright 2011 by

Laboratoire d'Informatique de Paris 6, equipe PEQUAN,
UPMC Universite Paris 06 - CNRS - UMR 7606 - LIP6, Paris, France.

Contributors Ch. Lauter

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
herefore means  that it is reserved for developers  and  experienced
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

#include "expression.h"
#include "execute.h"
#include "chain.h"
#include "assignment.h"
#include <string.h>
#include <stdlib.h>

/* Declare tryMatch immediately as it is recursively used almost
   everywhere */
int tryMatch(chain **, node *, node *);

/* Add association identifier -> thing to list 
   
   If identifier exists already:
      - And is associated to a thing that is equal to thing, return success (1)
      - And is associated to a different thing, return failure (0)

   Otherwise: create the association identifier -> thing and add it to the 
              list, return success (1)

*/
int associateThing(chain **associations, char *identifier, node *thing) {
  chain *curr;
  entry *newEntry;

  for (curr = *associations; curr != NULL; curr = curr->next) {
    if (!strcmp(identifier, ((char *) (((entry *) (curr->value))->name)))) {
      /* Identifier exists already in list */
      if (isEqualThing(thing, ((node *) (((entry *) (curr->value))->value)))) {
	/* But it is associated to the same thing */
	return 1;
      } else {
	/* But it is associated to another thing */
	return 0;
      }
    }
  }

  newEntry = (entry *) safeMalloc(sizeof(entry));
  newEntry->name = (char *) safeCalloc(strlen(identifier) + 1,sizeof(char));
  strcpy(newEntry->name, identifier);
  newEntry->value = copyThing(thing);
  *associations = addElement(*associations, newEntry);

  return 1;
}

int tryCombineAssociations(chain **associations, chain *assoc1, chain *assoc2) {
  int okay;
  chain *myAssociations = NULL;
  chain *curr;

  if ((assoc1 == NULL) && (assoc2 == NULL)) {
    *associations = NULL;
    return 1;
  }

  if (assoc1 == NULL) {
    *associations = copyChain(assoc2,copyEntryOnVoid);
    return 1;
  }

  if (assoc2 == NULL) {
    *associations = copyChain(assoc1,copyEntryOnVoid);
    return 1;
  }

  okay = 1;
  myAssociations = copyChain(assoc1,copyEntryOnVoid);

  for (curr=assoc2; curr != NULL; curr = curr->next) {
    if (!associateThing(&myAssociations, 
			((char *) ((entry *) (curr->value))->name), 
			((node *) ((entry *) (curr->value))->value))) {
      okay = 0;
      break;
    }
  }

  if (okay) {
    *associations = myAssociations;
  } else {
    if (myAssociations != NULL) {
      freeChain(myAssociations, freeEntryOnVoid);
    }
  }

  return okay;
}

node *headFunction(node *tree) {
  node *copy;
  
  switch (tree->nodeType) {
  case VARIABLE:
  case CONSTANT:
  case ADD:
  case SUB:
  case MUL:
  case DIV:
  case POW:
  case PI_CONST:
  case LIBRARYCONSTANT:
    copy = NULL;
    break;
  case SQRT:
  case EXP:
  case LOG:
  case LOG_2:
  case LOG_10:
  case SIN:
  case COS:
  case TAN:
  case ASIN:
  case ACOS:
  case ATAN:
  case SINH:
  case COSH:
  case TANH:
  case ASINH:
  case ACOSH:
  case ATANH:
  case NEG:
  case ABS:
  case DOUBLE:
  case SINGLE:
  case QUAD:
  case HALFPRECISION:
  case DOUBLEDOUBLE:
  case TRIPLEDOUBLE:
  case ERF: 
  case ERFC:
  case LOG_1P:
  case EXP_M1:
  case DOUBLEEXTENDED:
  case CEIL:
  case FLOOR:
  case NEARESTINT:
    copy = (node *) safeMalloc(sizeof(node));
    copy->nodeType = tree->nodeType;
    copy->child1 = makeVariable();
    break;
  case LIBRARYFUNCTION:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LIBRARYFUNCTION;
    copy->libFun = tree->libFun;
    copy->libFunDeriv = tree->libFunDeriv;
    copy->child1 = makeVariable();
    break;
  case PROCEDUREFUNCTION:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = PROCEDUREFUNCTION;
    copy->libFunDeriv = tree->libFunDeriv;
    copy->child1 = makeVariable();
    copy->child2 = copyThing(tree->child2);
    break;
  default:
   sollyaFprintf(stderr,"Error: headFunction: unknown identifier in the tree\n");
   exit(1);
  }
  return copy;
}

int tryMatchExtendedPureTree(chain **associations, node *thingToMatch, node *possibleMatcher) {
  chain *leftAssoc, *rightAssoc, *recursionAssoc;
  int okay, recursion, matchRecursion;
  node *currentThingToMatch, *currentPossibleMatcher, *headSymbol, *newHeadSymbol, *tempNode;
  char *currentIdentifier;
  
  /* Special case: possibleMatcher is default.
     Everthing that is correctly typed (and we know it is) matches
     without creating any binding.
  */
  if (possibleMatcher->nodeType == DEFAULT) {
    return 1;
  }

  /* Special case: possibleMatcher is a free variable to bind
     Check if it is possible equal to the mathematical free 
     variable. If not, create an association 
  */
  if (possibleMatcher->nodeType == TABLEACCESS) {
    if ((variablename != NULL) &&
	(!strcmp(variablename, possibleMatcher->string))) {
      /* Here, the free variable to bind is actually equal
	 to the free mathematical variable. 
	 
	 We have a match if the thing to match is 
	 also equal to the free mathematical variable.

	 Otherwise, we have no match.

	 In none of the cases, we have to establish an 
	 association nor call ourselves for recursion.
      */
      return (thingToMatch->nodeType == VARIABLE);
    }
    /* Here, the free variable is not the mathematical one.
       We have to establish an association and return success
       (unless the association fails).
    */
    return associateThing(associations, possibleMatcher->string, thingToMatch);
  }

  /* Second special case: possibleMatcher is a free variable in an 
     application context (e.g. f(thing)) 

     First check, if recursion is actually possible on the thing to match.
     Recursion is not possible:
       - On a variable
       - On a constant
       - On pi
       - On a library constant
       - On a binary function (as we have only one variable on the 
         possible matcher).

     Second check if it is equal to the free mathematical variable. If
     it is, consider the free mathematic variable as the identity
     function (emitting a warning) and return the match result of
     child tree.

     Otherwise, perform matching logic for the case when f(thing)
     makes sense.
     
  */
  if (possibleMatcher->nodeType == TABLEACCESSWITHSUBSTITUTE) {
    /* Check if recursion is possible on the thing to match */
    switch (thingToMatch->nodeType) {
    case VARIABLE:
    case CONSTANT:
    case PI_CONST:
    case LIBRARYCONSTANT:
    case ADD:
    case SUB:
    case MUL:
    case DIV:
    case POW:
      return 0;
      break;
    default:
      break;
    }

    /* If we are here, we are sure that recursion is possible
       on the one, first and only child of the thing to match.

       Now check if the identifier is equal to the free 
       mathematical variable.
    */
    if ((variablename != NULL) &&
	(!strcmp(variablename, possibleMatcher->string))) {
      /* Here, the free variable to bind is actually equal
	 to the free mathematical variable. 

      */
      okay = tryMatchExtendedPureTree(associations, thingToMatch->child1, ((node *) (possibleMatcher->arguments->value)));
      if (okay) {
	printMessage(1,"Warning: the identifier \"%s\" is bound to the current free variable. In a functional context it will be considered as the identity function.\n",
		     variablename);
      }
      return okay;
    }
    /* Here, the free variable is not the mathematical one. 
       
       We know that recursion is possible on the one, first and
       only child of the thing to match.
       
    */
    okay = 0;
    recursion = 1;
    currentThingToMatch = thingToMatch->child1; /* No copy, just the ptr */
    currentPossibleMatcher = (node *) (possibleMatcher->arguments->value); /* No copy, just the ptr */
    currentIdentifier = possibleMatcher->string; /* No copy, just the ptr */
    headSymbol = headFunction(thingToMatch); /* This one performs mallocs */
    if (headSymbol == NULL) {
      /* This case should never happen */
      return 0;
    }
    do {
      recursionAssoc = NULL;
      matchRecursion = tryMatchExtendedPureTree(&recursionAssoc, currentThingToMatch, currentPossibleMatcher);
      if (matchRecursion) {
	/* We have a match on recursion. This means we just have to check if we can associate 
	   the current identifier with the headSymbol */
	recursion = 0;
	okay = associateThing(&recursionAssoc, currentIdentifier, headSymbol);
	if (okay) {
	  *associations = copyChainWithoutReversal(recursionAssoc, copyEntryOnVoid);
	}
      } else {
	/* Here, we do not match on recursion. We have to check now if 
	   we can extend the head symbol to one level below and check 
	   again.

	   There is no recursion possible:
	     - if the current thing to match is one of 
	        -- variable
                -- constant
                -- pi 
                -- libraryconstant
                -- binary function
             - if the current possible matcher is a free variable in an application context
	*/
	switch (currentThingToMatch->nodeType) {
	case VARIABLE:
	case CONSTANT:
	case PI_CONST:
	case LIBRARYCONSTANT:
	case ADD:
	case SUB:
	case MUL:
	case DIV:
	case POW:
	  recursion = 0;
	  break;
	default:
	  break;
	}
	if (recursion) {
	  if (currentPossibleMatcher->nodeType == TABLEACCESSWITHSUBSTITUTE) recursion = 0;
	  if (recursion) {
	    /* Here, recursion is possible 

	       Compute a new head symbol.
	       
	       Move currentThingToMatch to the one, first and only
	       child of the current thing to match.

	     */
	    tempNode = headFunction(currentThingToMatch);
	    if (tempNode == NULL) {
	      /* This case should never happen */
	      recursion = 0;
	    } else {
	      newHeadSymbol = substitute(headSymbol, tempNode);
	      free_memory(tempNode);
	      free_memory(headSymbol);
	      headSymbol = newHeadSymbol;
	      currentThingToMatch = currentThingToMatch->child1;
	    }
	  }
	}
      }
      if (recursionAssoc != NULL) freeChain(recursionAssoc, freeEntryOnVoid);
    } while (recursion);
    free_memory(headSymbol);

    if (okay) {
      /* If we have a match, we have to check if the free mathematical
	 variable has already been named. Otherwise, we have created
	 a free mathematical variable in the association that does not
	 have a name */
      if (variablename == NULL) {
	printMessage(1,"Warning: the current free variable is not bound to an identifier. The matching of head function symbols requires this binding.\n");
	printMessage(1,"Will bind the current free variable to the identifier \"x\"\n");
	variablename = (char *) safeCalloc(2, sizeof(char));
	variablename[0] = 'x';
      }
    }
    return okay;
  }

  /* Here, possibleMatcher is itself a pure tree, i.e. a function without
     free variables to bind.

     To start with, we cannot have a match if the head symbols are not
     the same.
  */
  if (possibleMatcher->nodeType != thingToMatch->nodeType) return 0;

  /* Here, the head symbols are always the same */
  switch (possibleMatcher->nodeType) {
  case VARIABLE:
    /* The free mathematical variable matches the free mathematical variable */
    return 1;
    break;
  case CONSTANT:
    /* Constants match if they are the same. We have to be careful with NaNs */
    if (mpfr_nan_p(*(possibleMatcher->value)) && mpfr_nan_p(*(thingToMatch->value))) return 1;
    if (mpfr_equal_p(*(possibleMatcher->value),*(thingToMatch->value))) return 1; 
    return 0;
    break;
  case ADD:
  case SUB:
  case MUL:
  case DIV:
  case POW:
    /* Binary functions match if both sub-expressions match and if we can combine ("unify") 
       both association lists.
    */
    leftAssoc = NULL;
    rightAssoc = NULL;
    if (!tryMatchExtendedPureTree(&leftAssoc, thingToMatch->child1, possibleMatcher->child1)) return 0;
    if (!tryMatchExtendedPureTree(&rightAssoc, thingToMatch->child2, possibleMatcher->child2)) return 0;
    okay = tryCombineAssociations(associations, leftAssoc, rightAssoc);
    if (leftAssoc != NULL) freeChain(leftAssoc, freeEntryOnVoid);
    if (rightAssoc != NULL) freeChain(rightAssoc, freeEntryOnVoid);
    return okay;
    break;
  case SQRT:
  case EXP:
  case LOG:
  case LOG_2:
  case LOG_10:
  case SIN:
  case COS:
  case TAN:
  case ASIN:
  case ACOS:
  case ATAN:
  case SINH:
  case COSH:
  case TANH:
  case ASINH:
  case ACOSH:
  case ATANH:
  case NEG:
  case ABS:
  case DOUBLE:
  case SINGLE:
  case QUAD:
  case HALFPRECISION:
  case DOUBLEDOUBLE:
  case TRIPLEDOUBLE:
  case ERF: 
  case ERFC:
  case LOG_1P:
  case EXP_M1:
  case DOUBLEEXTENDED:
  case CEIL:
  case FLOOR:
  case NEARESTINT:
    /* Unary functions: we have a match if we have a match on recursion
       Possible associations are established on recursion. 
    */
    return tryMatchExtendedPureTree(associations, thingToMatch->child1, possibleMatcher->child1);
    break;
  case PI_CONST:
    /* Pi matches pi */
    return 1;
    break;
  case LIBRARYFUNCTION:
    /* Library functions to not match if they are not bound to the same code
       or if they are at different differentiation levels
    */
    if ((possibleMatcher->libFun != thingToMatch->libFun) ||
	(possibleMatcher->libFunDeriv != thingToMatch->libFunDeriv)) return 0;
    /* Otherwise they match when recursion matches */
    return tryMatchExtendedPureTree(associations, thingToMatch->child1, possibleMatcher->child1);
    break;
  case LIBRARYCONSTANT:
    /* Library constants match iff they are bound to the same code */
    return (possibleMatcher->libFun == thingToMatch->libFun);
    break;
  case PROCEDUREFUNCTION:
    /* Procedure functions do not match if they are not based on the same
       procedure or if they are at different differentiation levels
    */
    if ((!isEqualThing(possibleMatcher->child2, thingToMatch->child2)) ||
	(possibleMatcher->libFunDeriv != thingToMatch->libFunDeriv)) return 0;
    /* Otherwise they match when recursion matches */
    return tryMatchExtendedPureTree(associations, thingToMatch->child1, possibleMatcher->child1);    
    break;
  default:
    sollyaFprintf(stderr,"Error: tryMatchExtendedPureTree: unknown identifier (%d) in the tree\n",possibleMatcher->nodeType);
    exit(1);
  }

  /* We should never get here */
  return 0;
}

int tryMatchRange(chain **associations, mpfr_t a, mpfr_t b, node *possibleMatcher) {
  chain *myAssociations;
  node *tempNode;
  int okay;

  if (possibleMatcher->nodeType != RANGE) return 0;
  if (!((possibleMatcher->child1->nodeType == CONSTANT) || (possibleMatcher->child1->nodeType == TABLEACCESS) || (possibleMatcher->child1->nodeType == DEFAULT))) return 0;
  if (!((possibleMatcher->child2->nodeType == CONSTANT) || (possibleMatcher->child2->nodeType == TABLEACCESS) || (possibleMatcher->child2->nodeType == DEFAULT))) return 0;
  if (variablename != NULL) {
    if (possibleMatcher->child1->nodeType == TABLEACCESS) {
      if (!strcmp(possibleMatcher->child1->string,variablename)) return 0;
    }
    if (possibleMatcher->child2->nodeType == TABLEACCESS) {
      if (!strcmp(possibleMatcher->child2->string,variablename)) return 0;
    }
  }

  if (possibleMatcher->child1->nodeType == CONSTANT) {
    if ((!mpfr_equal_p(*(possibleMatcher->child1->value),a)) && 
	(!(mpfr_nan_p(*(possibleMatcher->child1->value)) && mpfr_nan_p(a)))) return 0;
  }
  if (possibleMatcher->child2->nodeType == CONSTANT) {
    if ((!mpfr_equal_p(*(possibleMatcher->child2->value),b)) && 
	(!(mpfr_nan_p(*(possibleMatcher->child2->value)) && mpfr_nan_p(b)))) return 0;
  }

  /* Here, we know that the possible Matcher is a range, that its
     childs are either constants equal to the bounds of the thing to
     match or that they are free variables different from the free
     mathematical variable or default */

  myAssociations = NULL;
  okay = 1;

  if (possibleMatcher->child1->nodeType == TABLEACCESS) {
    tempNode = makeConstant(a);
    okay = associateThing(&myAssociations, possibleMatcher->child1->string, tempNode);
    free_memory(tempNode);
  } 

  if (okay && (possibleMatcher->child2->nodeType == TABLEACCESS)) {
    tempNode = makeConstant(b);
    okay = associateThing(&myAssociations, possibleMatcher->child2->string, tempNode);
    free_memory(tempNode);
  }

  if (okay) {
    *associations = myAssociations;
  } else {
    if (myAssociations != NULL) freeChain(myAssociations, freeEntryOnVoid);
  }  
  return okay;
}

int isIntegerElement(int *res, node *thing) {
  mpfr_t a, b;
  int i, okay;

  if (!isPureTree(thing)) return 0;
  if (!isConstant(thing)) return 0;

  okay = 0; i = 0;
  mpfr_init2(a, tools_precision);
  if (evaluateThingToConstant(a, thing, NULL, 0)) {
    if (mpfr_integer_p(a)) {
      i = mpfr_get_si(a, GMP_RNDN);
      mpfr_init2(b, 8 * sizeof(i) + 5);
      mpfr_set_si(b, i, GMP_RNDN);
      if (mpfr_cmp(a, b) == 0) {
	okay = 1;
      }
      mpfr_clear(b);
    }
  }
  mpfr_clear(a);

  if (okay) {
    *res = i;
  }
  return okay;
}


int formConsecutiveIntegers(node *thing1, node *thing2) {
  mpfr_t a, b, c, d;
  int i, j, okay;
  
  if (!isPureTree(thing1)) return 0;
  if (!isPureTree(thing2)) return 0;
  if (!isConstant(thing1)) return 0;
  if (!isConstant(thing2)) return 0;

  okay = 0;
  mpfr_init2(a, tools_precision);
  if (evaluateThingToConstant(a, thing1, NULL, 0)) {
    if (mpfr_integer_p(a)) {
      i = mpfr_get_si(a, GMP_RNDN);
      mpfr_init2(b, 8 * sizeof(i) + 5);
      mpfr_set_si(b, i, GMP_RNDN);
      if (mpfr_cmp(a, b) == 0) {
	mpfr_init2(c, tools_precision);
	if (evaluateThingToConstant(c, thing2, NULL, 0)) {
	  if (mpfr_integer_p(c)) {
	    j = mpfr_get_si(c, GMP_RNDN);
	    mpfr_init2(d, 8 * sizeof(i) + 5);
	    mpfr_set_si(d, j, GMP_RNDN);
	    if (mpfr_cmp(c, d) == 0) {
	      okay = (i + 1 == j);
	    }
	    mpfr_clear(d);
	  }
	}
	mpfr_clear(c);
      }
      mpfr_clear(b);
    }
  }
  mpfr_clear(a);

  return okay;
}

chain *normalizeFinalEllipticList(chain *list) {
  node **thingArray;
  int i, len, shortenedLen, mayShorten;
  chain *curr;
  chain *copy;
  
  if (list == NULL) return NULL;

  len = lengthChain(list);
  shortenedLen = len;
  thingArray = (node **) safeCalloc(len,sizeof(node *));
  for (i=0, curr=list; curr != NULL; curr = curr->next, i++)
    thingArray[i] = copyThing((node *) (curr->value));

  while (shortenedLen >= 2) {
    mayShorten = 0;
    if ((thingArray[shortenedLen - 2]->nodeType != TABLEACCESS) && 
	(thingArray[shortenedLen - 1]->nodeType != TABLEACCESS)) {
      if (formConsecutiveIntegers(thingArray[shortenedLen - 2], 
				  thingArray[shortenedLen - 1])) {
	mayShorten = 1;
      } else {
	if (isEqualThing(thingArray[shortenedLen - 2],
			 thingArray[shortenedLen - 1]) || 
	    ((thingArray[shortenedLen - 2]->nodeType == CONSTANT) && 
	     (thingArray[shortenedLen - 1]->nodeType == CONSTANT) && 
	     mpfr_nan_p(*(thingArray[shortenedLen - 2]->value)) &&
	     mpfr_nan_p(*(thingArray[shortenedLen - 2]->value)))) {
	  mayShorten = 1;
	}
      }
    }
    if (mayShorten) {
      shortenedLen--;
    } else {
      break;
    }
  }

  copy = NULL;
  for (i=shortenedLen-1;i>=0;i--) {
    copy = addElement(copy,copyThing(thingArray[i]));
  }
  for (i=0;i<len;i++) freeThing(thingArray[i]);
  free(thingArray);

  return copy;
}

int tryMatchList(chain **associations, node *thingToMatch, node *possibleMatcher) {
  int okay, freeLists, lenPossibleMatcherList, lenThingToMatchList;
  chain *currThingToMatch, *currPossibleMatcher, *thingToMatchList, *possibleMatcherList;
  chain *myAssociations, *elementAssociations, *myNewAssociations;
  chain *lastThingToMatch;
  mpfr_t c, d;
  int i, integerBase, integerSequence;
  node *implicitElement;

  /* Make compiler happy */
  lastThingToMatch = NULL;
  /* End of compiler happiness */

  /* All possibilities for empty lists */
  if (isEmptyList(possibleMatcher)) {
    if (isEmptyList(thingToMatch)) return 1;
    return 0;
  }
  if (isEmptyList(thingToMatch)) return 0;

  /* Check that we have got lists */
  if (!(isPureList(thingToMatch) || isPureFinalEllipticList(thingToMatch))) return 0;
  if (!(isPureList(possibleMatcher) || isPureFinalEllipticList(possibleMatcher))) return 0;

  /* Here, both the possible matcher and the thing to match are lists.
 
     Check if both are of the same type of lists.
  */
  if (thingToMatch->nodeType != possibleMatcher->nodeType) return 0;

  /* Here, both lists are of the same type 

     Now check if normalization is needed for final elliptic lists.

   */
  okay = 1;
  myAssociations = NULL;
  freeLists = 0;
  if (isPureFinalEllipticList(thingToMatch) && 
      (lengthChain(thingToMatch->arguments) != lengthChain(possibleMatcher->arguments))) {
    thingToMatchList = normalizeFinalEllipticList(thingToMatch->arguments);
    possibleMatcherList = normalizeFinalEllipticList(possibleMatcher->arguments);
    freeLists = 1;
  } else {
    thingToMatchList = thingToMatch->arguments;
    possibleMatcherList = possibleMatcher->arguments;
    freeLists = 0;
  }

  /* After normalization, there can be no match if the lists are not
     of the same length (for lists) or if the possibleMatcherList is
     shorter than the thingToMatchList
  */
  lenThingToMatchList = lengthChain(thingToMatchList);
  lenPossibleMatcherList = lengthChain(possibleMatcherList);
  okay = (lenThingToMatchList == lenPossibleMatcherList) || 
    (isPureFinalEllipticList(thingToMatch) && 
     (lenThingToMatchList <= lenPossibleMatcherList));
  
  if (okay) {
    /* Matching is possible because the lists are of appropriate
       lengths.  Now check if each element pair of the lists
       matches. Keep attention to the associations.
    */
    myAssociations = NULL;
    for (currThingToMatch = thingToMatchList, currPossibleMatcher = possibleMatcherList;
	 (currThingToMatch != NULL) && (currPossibleMatcher != NULL);
	 currThingToMatch = currThingToMatch->next, currPossibleMatcher = currPossibleMatcher->next) {
      lastThingToMatch = currThingToMatch;
      elementAssociations = NULL;
      okay = tryMatch(&elementAssociations, (node *) (currThingToMatch->value), (node *) (currPossibleMatcher->value));
      if (okay) {
	myNewAssociations = NULL;
	okay = tryCombineAssociations(&myNewAssociations, myAssociations, elementAssociations);
	if (myAssociations != NULL) freeChain(myAssociations, freeEntryOnVoid);
	myAssociations = myNewAssociations;
      } 
      if (elementAssociations != NULL) freeChain(elementAssociations, freeEntryOnVoid);
      if (!okay) break;
    }
  }

  if (okay && isPureFinalEllipticList(thingToMatch) && (currPossibleMatcher != NULL)) {
    /* Here, the explicit possible matcher list is longer and both
       list are final elliptic.

       We have to continue on the possible matcher list until it is
       empty (explicitly speaking). For each of its elements, we have
       to "invent", i.e.  generate, an implicit element of the
       thing-to-match list. This generation is based on the type of
       the last explicit element in the thing-to-match list, stored
       on variable lastThingToMatch->value.
       
       We start by performing a test that allows us to decide if 
       we have a sequence of integers or another type continued to 
       infinity.
    */
    integerSequence = 0;
    integerBase = 0;
    mpfr_init2(c, tools_precision);
    if (evaluateThingToConstant(c, (node *) (lastThingToMatch->value), NULL, 0)) {
      if (mpfr_integer_p(c)) {
	integerBase = mpfr_get_si(c, GMP_RNDN);
	mpfr_init2(d, 8 * sizeof(i) + 5);
	mpfr_set_si(d, integerBase, GMP_RNDN);
	if (mpfr_cmp(c, d) == 0) {
	  integerSequence = 1;
	}
	mpfr_clear(d);
      }
    }
    mpfr_clear(c);

    /* Now check all remaining elements of the possible matcher */
    for (i = 1; (currPossibleMatcher != NULL); currPossibleMatcher = currPossibleMatcher->next, i++) {
      /* Start by generating the implicit element of the sequence */
      if (integerSequence) {
	mpfr_init2(c, 8 * sizeof(int) + 1 + 5);
	mpfr_set_si(c,integerBase,GMP_RNDN); /* exact */
	mpfr_add_si(c,c,i,GMP_RNDN); /* exact */
	implicitElement = makeConstant(c);
	mpfr_clear(c);
      } else {
	/* We just have to repeat the last element of the sequence 
	   
	   We avoid mallocs here. We'll have to account for that later.
	 */
	implicitElement = (node *) (lastThingToMatch->value);
      }

      /* Now perform the match check */
      elementAssociations = NULL;
      okay = tryMatch(&elementAssociations, implicitElement, (node *) (currPossibleMatcher->value));
      if (okay) {
	myNewAssociations = NULL;
	okay = tryCombineAssociations(&myNewAssociations, myAssociations, elementAssociations);
	if (myAssociations != NULL) freeChain(myAssociations, freeEntryOnVoid);
	myAssociations = myNewAssociations;
      } 
      if (elementAssociations != NULL) freeChain(elementAssociations, freeEntryOnVoid);

      /* If we have an integer sequence, we malloc'ed for the
	 implicit element. So we have to free it.
      */
      if (integerSequence) {
	freeThing(implicitElement);
      }
      if (!okay) break;
    }
  }

  /* Free the list if it was us who allocated them */
  if (freeLists) {
    freeChain(thingToMatchList,freeThingOnVoid);
    freeChain(possibleMatcherList,freeThingOnVoid);
  }
  
  if (okay) {
    *associations = myAssociations;
  } else {
    if (myAssociations != NULL) freeChain(myAssociations, freeEntryOnVoid);
  }  
  return okay;
}

int tryMatchPrepend(chain **associations, node *thingToMatch, node *possibleMatcher) {
  int okay;
  chain *myAssociations, *headAssoc, *tailAssoc;
  node *tailList, *tempNode;

  if ((thingToMatch->nodeType != LIST) && (thingToMatch->nodeType != FINALELLIPTICLIST)) return 0;
  
  myAssociations = NULL;
  
  headAssoc = NULL;
  okay = tryMatch(&headAssoc, (node *) (thingToMatch->arguments->value), possibleMatcher->child1);
  if (okay) {
    tempNode = makeTail(copyThing(thingToMatch));
    tailList = evaluateThing(tempNode);
    freeThing(tempNode);
    tailAssoc = NULL;
    okay = tryMatch(&tailAssoc, tailList, possibleMatcher->child2);
    if (okay) {
	okay = tryCombineAssociations(&myAssociations, headAssoc, tailAssoc);
    }
    if (tailAssoc != NULL) freeChain(tailAssoc, freeEntryOnVoid);
    freeThing(tailList);
  }
  if (headAssoc != NULL) freeChain(headAssoc, freeEntryOnVoid);

  if (okay) {
    *associations = myAssociations;
  } else {
    if (myAssociations != NULL) freeChain(myAssociations, freeEntryOnVoid);
  }  
  return okay;
}

int tryMatchAppend(chain **associations, node *thingToMatch, node *possibleMatcher) {
  int okay, len, i;
  chain *myAssociations, *headAssoc, *tailAssoc, *curr;
  node *headList, *tailElement, **elementArray;

  if (thingToMatch->nodeType != LIST) return 0;
  
  myAssociations = NULL;

  if (thingToMatch->arguments->next != NULL) {
    /* The list of things to match has more than one element. Its is
       easy to compute the head list and the tailElement.
    */
    len = lengthChain(thingToMatch->arguments);
    elementArray = (node **) safeCalloc(len,sizeof(node *));
    for (i=0, curr=thingToMatch->arguments; curr != NULL; curr = curr->next, i++) {
      elementArray[i] = (node *) (curr->value);
    }
    tailElement = elementArray[len-1];
    curr = NULL;
    for (i=len-2;i>=0;i--) {
      curr = addElement(curr,copyThing(elementArray[i]));
    }
    headList = makeList(curr);
    free(elementArray);
  } else {
    /* The list of things to match has only one element, so the head
       list is the empty list. The tail element is easy to get, too. 
    */
    tailElement = (node *) (thingToMatch->arguments->value);
    headList = makeEmptyList();
  }

  headAssoc = NULL;
  okay = tryMatch(&headAssoc, headList, possibleMatcher->child1);
  if (okay) {
    tailAssoc = NULL;
    okay = tryMatch(&tailAssoc, tailElement, possibleMatcher->child2);
    if (okay) {
	okay = tryCombineAssociations(&myAssociations, headAssoc, tailAssoc);
    }
    if (tailAssoc != NULL) freeChain(tailAssoc, freeEntryOnVoid);
  }
  if (headAssoc != NULL) freeChain(headAssoc, freeEntryOnVoid);
   
  freeThing(headList);

  if (okay) {
    *associations = myAssociations;
  } else {
    if (myAssociations != NULL) freeChain(myAssociations, freeEntryOnVoid);
  }  
  return okay;
}

int tryEvaluateRecursiveConcatMatcherToString(char **concatenatedString, node *tree) {
  char *buf, *bufLeft, *bufRight, *curr;
  int okayLeft, okayRight, okay;

  if (tree->nodeType == STRING) {
    buf = (char *) safeCalloc(strlen(tree->string) + 1, sizeof(char));
    strcpy(buf,tree->string);
    *concatenatedString = buf;
    return 1;
  }
  
  if (tree->nodeType == CONCAT) {
    okayLeft = tryEvaluateRecursiveConcatMatcherToString(&bufLeft, tree->child1);
    okayRight = tryEvaluateRecursiveConcatMatcherToString(&bufRight, tree->child2);
    
    okay = 0;
    if (okayLeft && okayRight) {
      buf = (char *) safeCalloc(strlen(bufLeft) + strlen(bufRight) + 1, sizeof(char));
      *concatenatedString = buf;
      for (curr=bufLeft; *curr != '\0'; curr++, buf++) {
	*buf = *curr;
      }
      for (curr=bufRight; *curr != '\0'; curr++, buf++) {
	*buf = *curr;
      }
      okay = 1;
    }
    if (okayLeft) free(bufLeft);
    if (okayRight) free(bufRight);

    return okay;
  }

  return 0;
}

int tryCutPrefix(char **rest, char *mainString, char *prefix) {
  int okay;
  char *ptrStr, *ptrPref, *buf;

  okay = 1;
  for (ptrStr = mainString, ptrPref = prefix;
       (*ptrStr != '\0') && (*ptrPref != '\0');
       ptrStr++, ptrPref++) {
    if (*ptrStr != *ptrPref) {
      okay = 0;
      break;
    }
  }
  if ((*ptrStr == '\0') && (*ptrPref != '\0')) {
    okay = 0;
  }

  if (okay) {
    buf = (char *) safeCalloc(strlen(ptrStr) + 1, sizeof(char));
    strcpy(buf, ptrStr);
    *rest = buf;
  }

  return okay;
}

char *revertString(char *buf) {
  char *revBuf;
  int len, i;

  len = strlen(buf);
  revBuf = (char *) safeCalloc(len + 1, sizeof(char));
  for (i=0;i<len;i++) {
    revBuf[len - 1 - i] = buf[i];
  }
  
  return revBuf;
}

int tryCutPostfix(char **rest, char *mainString, char *postfix) {
  char *revMainString, *revPostfix, *revRest;
  int okay;
  
  revMainString = revertString(mainString);
  revPostfix = revertString(postfix);
  okay = tryCutPrefix(&revRest, revMainString, revPostfix);
  if (okay) {
    *rest = revertString(revRest);
    free(revRest);
  }
  
  free(revMainString);
  free(revPostfix);

  return okay;
}

int tryMatchConcatOnString(chain **associations, char *stringToMatch, node *possibleMatcher) {
  int okayFullEvaluate, okay, okayLeftEvaluate, okayRightEvaluate, okayCut;
  char *stringFullEvaluate, *stringLeftEvaluate, *stringRightEvaluate, *restString;
  chain *myAssociations;
  node *restStringThing;

  myAssociations = NULL;

  okayFullEvaluate = tryEvaluateRecursiveConcatMatcherToString(&stringFullEvaluate, possibleMatcher);
  if (okayFullEvaluate) {
    okay = !strcmp(stringToMatch,stringFullEvaluate);
    free(stringFullEvaluate);
    return okay;
  }

  if (possibleMatcher->nodeType != CONCAT) return 0;

  okayLeftEvaluate = tryEvaluateRecursiveConcatMatcherToString(&stringLeftEvaluate, possibleMatcher->child1);
  okayRightEvaluate = tryEvaluateRecursiveConcatMatcherToString(&stringRightEvaluate, possibleMatcher->child2);

  if (okayLeftEvaluate || okayRightEvaluate) {
    okay = 0;
    if (okayLeftEvaluate) {
      okayCut = tryCutPrefix(&restString,stringToMatch,stringLeftEvaluate);
    } else {
      okayCut = tryCutPostfix(&restString,stringToMatch,stringRightEvaluate);
    }
    if (okayCut) {
      restStringThing = makeString(restString);
      myAssociations = NULL;
      if (okayLeftEvaluate) {
	okay = tryMatch(&myAssociations,restStringThing,possibleMatcher->child2);
      } else {
	okay = tryMatch(&myAssociations,restStringThing,possibleMatcher->child1);
      }
      freeThing(restStringThing);
      free(restString);
    }
    if (okayLeftEvaluate) free(stringLeftEvaluate);
    if (okayRightEvaluate) free(stringRightEvaluate);
    if (okay) {
      *associations = myAssociations;
    } else {
      if (myAssociations != NULL) freeChain(myAssociations, freeEntryOnVoid);
    }  
    return okay;
  } 

  return 0;
}

int tryEvaluateRecursiveConcatMatcherToList(node **concatenatedList, node *tree) {
  node *evalLeft, *evalRight;
  int okayLeft, okayRight, okay;
  chain *tempChain, *tempChain2;

  switch (tree->nodeType) {
  case EMPTYLIST:
  case LIST:
  case FINALELLIPTICLIST:
    *concatenatedList = copyThing(tree);
    return 1;
    break;
  case PREPEND:
    evalRight = NULL;
    okayRight = tryEvaluateRecursiveConcatMatcherToList(&evalRight, tree->child2);
    if (okayRight) {
      okay = 0;
      switch (evalRight->nodeType) {
      case EMPTYLIST:
	*concatenatedList = makeList(addElement(NULL, copyThing(tree->child1)));
	okay = 1;
	break;
      case LIST:
	tempChain = copyChainWithoutReversal(evalRight->arguments, copyThingOnVoid);
	tempChain = addElement(tempChain, copyThing(tree->child1));
	*concatenatedList = makeList(tempChain);
	okay = 1;
	break;	
      case FINALELLIPTICLIST:
	tempChain = copyChainWithoutReversal(evalRight->arguments, copyThingOnVoid);
	tempChain = addElement(tempChain, copyThing(tree->child1));
	*concatenatedList = makeFinalEllipticList(tempChain);
	okay = 1;
	break;
      default:
	okay = 0;
      }
      freeThing(evalRight);
      return okay;
    }
    return 0;
    break;
  case APPEND:
    evalLeft = NULL;
    okayLeft = tryEvaluateRecursiveConcatMatcherToList(&evalLeft, tree->child1);
    if (okayLeft) {
      okay = 0;
      switch (evalLeft->nodeType) {
      case EMPTYLIST:
	*concatenatedList = makeList(addElement(NULL, copyThing(tree->child2)));
	okay = 1;
	break;
      case LIST:
	tempChain = copyChain(evalLeft->arguments, copyThingOnVoid);
	tempChain = addElement(tempChain, copyThing(tree->child2));
	tempChain2 = copyChain(tempChain, copyThingOnVoid);
	freeChain(tempChain, freeThingOnVoid);
	*concatenatedList = makeList(tempChain2);
	okay = 1;
	break;	
      default:
	okay = 0;
      }
      freeThing(evalLeft);
      return okay;
    }
    return 0;
    break;
  case CONCAT:
    evalLeft = NULL;
    okayLeft = tryEvaluateRecursiveConcatMatcherToList(&evalLeft, tree->child1);
    if (okayLeft) {
      okay = 0;
      evalRight = NULL;
      okayRight = tryEvaluateRecursiveConcatMatcherToList(&evalRight, tree->child2);
      if (okayRight) {
	switch (evalLeft->nodeType) {
	case EMPTYLIST:
	  switch (evalRight->nodeType) {
	  case EMPTYLIST:
	  case LIST:
	  case FINALELLIPTICLIST:
	    *concatenatedList = copyThing(evalRight);
	    okay = 1;
	    break;
	  default:
	    okay = 0;
	    break;
	  }
	  break;
	case LIST:
	  switch (evalRight->nodeType) {
	  case EMPTYLIST:
	    *concatenatedList = copyThing(evalLeft);
	    okay = 1;
	    break;
	  case LIST:
	    tempChain = concatChains(copyChainWithoutReversal(evalLeft->arguments, copyThingOnVoid),
				     copyChainWithoutReversal(evalRight->arguments, copyThingOnVoid));
	    *concatenatedList = makeList(tempChain);
	    okay = 1;
	    break;
	  case FINALELLIPTICLIST:
	    tempChain = concatChains(copyChainWithoutReversal(evalLeft->arguments, copyThingOnVoid),
				     copyChainWithoutReversal(evalRight->arguments, copyThingOnVoid));
	    *concatenatedList = makeFinalEllipticList(tempChain);
	    okay = 1;	    
	    break;
	  default:
	    okay = 0;
	    break;
	  }
	  break;
	default:
	  okay = 0;
	  break;
	}
	freeThing(evalRight);
      }
      freeThing(evalLeft);
      return okay;
    }
    return 0;
    break;
  default:
    return 0;
  }

  return 0;
}

int tryCutPostfixList(chain **associations, node **restList, node *mainList, node *postfix) {
  int lenMainList, lenPostfix, i, k, okay, offset;
  node **mainListArray;
  chain *curr, *possibleMatchList, *possibleRestList;
  node *possibleThingToMatch, *possibleRest, *tempNode, *myMainList;
  chain *myAssociations;
  node *postfixFirstElem, *mainListLastElem;
  chain *mainListCurr;
  int mainListInt, postfixInt;

  if ((mainList->nodeType == FINALELLIPTICLIST) && 
      (postfix->nodeType != FINALELLIPTICLIST)) return 0;
  if ((postfix->nodeType == FINALELLIPTICLIST) && 
      (mainList->nodeType != FINALELLIPTICLIST)) return 0;

  switch (postfix->nodeType) {
  case EMPTYLIST:
    *restList = copyThing(mainList);
    *associations = NULL;
    return 1;
    break;
  case LIST:
    if (mainList->nodeType == LIST) {
      lenMainList = lengthChain(mainList->arguments);
      lenPostfix = lengthChain(postfix->arguments);
      if (lenMainList >= lenPostfix) {
	mainListArray = (node **) safeCalloc(lenMainList, sizeof(node *));
	for (i=0, curr = mainList->arguments; curr != NULL; curr = curr->next, i++) {
	  mainListArray[i] = (node *) (curr->value);
	}
	possibleMatchList = NULL;
	for (i=lenMainList-1, k=0; (i >= 0) && (k < lenPostfix); i--, k++) {
	  possibleMatchList = addElement(possibleMatchList, copyThing(mainListArray[i]));
	}
	possibleThingToMatch = makeList(possibleMatchList);
	if (lenMainList == lenPostfix) {
	  possibleRest = makeEmptyList();
	} else {
	  possibleRestList = NULL;
	  for (i=lenMainList-lenPostfix-1;i>=0;i--) {
	    possibleRestList = addElement(possibleRestList, copyThing(mainListArray[i]));
	  }
	  possibleRest = makeList(possibleRestList);
	}
	free(mainListArray);

	okay = tryMatch(associations, possibleThingToMatch, postfix); 
	freeThing(possibleThingToMatch);
	if (okay) {
	  *restList = possibleRest;
	} else {
	  freeThing(possibleRest);
	}
	return okay;
      } else {
	return 0;
      }
    } else {
      return 0;
    }
    break;
  case FINALELLIPTICLIST:
    if (mainList->nodeType == FINALELLIPTICLIST) {
      lenMainList = lengthChain(mainList->arguments);
      possibleRest = makeEmptyList();
      myMainList = copyThing(mainList);
      offset = 1;
      postfixFirstElem = (node *) (((chain *) (postfix->arguments))->value);
      for (mainListCurr = mainList->arguments;
	   mainListCurr->next != NULL;
	   mainListCurr = mainListCurr->next);
      mainListLastElem = (node *) (mainListCurr->value);
      if (isIntegerElement(&mainListInt, mainListLastElem) &&
	  isIntegerElement(&postfixInt, postfixFirstElem) &&
	  (postfixInt > mainListInt)) {
	offset = postfixInt - mainListInt + 2;
      }
      okay = 0;
      for (i=1; i<=lenMainList+offset; i++) {
	myAssociations = NULL;
	okay = tryMatch(&myAssociations, myMainList, postfix);
	if (okay) {
	  *associations = myAssociations;
	  *restList = copyThing(possibleRest);
	  break;
	} else {
	  if (myAssociations != NULL) freeChain(myAssociations, freeEntryOnVoid);
	  tempNode = makeAppend(possibleRest, makeHead(copyThing(myMainList)));
	  possibleRest = evaluateThing(tempNode);
	  freeThing(tempNode);
	  tempNode = makeTail(myMainList);
	  myMainList = evaluateThing(tempNode);
	  freeThing(tempNode);
	}
      }
      freeThing(possibleRest);
      freeThing(myMainList);
      return okay;
    } else {
      return 0;
    }
    break;
  default:
    return 0;
    break;
  }
  
  return 0;
}

int tryCutPrefixList(chain **associations, node **restList, node *mainList, node *prefix) {
  node *myRestList, *possibleMatchingPrefix, *tempNode;
  int lenPrefix, i, okay;

  switch (prefix->nodeType) {
  case EMPTYLIST:
    *restList = copyThing(mainList);
    *associations = NULL;
    return 1;
    break;
  case LIST:
    switch (mainList->nodeType) {
    case LIST:
    case FINALELLIPTICLIST:
      lenPrefix = lengthChain(prefix->arguments);
      possibleMatchingPrefix = makeEmptyList();
      myRestList = copyThing(mainList);
      for (i=0; i<lenPrefix; i++) {
	  tempNode = makeAppend(possibleMatchingPrefix, makeHead(copyThing(myRestList)));
	  possibleMatchingPrefix = evaluateThing(tempNode);
	  freeThing(tempNode);
	  tempNode = makeTail(myRestList);
	  myRestList = evaluateThing(tempNode);
	  freeThing(tempNode);
      }
      okay = tryMatch(associations, possibleMatchingPrefix, prefix);
      if (okay) {
	*restList = myRestList;
      } else {
	freeThing(myRestList);
      }
      freeThing(possibleMatchingPrefix);
      return okay;
      break;
    default:
      return 0;
      break;
    }
    return 0;
    break;
  default:
    return 0;
  }

  return 0;
}

int tryMatchConcatOnList(chain **associations, node *thingToMatch, node *possibleMatcher) {
  int okayFullEvaluate, okay, okayLeft, okayRight;
  node *listFullEvaluate, *leftEval, *rightEval;
  chain *myAssociations;
  node *restList;
  chain *cutAssociations, *restAssociations;

  myAssociations = NULL;

  listFullEvaluate = NULL;
  okayFullEvaluate = tryEvaluateRecursiveConcatMatcherToList(&listFullEvaluate, possibleMatcher); 
  if (okayFullEvaluate) {
    myAssociations = NULL;
    okay = tryMatch(&myAssociations, thingToMatch, listFullEvaluate);
    freeThing(listFullEvaluate);
    if (okay) {
      *associations = myAssociations;
    } else {
      if (myAssociations != NULL) freeChain(myAssociations, freeEntryOnVoid);
    }  
    return okay;    
  }

  if (possibleMatcher->nodeType != CONCAT) return 0;

  leftEval = NULL;
  rightEval = NULL;
  okayLeft = tryEvaluateRecursiveConcatMatcherToList(&leftEval, possibleMatcher->child1);
  okayRight = tryEvaluateRecursiveConcatMatcherToList(&rightEval, possibleMatcher->child2);
  
  if (okayLeft || okayRight) {
    myAssociations = NULL;
    okay = 1;
    
    if (okayLeft && (leftEval->nodeType == FINALELLIPTICLIST)) okay = 0;
    
    if (okay) {
      if (okayRight) {
	cutAssociations = NULL;
	restList = NULL;
	okay = tryCutPostfixList(&cutAssociations, &restList, thingToMatch, rightEval);
	if (okay) {
	  restAssociations = NULL;
	  okay = tryMatch(&restAssociations,restList,possibleMatcher->child1);
	  if (okay) {
	    myAssociations = NULL;
	    okay = tryCombineAssociations(&myAssociations, restAssociations, cutAssociations);
	    if (restAssociations != NULL) freeChain(restAssociations, freeEntryOnVoid);
	  }
	  if (cutAssociations != NULL) freeChain(cutAssociations, freeEntryOnVoid);
	  freeThing(restList);
	}
      } else {
	cutAssociations = NULL;
	restList = NULL;
	okay = tryCutPrefixList(&cutAssociations, &restList, thingToMatch, leftEval);
	if (okay) {
	  restAssociations = NULL;
	  okay = tryMatch(&restAssociations,restList,possibleMatcher->child2);
	  if (okay) {
	    myAssociations = NULL;
	    okay = tryCombineAssociations(&myAssociations, restAssociations, cutAssociations);
	    if (restAssociations != NULL) freeChain(restAssociations, freeEntryOnVoid);
	  }
	  if (cutAssociations != NULL) freeChain(cutAssociations, freeEntryOnVoid);
	  freeThing(restList);
	}
      }
    }

    if (okayLeft) freeThing(leftEval);
    if (okayRight) freeThing(rightEval);
    if (okay) {
      *associations = myAssociations;
    } else {
      if (myAssociations != NULL) freeChain(myAssociations, freeEntryOnVoid);
    }  
    return okay;        
  }

  return 0;
}

int tryMatchStructure(chain **associations, node *thingToMatch, node *possibleMatcher) {
  int okay, found, okayRecurse, okayCombine;
  chain *myAssociations, *currPossibleMatcher, *currThingToMatch, *recurseAssoc, *myCombinedAssoc;
  char *currPossibleMatcherName, *currThingToMatchName;
  node *currPossibleMatcherMatcher, *currThingToMatchThing;

  if (thingToMatch->nodeType != STRUCTURE) return 0;
  if (possibleMatcher->nodeType != STRUCTURE) return 0;

  okay = 1;
  myAssociations = NULL;

  for (currPossibleMatcher=possibleMatcher->arguments;
       currPossibleMatcher != NULL;
       currPossibleMatcher=currPossibleMatcher->next) {
    currPossibleMatcherName = (char *) (((entry *) (currPossibleMatcher->value))->name);
    currPossibleMatcherMatcher = (node *) (((entry *) (currPossibleMatcher->value))->value);
    for (found = 0, currThingToMatch=thingToMatch->arguments;
	 (!found) && (currThingToMatch != NULL);
	 currThingToMatch=currThingToMatch->next) {
      currThingToMatchName = (char *) (((entry *) (currThingToMatch->value))->name);
      currThingToMatchThing = (node *) (((entry *) (currThingToMatch->value))->value);
      if (!strcmp(currThingToMatchName,currPossibleMatcherName)) found = 1;
    }
    if (found) {
      recurseAssoc = NULL;
      okayRecurse = tryMatch(&recurseAssoc, currThingToMatchThing, currPossibleMatcherMatcher);
      if (okayRecurse) {
	myCombinedAssoc = NULL;
	okayCombine = tryCombineAssociations(&myCombinedAssoc, recurseAssoc, myAssociations);
	if (okayCombine) {
	  if (myAssociations != NULL) freeChain(myAssociations, freeEntryOnVoid);
	  myAssociations = myCombinedAssoc;
	} else {
	  okay = 0;
	}
	if (recurseAssoc != NULL) freeChain(recurseAssoc, freeEntryOnVoid);
      } else {
	okay = 0;
      }
    } else {
      okay = 0;
    }
    if (!okay) break;
  }
  
  if (okay) {
    *associations = myAssociations;
  } else {
    if (myAssociations != NULL) freeChain(myAssociations, freeEntryOnVoid);
  }  
  return okay;
}

int tryMatchInner(chain **associations, node *thingToMatch, node *possibleMatcher) {

  /* Default case: a match for everything, no association to perform */
  if (possibleMatcher->nodeType == DEFAULT) return 1;

  /* Base symbols: a match is obtained if the thing to match is equal
     to the matcher */
  if (isCorrectlyTypedBaseSymbol(possibleMatcher)) {
    if (isEqualThing(thingToMatch, possibleMatcher)) {
      return 1;
    }
    return 0;
  }

  /* Extended pure trees (functions with possible variables to bind to): 
     run specialized code if the thing to match is a pure tree (a function 
     without variables)
  */
  if (isExtendedPureTree(possibleMatcher)) {
    if (!isPureTree(thingToMatch) && !(possibleMatcher->nodeType == TABLEACCESS)) return 0;
    return tryMatchExtendedPureTree(associations, thingToMatch, possibleMatcher);
  }

  /* Match ranges */
  if (possibleMatcher->nodeType == RANGE) {
    if (!isRange(thingToMatch)) return 0;
    return tryMatchRange(associations, *(thingToMatch->child1->value), *(thingToMatch->child2->value), possibleMatcher);
  }

  /* Match lists */
  if (isMatchableList(possibleMatcher)) {
    if (!(isEmptyList(thingToMatch) ||
	  isPureList(thingToMatch) ||
	  isPureFinalEllipticList(thingToMatch))) return 0;
    return tryMatchList(associations, thingToMatch, possibleMatcher);
  }

  /* Match the prepend operator .: */
  if (isMatchablePrepend(possibleMatcher)) {
    if (!(isPureList(thingToMatch) || isPureFinalEllipticList(thingToMatch))) return 0;
    return tryMatchPrepend(associations, thingToMatch, possibleMatcher);
  }

  /* Match the append operator :. */
  if (isMatchableAppend(possibleMatcher)) {
    if (!isPureList(thingToMatch)) return 0;
    return tryMatchAppend(associations, thingToMatch, possibleMatcher);
  }

  /* Match the concat operator @ for strings and lists */
  if (isMatchableConcat(possibleMatcher)) {
    if (!(isPureList(thingToMatch) || 
	  isPureFinalEllipticList(thingToMatch) ||
	  isEmptyList(thingToMatch) ||
	  isString(thingToMatch))) return 0;
    if (isString(thingToMatch)) {
      return tryMatchConcatOnString(associations, thingToMatch->string, possibleMatcher);
    } else {
      return tryMatchConcatOnList(associations, thingToMatch, possibleMatcher);
    }
  }

  /* Match literate structures */
  if (isMatchableStructure(possibleMatcher)) {
    if (!isStructure(thingToMatch)) return 0;
    if (associationContainsDoubleEntries(thingToMatch->arguments)) return 0;
    return tryMatchStructure(associations, thingToMatch, possibleMatcher);
  }

  return 0;
}



int tryMatch(chain **associations, node *thingToMatch, node *possibleMatcher) {
  int okay;
  chain *myAssociations = NULL;

  okay = tryMatchInner(&myAssociations, thingToMatch, possibleMatcher);

  if (okay) {
    *associations = myAssociations;
  } else {
    if (myAssociations != NULL) {
      freeChain(myAssociations, freeEntryOnVoid);
    }
  }

  return okay;
}
