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
#include <mpfr.h>
#include <cstdlib>

#include"../utils.hpp"
using namespace std;


static void usage(char *name){
  cerr << endl << "Usage: "<<name<<" wE_in wF_in MaxMSB_in LSB_acc MSB_acc  x" << endl ;
  cerr << "  x is a binary string of (MSB_acc-LSB_acc+1) bits in fixed-point format," << endl ;
  cerr << "    this format being defined by MSB_acc and LSB_acc." << endl ;
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
  if(argc != 7) usage(argv[0]);
//  int wE = check_strictly_positive(argv[1], argv[0]);
//  int wF = check_strictly_positive(argv[2], argv[0]);
//  int MaxMSBX = atoi(argv[3]);
  int LSBA = atoi(argv[4]);
  int MSBA = atoi(argv[5]);
  int sizeAcc = MSBA-LSBA+1;

  char* x = argv[6];

  char *p=x;
  int l=0;
  while (*p){
    if(*p!='0' && *p!='1') {
      cout <<endl << *p <<endl;
      cerr<<"ERROR: expecting a binary string, got "<<argv[6]<<endl;
      usage(argv[0]);
    }
    p++; l++;
  }
  if(l != sizeAcc) {
    cerr<<"ERROR: binary string of size "<< l <<", should be of size "<<sizeAcc<<endl;
    usage(argv[0]);
  }


  // significand
  mpfr_t sig, one, two;
  mpfr_init2(one, 2);
  mpfr_init2(two, 2);
  mpfr_set_d(one, 1.0, GMP_RNDN);
  mpfr_set_d(two, 2.0, GMP_RNDN);

  mpfr_init2(sig, sizeAcc);

  // First bit decides sign in two's complement representation
  if(x[0]=='1')
    mpfr_set_d(sig, -1.0, GMP_RNDN); 
  else
    mpfr_set_d(sig, 0.0, GMP_RNDN);

  p=x+1;
  for (int i=1; i< sizeAcc; i++) {
    mpfr_mul(sig, sig, two, GMP_RNDN);
    if (*p=='1') {
      mpfr_add(sig, sig, one, GMP_RNDN);
    }
    p++;
  }
  // Now we have an int in sig. Scale it according to LSBA
  mpfr_mul_2si (sig, sig, LSBA, GMP_RNDN);

  // output on enough bits
  mpfr_out_str (0, // std out
		10, // base
		0, // enough digits so that number may be read back
		sig, 
		GMP_RNDN);
  cout << endl;
  return 0;
}
