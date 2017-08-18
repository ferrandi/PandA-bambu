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
using namespace flopoco;


static void usage(char *name){
  cerr << "\nUsage: "<<name<<" wE wF x" <<endl ;
  cerr << "  x is input as an arbitrary precision decimal number" <<endl ;
  cerr << "    and will be rounded to the nearest FP(wE,wF) number." <<endl ;
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


  mpfr_t mpx;

  mpfr_init2 (mpx, wF+1);
  mpfr_set_str (mpx, argv[3], 10, GMP_RNDN);

  cout<< fp2bin(mpx, wE, wF) << endl;
}
