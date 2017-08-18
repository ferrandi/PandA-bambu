/*
  4/pi * atan(2^x) function with input domain partitioning
 
  Author : Sylvain Collange
 
  This file is part of the FloPoCo project developed by the Arenaire
  team at Ecole Normale Superieure de Lyon

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved.

*/

// works only with sollya
#ifdef HAVE_SOLLYA
#include "AtanPow.hpp"
#include <sstream>
#include <vector>
#include "../FixFunctions/HOTBM.hpp"

using namespace std;

namespace flopoco{

	AtanPow::AtanPow(Target * target, int wE, int wF, int o, EvaluationMethod method) :
		Operator(target), wE(wE), wF(wF), order(o), method_(method)
	{
		T[0] = T[1] = T[2] = 0;

		ostringstream name;
		name<<"AtanPow_"<< wE <<"_"<< wF << "_" << o; 
		uniqueName_ = name.str();
		setCombinatorial();
		addInput ("x", wE + wF);
		addOutput("r", wF);
	
		if(wF > 7) {
			// Who is going to free this memory??
			T[0] = NewInterpolator(wF+wE-1, wF-7, o, -(1 << wE), -8, 1 << 7);
			oplist.push_back(T[0]);

			vhdl << tab << declare("x0", wF+wE-1) << " <= x(" << (wF+wE-2) << " downto 0);\n";
			inPortMap(T[0], "X", "x0");
			outPortMap(T[0], "R","out_t0");
			vhdl << instance(T[0], "inst_t0");	
		}

		vhdl << tab << declare("x1", wF+2) << " <= x(" << (wF+1) << " downto 0);\n";

		if(wF > 6) {
			T[1] = NewInterpolator(wF+2, wF-4, o, -8, -4, 1 << 4);
			oplist.push_back(T[1]);

			inPortMap(T[1], "X", "x1");
			outPortMap(T[1], "R","out_t1");
			vhdl << instance(T[1], "inst_t1");	
		}

		T[2] = NewInterpolator(wF+2, wF, o, -4, 0, 1);
		oplist.push_back(T[2]);

		inPortMap(T[2], "X", "x1");
		outPortMap(T[2], "R","out_t2");
		vhdl << instance(T[2], "inst_t2");	

		vhdl << tab << "r <= ";
	
		if(wF > 7) {
			vhdl << "(" << (wF-1) << " downto " << wF-6 << " => '0') & out_t0(" << (wF-7) << " downto 0)\n"
			     << "         when x(" << (wF+wE-1) << " downto " << (wF+3) << ") /= (" << (wF+wE-1) << " downto " << (wF+3) << " => '1') else\n       ";
		}


		if(wF > 6) {
			vhdl << "(" << (wF-1) << " downto " << (wF-3) << " => '0') & out_t1(" << (wF-4) << " downto 0)\n"
			     << "         when x(" << (wF+2) << ") /= '1' else\n       ";
		}
		vhdl << "out_t2(" << (wF-1) << " downto 0);\n";

	}
	
	Operator * AtanPow::NewInterpolator(int wI, int wO, int o, double xmin, double xmax, double scale)
	{
		char const * my_func = "4/Pi * atan(2^(x))";
		return NewEvaluator(target_, my_func, uniqueName_, wI, wO, o, xmin, xmax, scale, method_);
	}
	

	AtanPow::~AtanPow()
	{
	}

}
#endif// HAVE_SOLLYA
