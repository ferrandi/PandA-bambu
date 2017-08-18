/*
  LNS multiplication operator
 
  Author : Sylvain Collange
 
  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved.
 */

#include "LNSMul.hpp"
#include "../utils.hpp"
#include <cmath>
#include "../IntAdder.hpp"

using namespace std;

namespace flopoco{

	
	LNSMul::LNSMul(Target * target, int wE, int wF) :
		Operator(target), wE(wE), wF(wF)
	{
		ostringstream name;
		/* The name has the format: LNSMul_wE_wF where: 
			wE = width of the integral part of the exponent
			wF = width of the fractional part of the exponent */
		name << "LNSMul_" << wE << "_" << wF; 
		setName(name.str());
		setCopyrightString("Jérémie Detrey, Florent de Dinechin (2003-2004), Sylvain Collange (2008)");
		addInput ("nA", wE + wF + 3);
		addInput ("nB", wE + wF + 3);
		addOutput("nR", wE + wF + 3);
		
		addConstant("wE", "positive", wE);
		addConstant("wF", "positive", wF);
		
		//vhdl << tab << declare("eRn", wE+wF+1) << " <= (nA(wE+wF-1) & nA(wE+wF-1 downto 0)) + (nB(wE+wF-1) & nB(wE+wF-1 downto 0));\n";

		IntAdder *my_adder = new IntAdder(target, wE+wF+1);
		oplist.push_back(my_adder);
		vhdl << tab << declare("X", wE+wF+1) << "<= nA(wE+wF-1) & nA(wE+wF-1 downto 0);\n";
		vhdl << tab << declare("Y", wE+wF+1) << "<= nB(wE+wF-1) & nB(wE+wF-1 downto 0);\n";
		inPortMap   (my_adder, "X", "X");
		inPortMap   (my_adder, "Y", "Y");
		inPortMapCst(my_adder, "Cin", "'0'");
		outPortMap (my_adder, "R","eRn");
		vhdl << instance(my_adder, "my_add");	
		
		vhdl << tab << declare("sRn") << " <= nA(wE+wF) xor nB(wE+wF);\n";
		vhdl << tab << declare("xRn", 2) << " <= \"00\" when eRn(wE+wF downto wE+wF-1) = \"10\" else\n"
			<< tab << "	 \"10\" when eRn(wE+wF downto wE+wF-1) = \"01\" else\n"
			<< tab << "	 \"01\";\n";
		vhdl << tab << declare("nRn", wE+wF+3) << " <= xRn & sRn & eRn(wE+wF-1 downto 0);\n";

		vhdl << tab << declare("xA", 2) << " <= nA(wE+wF+2 downto wE+wF+1);\n";
		vhdl << tab << declare("xB", 2) << " <= nB(wE+wF+2 downto wE+wF+1);\n";
		vhdl << tab << declare("xAB", 4) << " <= xA & xB when xA >= xB else\n"
			<< tab << "	 xB & xA;\n";
		vhdl
			<< tab << "with xAB select\n"
			<< tab << tab << "nR(wE+wF+2 downto wE+wF+1) <= xRn  when \"0101\",\n"
			<< tab << "	                                    \"00\" when \"0000\" | \"0100\",\n"
			<< tab << "	                                    \"10\" when \"1001\" | \"1010\",\n"
			<< tab << "	                                    \"11\" when others;\n"
			<< tab << "\n"
			<< tab << "nR(wE+wF downto 0) <= nRn(wE+wF downto 0);\n";
	}

	LNSMul::~LNSMul()
	{
	}

	void LNSMul::emulate(TestCase * tc)
	{
		mpz_class a = tc->getInputValue("nA");
		mpz_class b = tc->getInputValue("nB");
		
		bool sr = mpz_tstbit(a.get_mpz_t(), wE+wF) ^ mpz_tstbit(b.get_mpz_t(), wE+wF);
		bool xa0 = mpz_tstbit(a.get_mpz_t(), wE+wF+1);
		bool xa1 = mpz_tstbit(a.get_mpz_t(), wE+wF+2);
		bool xb0 = mpz_tstbit(b.get_mpz_t(), wE+wF+1);
		bool xb1 = mpz_tstbit(b.get_mpz_t(), wE+wF+2);
		bool xr0,xr1;
		
		bool nan = (xa0 & xa1) | (xb0 & xb1);		// 11
		bool zero = !(xa0 | xa1) | !(xb0 | xb1);	// 00
		bool inf = (xa1 & !xa0) | (xb1 & !xb0);		// 10
		
		mpz_class r = a + b;
		bool ovf = (mpz_sizeinbase(r.get_mpz_t(), 2) > unsigned(wE+wF)) && (sgn(r) > 0);
		bool udf = (mpz_sizeinbase(r.get_mpz_t(), 2) > unsigned(wE+wF)) && (sgn(r) < 0);

		if(nan | (zero & inf)) {
			xr0 = xr1 = 1;
		}
		else if(zero) {
			xr1 = xr0 = 0;
		}
		else if(inf | ovf) {
			xr1 = 1;
			xr0 = 0;
		}
		else if(udf) {
			xr1 = xr0 = 0;
		}
		else {
			xr1 = 0;
			xr0 = 1;
		}
		if(sr) mpz_setbit(r.get_mpz_t(), wE+wF); else mpz_clrbit(r.get_mpz_t(), wE+wF);
		if(xr0) mpz_setbit(r.get_mpz_t(), wE+wF+1); else mpz_clrbit(r.get_mpz_t(), wE+wF+1);
		if(xr1) mpz_setbit(r.get_mpz_t(), wE+wF+2); else mpz_clrbit(r.get_mpz_t(), wE+wF+2);
		r &= mpzpow2(wE+wF+3)-1;	// Discard overflowing bits
		
		tc->addExpectedOutput("nR", r);
	}
}
