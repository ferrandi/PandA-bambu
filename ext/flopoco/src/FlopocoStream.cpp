/**
  FloPoCo Stream for VHDL code including cycle information

  This file is part of the FloPoCo project developed by the Arenaire
  team at Ecole Normale Superieure de Lyon
  
  Authors :   Bogdan Pasca, Nicolas Brunie

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved. */


#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <math.h>
#include <string.h>
#include <gmp.h>
#include <mpfr.h>
#include <cstdlib>
#include <gmpxx.h>
#include "utils.hpp"
#include "FlopocoStream.hpp"


using namespace std;

namespace flopoco{

	/** The FlopocoStream class.  */
	FlopocoStream::FlopocoStream(){
		vhdlCode.str("");
		vhdlCodeBuffer.str("");
		currentCycle_ = 0;
	}


	FlopocoStream::~FlopocoStream(){
	}

}
