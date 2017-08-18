/*
 * Utility for converting a FP number into its binary representation,
 * for testing etc
 *
 * Author : Florent de Dinechin
 *
 * This file is part of the FloPoCo project developed by the Arenaire
 * team at Ecole Normale Superieure de Lyon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
*/

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <gmpxx.h>
#include <mpfr.h>
#include <cstdlib>

#include"../utils.hpp"
using namespace std;
using namespace flopoco;


static void usage(char *name){
  cerr << endl << "Usage: "<<name<<" wE wF x" << endl ;
  cerr << "  x is a binary string of (wE+wF+3) bits, in the FPLibrary/FloPoCo format:" << endl ;
  cerr << "    <exceptions><sign><exponent><significand>" << endl ;
  cerr << "    exceptions: 2 bits, 00: null, 01: normal number, 10: infinity (signed), 11: NaN" << endl ;
  cerr << "    sign:  0: positive, 1: negative" << endl ;
  cerr << "    exponent is on wF bits, biased such that 0 is coded by 011...11" << endl ;
  cerr << "    significand is on wF bits and codes 1.significand (implicit leading one)" << endl ;
  exit (EXIT_FAILURE);
}




int check_strictly_positive(char* s, char* cmd) {
  int n=atoi(s);
  if (n<=0){
    cerr<<"ERROR: got "<<s<<", expected strictly positive number."<<endl;
    usage(cmd);
  }
  return n;
}

int main(int argc, char* argv[] )
{
  if(argc != 4) usage(argv[0]);
  int wE = check_strictly_positive(argv[1], argv[0]);
  int wF = check_strictly_positive(argv[2], argv[0]);
  char* x = argv[3];

  char *p=x;
  int l=0;
  while (*p){
    if(*p!='0' && *p!='1') {
      cerr<<"ERROR: expecting a binary string, got "<<argv[3]<<endl;
     usage(argv[0]);
    }
    p++; l++;
  }
  if(l != wE+wF+3) {
    cerr<<"ERROR: binary string of size "<< l <<", should be of size "<<wE+wF+3<<endl;
    usage(argv[0]);
  }


  // significand
  mpfr_t sig, one, two;
  mpfr_init2(one, 2);
  mpfr_set_d(one, 1.0, GMP_RNDN);
  mpfr_init2(two, 2);
  mpfr_set_d(two, 2.0, GMP_RNDN);

  mpfr_init2(sig, wF+1);
  mpfr_set_d(sig, 1.0, GMP_RNDN); // this will be the implicit one

  p=x+3+wE;
  for (int i=0; i< wF; i++) {
    mpfr_mul(sig, sig, two, GMP_RNDN);
    if (*p=='1') {
      mpfr_add(sig, sig, one, GMP_RNDN);
    }
    p++;
  }

  // set sign
  if(x[2]=='1')
    mpfr_neg(sig, sig, GMP_RNDN);

  // bring back between 1 and 2: multiply by 2^-wF
  mpfr_mul_2si (sig, sig, -wF, GMP_RNDN);

  // exponent
  int exp=0;
  p=x+3;
  for (int i=0; i< wE; i++) {
    exp = exp<<1;
    if (*p=='1') {
      exp+=1;
    }
    p++;
  }
  // subtract bias
  exp -= ((1<<(wE-1)) -1);

  // scale sig according to exp
  mpfr_mul_2si (sig, sig, exp, GMP_RNDN);

  // output on enough bits
  mpfr_out_str (0, // std out
		10, // base
		0, // enough digits so that number may be read back
		sig, 
		GMP_RNDN);
  cout << endl;
  return 0;
}
