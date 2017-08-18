/*
  log sin(Pi/4 x) and log cos(Pi/4 x) functions for CLNS
 
  Author : Sylvain Collange
 
  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved.
*/

#include "LogSinCos.hpp"
#include <sstream>
#include <vector>
#include <cmath>
#include "../utils.hpp"

using namespace std;

namespace flopoco{

	LogSinCos::LogSinCos(Target * target, int fL, int fTheta, int o, EvaluationMethod method) :
		Operator(target), fL(fL), fTheta(fTheta), order(o), method_(method)
	{
		ostringstream name;
		name<<"LogSinCos_"<< fL <<"_"<< fTheta << "_" << o; 
		uniqueName_ = name.str();
		setCombinatorial();
		
		int eEssZero = intlog2(fL);
		
		addInput ("x", fTheta + 3);
		addOutput("logsin", 1 + eEssZero + fL);
		addOutput("logcos", 1 + eEssZero + fL);
		
		// Range reduction: split x between quandrant and reduced argument
		
		// Octant (together with sign bit)
		vhdl << tab << declare("q", 2) << " <= x" << range(1+fTheta, fTheta) << ";\n";

		// Reduced argument
		vhdl << tab << declare("xr", fTheta) << " <= x" << range(fTheta-1, 0) << ";\n";
		vhdl << tab << declare("s") << " <= x" << of(2+fTheta) << ";\n";
		
		// Compute |xr|, -|xr|
		vhdl << tab << declare("minusxr", fTheta) << " <= not xr + 1;\n";

		vhdl << tab << "with s select\n";
		vhdl << tab << declare("xr_pos", fTheta) << " <=\n";
		vhdl << tab << tab << "minusxr when '1',\n";
		vhdl << tab << tab << "xr when others;\n";
		
		vhdl << tab << "with s select\n";
		vhdl << tab << declare("xr_neg", fTheta) << " <=\n";
		vhdl << tab << tab << "xr when '1',\n";
		vhdl << tab << tab << "minusxr when others;\n";
		
		char const * logcos_func = "log2(cos((Pi/4)*x))";
		logcos = NewEvaluator(target_, logcos_func, uniqueName_,
			fTheta, 1+eEssZero+fL, o, 0, 1, exp2(double(fL-fTheta)), method_);
		
		oplist.push_back(logcos);

		inPortMap(logcos, "X", "xr_pos");
		outPortMap(logcos, "R","lc1");
		vhdl << instance(logcos, "inst_logcos");	
		
		// Danger zone?
		
		// Always negative numbers: 1-extend
		// Shift left and align
		int d2arg_width = 2+eEssZero+max(fL,fTheta);
		vhdl << tab << declare("lc1_twice", d2arg_width) << " <= " << align(0, "lc1", fTheta-fL+1) << ";\n";
		vhdl << tab << declare("xr_neg_aligned", d2arg_width)
			<< " <= (" << og(2+eEssZero) << " & " << align(0, "xr_neg", fL-fTheta) << ");\n";
		
		vhdl << tab << "with dz select\n"
			<< tab << declare("d2arg", d2arg_width) << " <=\n"
			<< tab << tab << "xr_neg_aligned when '1',\n"
			<< tab << tab << "lc1_twice when others;\n";
		// .....
		// TODO
		
		vhdl << tab << "logsin <= lc1;\n";
		vhdl << tab << "logcos <= lc1;\n";

	}
	
	LogSinCos::~LogSinCos()
	{
	}

}
