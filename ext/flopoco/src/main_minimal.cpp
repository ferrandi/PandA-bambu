/*
  A minimal example on how use libFloPoCo to generate operators for you.

  In the FloPoCo distribution it is built into executable fpadder_example

  To compile it independently, do something like this:
  gcc -c -I$FLOPOCO_DIR/src main_minimal.cpp
  gcc -L$FLOPOCO_DIR -pg -Wall main_minimal.o  -o fpadder_example -lFloPoCo -lmpfr -lgmp -lgmpxx -lxml2 -lmpfi /usr/local/lib/libsollya.so 



  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Author : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2011.
  All rights reserved.

*/

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cstdlib>


#include "FloPoCo.hpp" 


using namespace std;
using namespace flopoco;




int main(int argc, char* argv[] )
{

	Target* target = new Virtex4();

	int wE = 9;
	int wF = 33;
	
	Operator*	op = new FPAdderSinglePath(target, wE, wF, wE, wF, wE, wF);
	
	ofstream file;
	file.open("FPAdder.vhdl", ios::out);
	
	op->outputVHDLToFile(file);

	file.close();
	
	
	cerr << endl<<"Final report:"<<endl;
	op->outputFinalReport(0);
	return 0;
}



