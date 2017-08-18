/*

Copyright 2006-2011 by

Laboratoire de l'Informatique du Parallelisme,
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668,

Laboratoire d'Informatique de Paris 6, equipe PEQUAN,
UPMC Universite Paris 06 - CNRS - UMR 7606 - LIP6, Paris, France,

and by

Centre de recherche INRIA Sophia-Antipolis Mediterranee, equipe APICS,
Sophia Antipolis, France.

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

#include <gmp.h>
#include <mpfr.h>
#include "mpfi-compat.h"
#include <stdio.h> /* fprintf, fopen, fclose, */
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "expression.h"
#include "double.h"
#include "general.h"
#include "infnorm.h"


typedef union {
  int32_t i[2]; 
  double d;
} db_number;

typedef union {
  int32_t i; 
  float f;
} fl_number;



int round_to_format(mpfr_t rop, mpfr_t op, int prec, mp_rnd_t mode) {
  mpfr_t res;
  int round_dir;

  mpfr_init2(res,(mp_prec_t)prec);
  
  round_dir = mpfr_set(res,op, mode);
  if (mpfr_set(rop, res, GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: an undesired rounding occurred on invoking round_to_format.\n");
      printMessage(1,"Try to increase the working precision.\n");
    }
  }

  mpfr_clear(res);
  return round_dir;
}



int mpfr_round_to_double(mpfr_t rop, mpfr_t op) {
  double d;
  int res;

  d = mpfr_get_d(op,GMP_RNDN);
  if (mpfr_set_d(rop,d,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the double precision rounding operator.\n");
      printMessage(1,"Try to increase the working precision.\n");
    }
  }
  
  res = mpfr_cmp(rop,op);

  return res;
}

int mpfr_round_to_double_mode(mpfr_t rop, mpfr_t op, mp_rnd_t mode) {
  double d;
  int res;

  d = mpfr_get_d(op,mode);
  if (mpfr_set_d(rop,d,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the double precision rounding operator.\n");
      printMessage(1,"Try to increase the working precision.\n");
    }
  }
  
  res = mpfr_cmp(rop,op);

  return res;
}


int mpfr_round_to_prec(mpfr_t rop, mpfr_t op, mp_prec_t prec) {
  mp_prec_t p;
  mpfr_t temp;
  int res;

  if (prec < 6) p = 6; else p = prec;
  mpfr_init2(temp,p);
  mpfr_set(temp,op,GMP_RNDN);
  if (mpfr_set(rop,temp,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the double precision rounding operator.\n");
      printMessage(1,"Try to increase the working precision.\n");
    }
  }
  mpfr_clear(temp);
  
  res = mpfr_cmp(rop,op);

  return res;
}


int mpfr_round_to_single(mpfr_t rop, mpfr_t x) {
  int res;
  double d, dd;
  mpfr_t temp, tempRound, temp2;
  fl_number xfl;
  float xfloat;

  mpfr_init2(temp,mpfr_get_prec(x));
  mpfr_init2(temp2,mpfr_get_prec(x));
  mpfr_init2(tempRound,24);

  mpfr_set(tempRound,x,GMP_RNDN);

  d = mpfr_get_d(tempRound,GMP_RNDN);
  xfloat = d;
  dd = xfloat;
  if (mpfr_set_d(temp,dd,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: rounding occurred unexpectedly on reconverting a double value.\n");
    }
  }
  
  res = mpfr_cmp(temp,x);
  if (res && mpfr_number_p(x) && mpfr_number_p(temp)) {
    xfl.f = xfloat;
    if (xfl.i == 0x80000000U) {
      xfl.i = 0x80000001U;
    } else {
      if (res < 0) {
        if (xfl.i >= 0) {
          xfl.i++;
        } else {
          xfl.i--;
        }
      } else {
        if (xfl.i < 0) {
          xfl.i--;
        } else {
          xfl.i++;
        }
      }
    }
    dd = xfl.f;
    if (mpfr_set_d(temp2,dd,GMP_RNDN) != 0) {
      if (!noRoundingWarnings) {
        printMessage(1,"Warning: rounding occurred unexpectedly on reconverting a double value.\n");
      }
    }
    mpfr_sub(temp,temp,x,GMP_RNDN);
    mpfr_sub(temp2,temp2,x,GMP_RNDN);
    if (mpfr_cmpabs(temp2,temp) < 0) {
      xfloat = dd;
      if (mpfr_set_d(temp,dd,GMP_RNDN) != 0) {
        if (!noRoundingWarnings) {
          printMessage(1,"Warning: rounding occurred unexpectedly on reconverting a double value.\n");
        }
      }
    }
  }

  dd = xfloat;

  if (mpfr_set_d(rop,dd,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the single rounding operator.\n");
      printMessage(1,"Try to increase the working precision.\n");
    }
  }

  res = mpfr_cmp(rop,x);

  mpfr_clear(temp);
  mpfr_clear(temp2);
  mpfr_clear(tempRound);
  return res;
}

int mpfr_round_to_single_mode(mpfr_t rop, mpfr_t x, mp_rnd_t mode) {
  int res;
  double d, dd;
  mpfr_t temp, tempRound, temp2;
  fl_number xfl;
  float xfloat;

  mpfr_init2(temp,mpfr_get_prec(x));
  mpfr_init2(temp2,mpfr_get_prec(x));
  mpfr_init2(tempRound,24);

  mpfr_set(tempRound,x,GMP_RNDN);

  d = mpfr_get_d(tempRound,GMP_RNDN);
  xfloat = d;
  dd = xfloat;
  if (mpfr_set_d(temp,dd,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: rounding occurred unexpectedly on reconverting a double value.\n");
    }
  }
  
  res = mpfr_cmp(temp,x);
  if ((res != 0) && mpfr_number_p(x) && mpfr_number_p(temp)) {
    xfl.f = xfloat;
    if (xfl.i == 0x80000000U) {
      xfl.i = 0x80000001U;
    } else {
      if (res < 0) {
        if (xfl.i >= 0) {
          xfl.i++;
        } else {
          xfl.i--;
        }
      } else {
        if (xfl.i < 0) {
          xfl.i--;
        } else {
          xfl.i++;
        }
      }
    }
    dd = xfl.f;
    if (mpfr_set_d(temp2,dd,GMP_RNDN) != 0) {
      if (!noRoundingWarnings) {
        printMessage(1,"Warning: rounding occurred unexpectedly on reconverting a double value.\n");
      }
    }
    mpfr_sub(temp,temp,x,GMP_RNDN);
    mpfr_sub(temp2,temp2,x,GMP_RNDN);
    if (mpfr_cmpabs(temp2,temp) < 0) {
      xfloat = dd;
      if (mpfr_set_d(temp,dd,GMP_RNDN) != 0) {
        if (!noRoundingWarnings) {
          printMessage(1,"Warning: rounding occurred unexpectedly on reconverting a double value.\n");
        }
      }
    }
  }

  dd = xfloat;
  if (mpfr_set_d(temp,dd,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: rounding occurred unexpectedly on reconverting a double value.\n");
    }
  }

  // Now, temp is equal to the single rounding of x under round to nearest
  //
  res = mpfr_cmp(temp,x);
  if ((mode != GMP_RNDN) && 
      mpfr_number_p(x) && 
      mpfr_number_p(temp) && 
      (res != 0))  {
    // Here, we have an inexact single rounding to nearest that is a number
    // but we must produce a directed rounding
    if ((res < 0) && 
	((mode == GMP_RNDU) || 
	 ((mode == GMP_RNDZ) && (mpfr_sgn(x) < 0)))) {
      // The rounded value is too small
      xfl.f = xfloat;
      if (xfl.i >= 0) {
	xfl.i++;
      } else {
	xfl.i--;
      }
      dd = xfl.f;
      if (mpfr_set_d(temp,dd,GMP_RNDN) != 0) {
	if (!noRoundingWarnings) {
	  printMessage(1,"Warning: rounding occurred unexpectedly on reconverting a double value.\n");
	}
      }
    } else {
      if ((res > 0) && 
	  ((mode == GMP_RNDD) || 
	   ((mode == GMP_RNDZ) && (mpfr_sgn(x) > 0)))) {
	// The rounded value is too great
	xfl.f = xfloat;
	if (xfl.i >= 0) {
	  xfl.i--;
	} else {
	  xfl.i++;
	}
	dd = xfl.f;
	if (mpfr_set_d(temp,dd,GMP_RNDN) != 0) {
	  if (!noRoundingWarnings) {
	    printMessage(1,"Warning: rounding occurred unexpectedly on reconverting a double value.\n");
	  }
	}
      } 
    }
  }

  if (mpfr_set(rop,temp,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the single rounding operator.\n");
      printMessage(1,"Try to increase the working precision.\n");
    }
  }

  res = mpfr_cmp(rop,x);

  mpfr_clear(temp);
  mpfr_clear(temp2);
  mpfr_clear(tempRound);
  return res;
}



int mpfr_round_to_doubledouble(mpfr_t rop, mpfr_t op) {
  double d;
  mpfr_t accu, temp, rest;
  mp_prec_t prec;
  int res;

  prec = mpfr_get_prec(op);
  if (prec < 106) {
    prec = 106;
  }

  mpfr_init2(accu, prec);
  mpfr_init2(temp, prec);
  mpfr_init2(rest, prec);

  d = mpfr_get_d(op,GMP_RNDN);
  if (mpfr_set_d(accu,d,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the double-double rounding operator.\n");
      printMessage(1,"The rounding occurred on recasting to MPFR. This should not occur.\n");
    }
  }
  if (mpfr_sub(rest,op,accu,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the double-double rounding operator.\n");
      printMessage(1,"The rounding occurred on substracting in MPFR. This should not occur.\n");
    }
  }
  d = mpfr_get_d(rest,GMP_RNDN);
  if (mpfr_set_d(temp,d,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the double-double rounding operator.\n");
      printMessage(1,"The rounding occurred on recasting to MPFR. This should not occur.\n");
    }
  }
  if (mpfr_add(accu,accu,temp,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the double-double rounding operator.\n");
      printMessage(1,"The rounding occurred on substracting in MPFR. This should not occur.\n");
    }
  }
  if (mpfr_set(rop,accu,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the double-double rounding operator.\n");
      printMessage(1,"Try to increase the working precision.\n");
    }
  }

  res = mpfr_cmp(rop,op);

  mpfr_clear(accu);
  mpfr_clear(temp);
  mpfr_clear(rest);
  return res;
}

int mpfr_round_to_doubledouble_mode(mpfr_t rop, mpfr_t op, mp_rnd_t mode) {
  double d;
  mpfr_t accu, temp, rest;
  mp_prec_t prec;
  int res;

  prec = mpfr_get_prec(op);
  if (prec < 106) {
    prec = 106;
  }

  mpfr_init2(accu, prec);
  mpfr_init2(temp, prec);
  mpfr_init2(rest, prec);

  d = mpfr_get_d(op,GMP_RNDN);
  if (mpfr_set_d(accu,d,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the double-double rounding operator.\n");
      printMessage(1,"The rounding occurred on recasting to MPFR. This should not occur.\n");
    }
  }
  if (mpfr_sub(rest,op,accu,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the double-double rounding operator.\n");
      printMessage(1,"The rounding occurred on substracting in MPFR. This should not occur.\n");
    }
  }
  d = mpfr_get_d(rest,mode);
  if (mpfr_set_d(temp,d,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the double-double rounding operator.\n");
      printMessage(1,"The rounding occurred on recasting to MPFR. This should not occur.\n");
    }
  }
  if (mpfr_add(accu,accu,temp,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the double-double rounding operator.\n");
      printMessage(1,"The rounding occurred on substracting in MPFR. This should not occur.\n");
    }
  }
  if (mpfr_set(rop,accu,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the double-double rounding operator.\n");
      printMessage(1,"Try to increase the working precision.\n");
    }
  }

  res = mpfr_cmp(rop,op);

  mpfr_clear(accu);
  mpfr_clear(temp);
  mpfr_clear(rest);
  return res;
}


int mpfr_round_to_tripledouble(mpfr_t rop, mpfr_t op) {
  double d;
  mpfr_t accu, temp, rest;
  mp_prec_t prec;
  int res;

  prec = mpfr_get_prec(op);
  if (prec < 159) {
    prec = 159;
  }

  mpfr_init2(accu, prec);
  mpfr_init2(temp, prec);
  mpfr_init2(rest, prec);

  d = mpfr_get_d(op,GMP_RNDN);
  if (mpfr_set_d(accu,d,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the triple-double rounding operator.\n");
      printMessage(1,"The rounding occurred on recasting to MPFR. This should not occur.\n");
    }
  }
  if (mpfr_sub(rest,op,accu,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the triple-double rounding operator.\n");
      printMessage(1,"The rounding occurred on substracting in MPFR. This should not occur.\n");
    }
  }
  d = mpfr_get_d(rest,GMP_RNDN);
  if (mpfr_set_d(temp,d,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the triple-double rounding operator.\n");
      printMessage(1,"The rounding occurred on recasting to MPFR. This should not occur.\n");
    }
  }
  if (mpfr_add(accu,accu,temp,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the triple-double rounding operator.\n");
      printMessage(1,"The rounding occurred on substracting in MPFR. This should not occur.\n");
    }
  }
  if (mpfr_sub(rest,op,accu,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the triple-double rounding operator.\n");
      printMessage(1,"The rounding occurred on substracting in MPFR. This should not occur.\n");
    }
  }
  d = mpfr_get_d(rest,GMP_RNDN);
  if (mpfr_set_d(temp,d,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the triple-double rounding operator.\n");
      printMessage(1,"The rounding occurred on recasting to MPFR. This should not occur.\n");
    }
  }
  if (mpfr_add(accu,accu,temp,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the triple-double rounding operator.\n");
      printMessage(1,"The rounding occurred on substracting in MPFR. This should not occur.\n");
    }
  }
  if (mpfr_set(rop,accu,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the triple-double rounding operator.\n");
      printMessage(1,"Try to increase the working precision.\n");
    }
  }

  res = mpfr_cmp(rop,op);

  mpfr_clear(accu);
  mpfr_clear(temp);
  mpfr_clear(rest);

  return res;
}

int mpfr_round_to_tripledouble_mode(mpfr_t rop, mpfr_t op, mp_rnd_t mode) {
  double d;
  mpfr_t accu, temp, rest;
  mp_prec_t prec;
  int res;

  prec = mpfr_get_prec(op);
  if (prec < 159) {
    prec = 159;
  }

  mpfr_init2(accu, prec);
  mpfr_init2(temp, prec);
  mpfr_init2(rest, prec);

  d = mpfr_get_d(op,GMP_RNDN);
  if (mpfr_set_d(accu,d,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the triple-double rounding operator.\n");
      printMessage(1,"The rounding occurred on recasting to MPFR. This should not occur.\n");
    }
  }
  if (mpfr_sub(rest,op,accu,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the triple-double rounding operator.\n");
      printMessage(1,"The rounding occurred on substracting in MPFR. This should not occur.\n");
    }
  }
  d = mpfr_get_d(rest,GMP_RNDN);
  if (mpfr_set_d(temp,d,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the triple-double rounding operator.\n");
      printMessage(1,"The rounding occurred on recasting to MPFR. This should not occur.\n");
    }
  }
  if (mpfr_add(accu,accu,temp,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the triple-double rounding operator.\n");
      printMessage(1,"The rounding occurred on substracting in MPFR. This should not occur.\n");
    }
  }
  if (mpfr_sub(rest,op,accu,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the triple-double rounding operator.\n");
      printMessage(1,"The rounding occurred on substracting in MPFR. This should not occur.\n");
    }
  }
  d = mpfr_get_d(rest,mode);
  if (mpfr_set_d(temp,d,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the triple-double rounding operator.\n");
      printMessage(1,"The rounding occurred on recasting to MPFR. This should not occur.\n");
    }
  }
  if (mpfr_add(accu,accu,temp,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the triple-double rounding operator.\n");
      printMessage(1,"The rounding occurred on substracting in MPFR. This should not occur.\n");
    }
  }
  if (mpfr_set(rop,accu,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the triple-double rounding operator.\n");
      printMessage(1,"Try to increase the working precision.\n");
    }
  }

  res = mpfr_cmp(rop,op);

  mpfr_clear(accu);
  mpfr_clear(temp);
  mpfr_clear(rest);

  return res;
}


int printDoubleInHexa(mpfr_t x) {
  int res;
  double d;
  mpfr_t temp;
  db_number xdb, endianessdb;

  mpfr_init2(temp,mpfr_get_prec(x));
  
  d = mpfr_get_d(x,GMP_RNDN);
  if (mpfr_set_d(temp,d,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: rounding occurred unexpectedly on reconverting a double value.\n");
    }
  }
  
  res = mpfr_cmp(temp,x);

  if (res) {
    if (!noRoundingWarnings) {
      if (res < 0) 
        printMessage(1,"Warning: rounding down occurred before printing a value as a double.\n");
      else 
        printMessage(1,"Warning: rounding up occurred before printing a value as a double.\n");
    }
  }

  xdb.d = d;
  endianessdb.d = 1.0;
  if ((endianessdb.i[1] == 0x3ff00000) && (endianessdb.i[0] == 0)) {
    sollyaPrintf("0x%08x%08x\n",xdb.i[1],xdb.i[0]);
  } else {
    if ((endianessdb.i[0] == 0x3ff00000) && (endianessdb.i[1] == 0)) {
      sollyaPrintf("0x%08x%08x\n",xdb.i[0],xdb.i[1]);
    } else {
      printMessage(1,"Warning: could not figure out the endianess of the system. Will print 1.0 instead of the value.\n");
      sollyaPrintf("0x3ff0000000000000\n");
    }
  }

  mpfr_clear(temp);
  return res;
}

int printSimpleInHexa(mpfr_t x) {
  int res;
  double d, dd;
  mpfr_t temp, tempRound, temp2;
  fl_number xfl;
  float xfloat;

  mpfr_init2(temp,mpfr_get_prec(x));
  mpfr_init2(temp2,mpfr_get_prec(x));
  mpfr_init2(tempRound,24);

  mpfr_set(tempRound,x,GMP_RNDN);

  d = mpfr_get_d(tempRound,GMP_RNDN);
  xfloat = d;
  dd = xfloat;
  if (mpfr_set_d(temp,dd,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: rounding occurred unexpectedly on reconverting a double value.\n");
    }
  }
  
  res = mpfr_cmp(temp,x);
  if (res && mpfr_number_p(x) && mpfr_number_p(temp)) {
    xfl.f = xfloat;
    if (xfl.i == 0x80000000U) {
      xfl.i = 0x80000001U;
    } else {
      if (res < 0) {
        if (xfl.i >= 0) {
          xfl.i++;
        } else {
          xfl.i--;
        }
      } else {
        if (xfl.i < 0) {
          xfl.i--;
        } else {
          xfl.i++;
        }
      }
    }
    dd = xfl.f;
    if (mpfr_set_d(temp2,dd,GMP_RNDN) != 0) {
      if (!noRoundingWarnings) {
        printMessage(1,"Warning: rounding occurred unexpectedly on reconverting a double value.\n");
      }
    }
    mpfr_sub(temp,temp,x,GMP_RNDN);
    mpfr_sub(temp2,temp2,x,GMP_RNDN);
    if (mpfr_cmpabs(temp2,temp) < 0) {
      xfloat = dd;
      if (mpfr_set_d(temp,dd,GMP_RNDN) != 0) {
        if (!noRoundingWarnings) {
          printMessage(1,"Warning: rounding occurred unexpectedly on reconverting a double value.\n");
        }
      }
    }
  }

  res = mpfr_cmp(temp,x);

  if (res) {
    if (!noRoundingWarnings) {
      if (res < 0) 
        printMessage(1,"Warning: rounding down occurred before printing a value as a simple.\n");
      else 
        printMessage(1,"Warning: rounding up occurred before printing a value as a simple.\n");
    }
  }

  xfl.f = xfloat;
  sollyaPrintf("0x%08x\n",xfl.i);

  mpfr_clear(temp);
  mpfr_clear(temp2);
  mpfr_clear(tempRound);
  return res;
}


int readHexaDouble(mpfr_t res, char *c) {
  int ret, i;
  int32_t msb, lsb;
  double x;
  char msbstr[9], lsbstr[9];
  db_number xdb, endianessdb;
  
  x = 1.0;
  c += 2; /* Skip over "0x" */
  for (i=0;i<9;i++) {
    msbstr[i] = '\0';
    lsbstr[i] = '\0';
  }
  for (i=0;i<8;i++) {
    msbstr[i] = *c;
    c++;
  }
  for (i=0;i<8;i++) {
    lsbstr[i] = *c;
    c++;
  }

  msb = strtoll(msbstr,NULL,16);
  lsb = strtoll(lsbstr,NULL,16);

  endianessdb.d = 1.0;
  if ((endianessdb.i[1] == 0x3ff00000) && (endianessdb.i[0] == 0)) {
    xdb.i[1] = msb;
    xdb.i[0] = lsb;
  } else {
    if ((endianessdb.i[0] == 0x3ff00000) && (endianessdb.i[1] == 0)) {
      xdb.i[0] = msb;
      xdb.i[1] = lsb;
    } else {
      printMessage(1,"Warning: could not figure out the endianess of the system. Will read 1.0 instead of the value.\n");
      xdb.d = 1.0;
    }
  }

  x = xdb.d;

  if ((!(x == x)) && ((msb & 0x00080000) == 0)) {
    printMessage(1,"Warning: a sNaN might have been converted to a qNaN.\n");
  }


  if (mpfr_set_d(res,x,GMP_RNDN) != 0) ret = 0; else ret = 1;
  return ret;
}

int readHexaSimple(mpfr_t res, char *c) {
  int ret, i;
  int32_t msb;
  double x;
  char msbstr[9];
  fl_number xfl;
  float xfloat;
  
  x = 1.0;
  c += 2; /* Skip over "0x" */
  for (i=0;i<9;i++) {
    msbstr[i] = '\0';
  }
  for (i=0;i<8;i++) {
    msbstr[i] = *c;
    c++;
  }
  

  msb = strtoll(msbstr,NULL,16);

  xfl.i = msb;

  xfloat = xfl.f;

  x = xfloat;

  if ((!(x == x)) && ((msb & 0x00400000) == 0)) {
    printMessage(1,"Warning: a sNaN might have been converted to a qNaN.\n");
  }


  if (mpfr_set_d(res,x,GMP_RNDN) != 0) ret = 0; else ret = 1;
  return ret;
}



int readHexa(mpfr_t res, char *c) {
  
  if (strlen(c) == 18) 
    return readHexaDouble(res, c);

  if (strlen(c) == 10) 
    return readHexaSimple(res, c);

  printMessage(1,"Warning: unable to convert the hexadecimal sequence \"%s\" to a constant.\n");
  
  mpfr_set_nan(res);

  return 0;
}


node *roundPolynomialCoefficients(node *poly, chain *formats, mp_prec_t prec) {
  int degree, listLength, i, deg, res, fillUp, k;
  chain *curr, *monomials;
  int *formatsArray, *tempArray;
  node *roundedPoly, *temp;
  node **coefficients;
  mpfr_t *fpcoefficients;
  mpfr_t tempMpfr, dummyX;
  node *subPolyToRound, *subPolyRest, *tempNode;

  degree = getDegree(poly);

  if (degree < 0) {
    printMessage(1,"Warning: the given function is not a polynomial.\n");
    return copyTree(poly);
  }

  listLength = lengthChain(formats);

  if (*((int *) formats->value) == -1) {
    fillUp = 1;
    curr = formats->next;
  } else {
    curr = formats;
    fillUp = 0;
    if (listLength < (degree + 1)) {
      printMessage(1,"Warning: the number of the given formats does not correspond to the degree of the given polynomial.\n");
      monomials = makeIntPtrChainFromTo(0, listLength-1);
      subPolyToRound = getSubpolynomial(poly, monomials, 0, prec);
      freeChain(monomials, freeIntPtr);
      monomials = makeIntPtrChainFromTo(listLength, degree);
      subPolyRest = getSubpolynomial(poly, monomials, 0, prec);
      freeChain(monomials, freeIntPtr);
      tempNode = makeAdd(roundPolynomialCoefficients(subPolyToRound, formats, prec), subPolyRest);
      free_memory(subPolyToRound);
      temp = horner(tempNode);
      free_memory(tempNode);
      return temp;
    }
  }
  
  tempArray = (int *) safeCalloc(degree + 1,sizeof(int));

  i = 0;
  while ((curr != NULL) && (i <= degree)) {
    tempArray[i] = *((int *) curr->value);
    i++;
    curr = curr->next;
  }
  k = i;
  
  formatsArray = (int *) safeCalloc(degree + 1,sizeof(int));

  if (fillUp) {
    for (i=k-1;i>=0;i--) {
      formatsArray[(k-1) - i] = tempArray[i];
    }
    for (i=k;i<=degree;i++) {
      formatsArray[i] = formatsArray[k-1];
    }
  } else {
    for (i=degree;i>=0;i--) {
      formatsArray[degree - i] = tempArray[i];
    }
  }

  free(tempArray);

  getCoefficients(&deg,&coefficients,poly);

  if (deg != degree) {
    printMessage(1,"Warning: an error occurred while extracting the coefficients of the polynomial.\n");
    for (i=0;i<=deg;i++) {
      if (coefficients[i] != NULL) free_memory(coefficients[i]);
    }
    free(coefficients);
    return copyTree(poly);
  }

  fpcoefficients = (mpfr_t *) safeCalloc(degree+1,sizeof(mpfr_t));

  mpfr_init2(tempMpfr,prec > 160 ? prec : 160);

  res = 0;

  for (i=0;i<=degree;i++) {
    if (coefficients[i] != NULL) {
      temp = simplifyTreeErrorfree(coefficients[i]);
      free_memory(coefficients[i]);
      if (temp->nodeType != CONSTANT) {
	if (!noRoundingWarnings) {
	  printMessage(1,"Warning: the %dth coefficient of the given polynomial does not evaluate to a floating-point constant without any rounding.\n",i);
	  printMessage(1,"Will evaluate the coefficient in the current precision in floating-point before rounding to the target format.\n");
	}
	mpfr_init2(fpcoefficients[i],prec);
	mpfr_init2(dummyX, prec);
	mpfr_set_si(dummyX, 1, GMP_RNDN);
	if (!evaluateFaithful(fpcoefficients[i], temp, dummyX, prec)) {
	  if (!noRoundingWarnings) {
	    printMessage(1,"Warning: the evaluation of the %dth coefficient is not faithful.\n",i);
	  }
	  evaluateConstantExpression(fpcoefficients[i], temp, 256 * prec);
	}
	mpfr_clear(dummyX);
	res = 1;
      } else {
	mpfr_init2(fpcoefficients[i],mpfr_get_prec(*(temp->value)));
	mpfr_set(fpcoefficients[i],*(temp->value),GMP_RNDN);
      }
      free_memory(temp);
    } else {
      mpfr_init2(fpcoefficients[i],prec);
      mpfr_set_d(fpcoefficients[i],0.0,GMP_RNDN);
    }
    switch (formatsArray[i]) {
    case 7:
      if (mpfr_round_to_quad(tempMpfr, fpcoefficients[i]) != 0) res = 1;
      break;
    case 6:
      if (mpfr_round_to_halfprecision(tempMpfr, fpcoefficients[i]) != 0) res = 1;
      break;
    case 5:
      if (mpfr_round_to_single(tempMpfr, fpcoefficients[i]) != 0) res = 1;
      break;
    case 4:
      if (mpfr_round_to_doubleextended(tempMpfr, fpcoefficients[i]) != 0) res = 1;
      break;
    case 3:
      if (mpfr_round_to_tripledouble(tempMpfr, fpcoefficients[i]) != 0) res = 1;
      break;
    case 2:
      if (mpfr_round_to_doubledouble(tempMpfr, fpcoefficients[i]) != 0) res = 1;
      break;
    case 1:
      if (mpfr_round_to_double(tempMpfr, fpcoefficients[i]) != 0) res = 1;
      break;
    default:
      if (formatsArray[i] > 5) {
	if (mpfr_round_to_prec(tempMpfr, fpcoefficients[i], (mp_prec_t) (formatsArray[i] - 6)) != 0) 
	  res = 1;
      } else {
	printMessage(1,"Warning: unknown expansion format found. No rounding will be performed.\n");
	mpfr_set(tempMpfr,fpcoefficients[i],GMP_RNDN);
      }
    }
    if (mpfr_set(fpcoefficients[i],tempMpfr,GMP_RNDN) != 0) {
      if (!noRoundingWarnings) {
	printMessage(1,"Warning: double rounding occurred on internal handling of a coefficient.\nTry to increase the precision.\n");
      }
    }
  }

  free(coefficients);

  if (res) {
    if (!noRoundingWarnings) {
      printMessage(2,"Information: at least one coefficient has been rounded.\n");
    }
  } else {
    if (!noRoundingWarnings) {
      printMessage(2,"Information: there has not been any rounding of the coefficients.\n");
    }
  }

  roundedPoly = makePolynomial(fpcoefficients, degree);

  for (i=0;i<=degree;i++) mpfr_clear(fpcoefficients[i]);
  free(fpcoefficients);
  free(formatsArray);
  mpfr_clear(tempMpfr);
  return roundedPoly;
}

int printDoubleExpansion(mpfr_t x) {
  double d;
  mpfr_t temp, rest;
  db_number xdb, endianessdb;
  int noBrackets, roundingOccured;

  mpfr_init2(temp,mpfr_get_prec(x));
  mpfr_init2(rest,mpfr_get_prec(x));

  mpfr_set(rest,x,GMP_RNDN);

  roundingOccured = 0;
  noBrackets = 0;
  d = mpfr_get_d(x,GMP_RNDN);
  if (mpfr_set_d(temp,d,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: rounding occurred unexpectedly on reconverting a double value.\n");
    }
    roundingOccured = 1;
  }
  
  if (!mpfr_number_p(temp)) {
    printMessage(1,"Warning: will not print a number that is not real as a double expansion.\n");
    printValue(&temp);
    mpfr_clear(temp);
    mpfr_clear(rest);
    return 1;
  }

  if (mpfr_cmp(temp,x) == 0) 
    noBrackets = 1;

  if (!noBrackets) 
    sollyaPrintf("(");

  do {
    d = mpfr_get_d(rest,GMP_RNDN);
    if (mpfr_set_d(temp,d,GMP_RNDN) != 0) {
      if (!noRoundingWarnings) {
	printMessage(1,"Warning: rounding occurred unexpectedly on reconverting a double value.\n");
      }
      roundingOccured = 1;
    }
    
    xdb.d = d;
    endianessdb.d = 1.0;
    if ((endianessdb.i[1] == 0x3ff00000) && (endianessdb.i[0] == 0)) {
      sollyaPrintf("0x%08x%08x",xdb.i[1],xdb.i[0]);
    } else {
      if ((endianessdb.i[0] == 0x3ff00000) && (endianessdb.i[1] == 0)) {
	sollyaPrintf("0x%08x%08x",xdb.i[0],xdb.i[1]);
      } else {
	printMessage(1,"Warning: could not figure out the endianess of the system. Will print 1.0 instead of the value.\n");
	sollyaPrintf("0x3ff0000000000000\n");
	roundingOccured = 1;
      }
    }

    if (mpfr_sub(rest,rest,temp,GMP_RNDN) != 0) {
      if (!noRoundingWarnings) {
	printMessage(1,"Warning: rounding occurred unexpectedly on subtracting.\n");
      }
      roundingOccured = 1;
    }
    
    if ((d != 0.0) && (!mpfr_zero_p(rest))) {
      sollyaPrintf(" + ");
    }

  } while ((d != 0.0) && (!mpfr_zero_p(rest)));

  if (!noBrackets) 
    sollyaPrintf(")");


  if (!mpfr_zero_p(rest)) {
    if (!noRoundingWarnings) {
      printMessage(1,"\nWarning: the expansion is not complete because of the limited exponent range of double precision.");
    }
    roundingOccured = 1;
  }

  mpfr_clear(temp);
  mpfr_clear(rest);
  return roundingOccured;
}

int printPolynomialAsDoubleExpansion(node *poly, mp_prec_t prec) {
  int degree, roundingOccured, i, k, l;
  node **coefficients;
  node *tempNode, *simplifiedTreeSafe, *simplifiedTree, *myTree;
  mpfr_t tempValue, tempValue2;

  roundingOccured = 0;

  tempNode = horner(poly);
  simplifiedTreeSafe = simplifyTreeErrorfree(tempNode);
  free_memory(tempNode);
  simplifiedTree = simplifyTree(simplifiedTreeSafe);

  if (!isPolynomial(simplifiedTreeSafe)) {
    if (!isPolynomial(simplifiedTree)) {
      printMessage(1,"Warning: the given expression is not a polynomial.");
      free_memory(simplifiedTree);
      free_memory(simplifiedTreeSafe);
      return -1;
    } else {
      if (!noRoundingWarnings) {
	printMessage(1,"Warning: rounding occurred while simplifying to a polynomial form.\n");
      }
      roundingOccured = 1;
      myTree = simplifiedTree;
      free_memory(simplifiedTreeSafe);
    }
  } else {
    myTree = simplifiedTreeSafe;
    free_memory(simplifiedTree);
  }

  getCoefficients(&degree, &coefficients, myTree);

  mpfr_init2(tempValue,prec);
  mpfr_init2(tempValue2,prec);
  mpfr_set_d(tempValue2,1.0,GMP_RNDN);

  k = 0; l = 0;
  for (i=0;i<=degree;i++) {
    if (coefficients[i] != NULL) {
      if (k > 0) {
	if (k == 1) {
	  sollyaPrintf("%s * ",variablename);
	} else {
	  sollyaPrintf("%s^%d * ",variablename,k);
	}
      }

      if ((i != degree) && (i != 0)) {
	sollyaPrintf("(");
	l++;
      }

      tempNode = simplifyTreeErrorfree(coefficients[i]);
      if (tempNode->nodeType == CONSTANT) {
	roundingOccured |=  printDoubleExpansion(*(tempNode->value));
      } else {
	if (!isConstant(tempNode)) {
	  printMessage(1,"Error: a coefficient of a polynomial is not constant.\n");
	  recoverFromError();
	}
	if (!evaluateFaithful(tempValue, tempNode, tempValue2, prec)) {
	  if (!noRoundingWarnings) {
	    printMessage(1,"Warning: an evaluation is not faithful.\n");
	  }
	  evaluate(tempValue, tempNode, tempValue2, 256 * prec);
	}
	printDoubleExpansion(tempValue);
	roundingOccured = 1;
      }
      free_memory(tempNode);
      free_memory(coefficients[i]);
      k = 1;

      if (i != degree) {
	sollyaPrintf(" + ");
      }
    } else {
      k++;
    }
  }
  for (i=0;i<l;i++) 
    sollyaPrintf(")");

  free(coefficients);
  mpfr_clear(tempValue);
  mpfr_clear(tempValue2);
  free_memory(myTree);

  return roundingOccured;
}


void mpfr_round_to_format(mpfr_t rop, mpfr_t op, int format) {
  switch (format) {
  case 7:
    mpfr_round_to_quad(rop, op);
    break;
  case 6:
    mpfr_round_to_halfprecision(rop, op);
    break;
  case 5:
    mpfr_round_to_single(rop, op);
    break;
  case 4:
    mpfr_round_to_doubleextended(rop, op);
    break;
  case 3:
    mpfr_round_to_tripledouble(rop, op);
    break;
  case 2:
    mpfr_round_to_doubledouble(rop, op);
    break;
  case 1:
    mpfr_round_to_double(rop, op);
    break;
  default:
    sollyaFprintf(stderr,"Error: mpfr_round_to_format: unknown format type.\n");
    exit(1);
  }
}

int round_to_expansion_format(mpfr_t rop, mpfr_t op, int format, mp_rnd_t mode) {
  int res;
  switch (format) {
  case 7:
    mpfr_round_to_quad_mode(rop, op, mode);
    break;
  case 6:
    mpfr_round_to_halfprecision_mode(rop, op, mode);
    break;
  case 5:
    mpfr_round_to_single_mode(rop, op, mode);
    break;
  case 4:
    mpfr_round_to_doubleextended_mode(rop, op, mode);
    break;
  case 3:
    mpfr_round_to_tripledouble_mode(rop, op, mode);
    break;
  case 2:
    mpfr_round_to_doubledouble_mode(rop, op, mode);
    break;
  case 1:
    mpfr_round_to_double_mode(rop, op, mode);
    break;
  default:
    sollyaFprintf(stderr,"Error: round_to_expansion_format: unknown format type.\n");
    exit(1);
  }

  res = mpfr_cmp(rop,op);

  return res;
}



int mpfr_mant_exp(mpfr_t rop, mp_exp_t *expo, mpfr_t op) {
  mp_exp_t e;
  mp_prec_t p;
  mpfr_t temp;
  int res;

  if (!mpfr_number_p(op)) {
    *expo = 0;
    return mpfr_set(rop,op,GMP_RNDN);
  }

  if (mpfr_zero_p(op)) {
    *expo = 0;
    return mpfr_set(rop,op,GMP_RNDN);
  }

  p = mpfr_get_prec(op);  
  mpfr_init2(temp,p);
  mpfr_set(temp,op,GMP_RNDN);
  
  e = mpfr_get_exp(temp) - p;
  mpfr_set_exp(temp,p);
  
  while (mpfr_integer_p(temp)) {
    mpfr_div_2ui(temp,temp,1,GMP_RNDN);
    e++;
  }
  mpfr_mul_2ui(temp,temp,1,GMP_RNDN);
  e--;
  
  *expo = e;
  res = mpfr_set(rop,temp,GMP_RNDN);

  mpfr_clear(temp);

  return res;
}


int roundRangeCorrectly(mpfr_t rop, mpfr_t a, mpfr_t b) {
  mp_exp_t expoA, expoB;
  int expoDiff;
  mp_prec_t prec, p;
  mpfr_t tempA, tempB;
  int okay;

  if (mpfr_sgn(a) != mpfr_sgn(b)) {
    mpfr_set_nan(rop);
    return 0;
  }
  
  expoA = mpfr_get_exp(a);
  expoB = mpfr_get_exp(b);

  expoDiff = expoA - expoB;
  if (expoDiff < 0) expoDiff = -expoDiff;

  if (expoDiff > 1) {
    mpfr_set_nan(rop);
    return 0;
  }

  prec = mpfr_get_prec(a);
  p = mpfr_get_prec(b);
  if (p > prec) prec = p;

  mpfr_init2(tempA,prec);
  mpfr_init2(tempB,prec);

  okay = 0;
  while (prec >= 3) {
    mpfr_set(tempA,a,GMP_RNDN);
    mpfr_set(tempB,b,GMP_RNDN);
    if (mpfr_cmp(tempA,tempB) == 0) {
      okay = 1;
      break;
    }
    prec--;
    mpfr_set_prec(tempA,prec);
    mpfr_set_prec(tempB,prec);
  }
  if (prec < 12) prec = 12;
  if (okay) {
    mpfr_set_prec(rop,prec);
    mpfr_set(rop,tempA,GMP_RNDN);
  } else{
    mpfr_set_nan(rop);
  }

  mpfr_clear(tempA);
  mpfr_clear(tempB);

  return okay;
}




void continuedFrac(mpq_t q, sollya_mpfi_t x) {
  sollya_mpfi_t xprime;
  mpfr_t a,b;
  mpfr_t m1,m2;
  mp_prec_t t;
  mpq_t res;
  mpz_t u;

  t = sollya_mpfi_get_prec(x);
  sollya_mpfi_init2(xprime,t);
  mpfr_init2(a,t);
  mpfr_init2(b,t);
  mpfr_init2(m1,t);
  mpfr_init2(m2,t);
  mpq_init(res);
  mpz_init(u);

  sollya_mpfi_get_left(a,x);
  sollya_mpfi_get_right(b,x);
  mpfr_floor(m1,a);
  mpfr_floor(m2,b);

  if (mpfr_equal_p(m1,m2) && !mpfr_equal_p(a,m1)) {
    mpfr_get_z(u,m1,GMP_RNDN); //exact
    mpfr_sub(a,a,m1,GMP_RNDD);
    mpfr_sub(b,b,m1,GMP_RNDU);
    sollya_mpfi_interv_fr(xprime,a,b);
    sollya_mpfi_inv(xprime,xprime);
    continuedFrac(res,xprime);
    mpq_inv(res,res);
    mpq_set_num(q,u);
    mpz_set_ui(u,1);
    mpq_set_den(q,u);
    mpq_add(q,q,res);
  }
  else {
    mpfr_add(m1,a,b,GMP_RNDN);
    mpfr_div_2ui(m1,m1,1,GMP_RNDN);
    mpfr_get_z(u,m1,GMP_RNDN);
    mpq_set_num(q,u);
    mpz_set_ui(u,1);
    mpq_set_den(q,u);
  }
  
  sollya_mpfi_clear(xprime);
  mpfr_clear(a);
  mpfr_clear(b);
  mpfr_clear(m1);
  mpfr_clear(m2);
  mpq_clear(res);
  mpz_clear(u);
  return;
}

node *rationalApprox(mpfr_t x, unsigned int n) {
  mpq_t q;
  mpz_t u;
  sollya_mpfi_t xprime;
  node *tree;
  node *num;
  node *denom;
  mpfr_t *numerator;
  mpfr_t *denominator;

  if ( (!mpfr_number_p(x)) || mpfr_zero_p(x) )  return makeConstant(x);
  mpq_init(q);
  mpz_init(u);
  sollya_mpfi_init2(xprime,(mp_prec_t)n);

  sollya_mpfi_set_fr(xprime,x);
  continuedFrac(q,xprime);
 
  mpq_get_num(u,q);
  numerator = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
  mpfr_init2(*numerator,mp_bits_per_limb*mpz_size(u));
  mpfr_set_z(*numerator,u,GMP_RNDN);

  mpq_get_den(u,q);
  denominator = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
  mpfr_init2(*denominator,mp_bits_per_limb*mpz_size(u));
  mpfr_set_z(*denominator,u,GMP_RNDN);

  tree = safeMalloc(sizeof(node));
  tree->nodeType = DIV;
  num = safeMalloc(sizeof(node));
  denom = safeMalloc(sizeof(node));
  num->nodeType = CONSTANT;
  denom->nodeType = CONSTANT;
  num->value = numerator;
  denom->value = denominator;
  tree->child1 = num;
  tree->child2 = denom;

  sollya_mpfi_clear(xprime);
  mpq_clear(q);
  mpz_clear(u);
  return tree;
}

int mpfr_round_to_ieee_format(mpfr_t rop, mpfr_t op, mp_prec_t prec, unsigned int width, mp_rnd_t mode) {
  int res;
  mpfr_t result;
  unsigned int exponent;
  mpfr_t largest, smallest, temp;
  mp_prec_t p;

  /* In the case when the function is called with a silly width (less than 3 or larger than 30),
     we exit from the tool. This case should never happen.
  */
  if ((width < 3) || (width > 30)) {
    sollyaFprintf(stderr,"Error: mpfr_round_to_ieee_format: an exponent width of less than 3 or larger than 30 is not supported\n");
    exit(1);
  }

  /* We will first produce an internal result and then write back to rop */
  mpfr_init2(result,prec);

  /* Handle the special cases: +/- 0, +/- Inf, NaN */
  if (mpfr_zero_p(op) || (!mpfr_number_p(op))) {
    /* The result is the input */
    mpfr_set(result, op, GMP_RNDN); /* exact */
  } else {
    /* Here, the input is not zero, nor Inf nor NaN

       We start with the first rounding step with unbounded exponent.

    */
    mpfr_set(result, op, mode); /* performs a rounding to the desired precision
                                   but with unbounded exponent
				*/

    /* Now check if overflow occurs: compare in magnitude with the largest
       representable number of the format.

       The largest number is 1 ulp (of precision prec) below 2^(2^(width - 1)).
    */
    exponent = 1 << (width - 1);
    mpfr_init2(largest, prec);
    mpfr_set_ui(largest, 1, GMP_RNDN); /* exact: power of 2 */
    mpfr_mul_2ui(largest, largest, exponent, GMP_RNDN); /* exact: power of 2 */
    mpfr_nextbelow(largest); /* exact by specification */
    if (mpfr_cmpabs(result, largest) > 0) {
      /* Here, we have an overflow

	 Depending on the rounding mode and the sign of the input
	 we get the largest representable number or +/- Inf as a result

      */
      switch (mode) {
      case GMP_RNDN:
	/* -Inf, +Inf */
	if (mpfr_sgn(op) < 0) {
	  mpfr_set_inf(result, -1);
	} else {
	  mpfr_set_inf(result, +1);
	}
	break;
      case GMP_RNDD:
	/* -Inf, +largest */
	if (mpfr_sgn(op) < 0) {
	  mpfr_set_inf(result, -1);
	} else {
	  mpfr_set(result, largest, GMP_RNDN); /* exact: same precision */
	}
	break;
      case GMP_RNDU:
	/* -largest, +Inf */
	if (mpfr_sgn(op) < 0) {
	  mpfr_neg(result, largest, GMP_RNDN); /* exact: same precision */
	} else {
	  mpfr_set_inf(result, +1);
	}
	break;
      case GMP_RNDZ:
	/* -largest, +largest */
	if (mpfr_sgn(op) < 0) {
	  mpfr_neg(result, largest, GMP_RNDN); /* exact: same precision */
	} else {
	  mpfr_set(result, largest, GMP_RNDN); /* exact: same precision */
	}
	break;
      default:
	sollyaFprintf(stderr,"Error: mpfr_round_to_ieee_format: unknown rounding mode %d\n", (int) mode);
	exit(1);
      }
    } else {
      /* Here, the result is either a signed 0, denormal or normal

	 We continue by checking if the first rounding is larger
	 than the least normal.

	 For a format of width bits of exponent, the smallest normal
	 is 2^(-2^(width - 1) + 2).

      */
      mpfr_init2(smallest, prec);
      exponent = 1 << (width - 1);
      exponent -= 2;
      mpfr_set_ui(smallest, 1, GMP_RNDN); /* exact: power of 2 */
      mpfr_div_2ui(smallest, smallest, exponent, GMP_RNDN); /* exact: power of 2 */

      if (mpfr_cmpabs(result, smallest) < 0) {
	/* Here, we have to emulate denormal rounding

	   Denormal rounding for precision prec and exponent width
	   width is:

	   result = 2^(-prec - 2^(width - 1) + 3) * round_integer((1/(2^(-prec - 2^(width - 1) + 3))) * op, mode)

           In other words,

           result = smallest * 2^(1-prec) * round_integer( 2^(prec-1)*op/smallest, mode)

           ( the interval [0, smallest) is sent to [0, 2^(prec-1) ) where fixed-point computations at
             precision prec are exactly integer computations, and then we send it back to [0, smallest)
           )
	 */
	p = mpfr_get_prec(op);
	mpfr_init2(temp, p);
	mpfr_set(temp, op, GMP_RNDN); /* exact: precision of temp not less than the one of op */
	exponent = 1 << (width - 1);
	exponent -= 3;
	exponent += prec;
	mpfr_mul_2ui(temp, temp, exponent, GMP_RNDN); /* exact: power of 2 and same precision */
	mpfr_rint(result, temp, mode); /* Performs round_integer with mode
					  no wrong rounding possible as precision of
					  result is prec and |temp| <= 2^(prec-1)
				       */
	mpfr_div_2ui(result, result, exponent, GMP_RNDN); /* exact: power of 2 and same precision */
	mpfr_clear(temp);
      }
      /* Otherwise the first rounding is already
	 the final result.
      */

      mpfr_clear(smallest);
    }
    mpfr_clear(largest);
  }

  /* Write back the result, while verifying if we don't get a double rounding there. */
  /* Double-rounding may occur if the precision of rop is smaller than the prec.     */
  if (mpfr_set(rop,result,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: double rounding occurred on invoking the IEEE 754-2008 general rounding operator.\n");
      printMessage(1,"Try to increase the working precision.\n");
    }
  }

  mpfr_clear(result);

  /* Compute the rounding direction that has finally been chosen */
  res = mpfr_cmp(rop,op);

  return res;
}

int mpfr_round_to_quad(mpfr_t rop, mpfr_t x) {
  return mpfr_round_to_ieee_format(rop, x, 113, 15, GMP_RNDN);
}

int mpfr_round_to_halfprecision(mpfr_t rop, mpfr_t x) {
  return mpfr_round_to_ieee_format(rop, x, 11, 5, GMP_RNDN);
}

int mpfr_round_to_quad_mode(mpfr_t rop, mpfr_t x, mp_rnd_t mode) {
  return mpfr_round_to_ieee_format(rop, x, 113, 15, mode);
}

int mpfr_round_to_halfprecision_mode(mpfr_t rop, mpfr_t x, mp_rnd_t mode) {
  return mpfr_round_to_ieee_format(rop, x, 11, 5, mode);
}

int mpfr_round_to_doubleextended(mpfr_t rop, mpfr_t op) {
  return mpfr_round_to_ieee_format(rop, op, 64, 15, GMP_RNDN);
}

int mpfr_round_to_doubleextended_mode(mpfr_t rop, mpfr_t op, mp_rnd_t mode) {
  return mpfr_round_to_ieee_format(rop, op, 64, 15, mode);
}
