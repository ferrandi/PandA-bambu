/*
  LNS division operator
 
  Author : Sylvain Collange
 
  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved.
 */

#include "LNSDiv.hpp"
#include "../utils.hpp"
#include <cmath>

using namespace std;

namespace flopoco{

	LNSDiv::LNSDiv(Target * target, int wE, int wF) :
		Operator(target), wE(wE), wF(wF)
	{
		setCombinatorial();	
		ostringstream name;
		/* The name has the format: LNSDiv_wE_wF where: 
			wE = width of the integral part of the exponent
			wF = width of the fractional part of the exponent */
		name << "LNSDiv_" << wE << "_" << wF; 
		setName(name.str()); 
		setCombinatorial(); // TODO this should no longer be useful in the new framework


		addInput ("nA", wE + wF + 3);
		addInput ("nB", wE + wF + 3);

		addOutput("nR", wE + wF + 3);
	}

	LNSDiv::~LNSDiv()
	{
	}


	void LNSDiv::outputVHDL(std::ostream& o, std::string name)
	{
		licence(o,"Jérémie Detrey, Florent de Dinechin (2003-2004), Sylvain Collange (2008)");
		Operator::stdLibs(o);
		outputVHDLEntity(o);
		newArchitecture(o,name);

		o
			<< tab << "constant wE : positive := " << wE <<";\n"
			<< tab << "constant wF : positive := " << wF <<";\n"
			<< tab << "\n"
			<< tab << "signal sRn : std_logic;\n"
			<< tab << "signal eRn : std_logic_vector(wE+wF downto 0);\n"
			<< tab << "signal xRn : std_logic_vector(1 downto 0);\n"
			<< tab << "signal nRn : std_logic_vector(wE+wF+2 downto 0);\n"
			<< tab << "signal nRx : std_logic_vector(wE+wF+2 downto 0);\n"
			<< tab << "\n"
			<< tab << "signal xA  : std_logic_vector(1 downto 0);\n"
			<< tab << "signal xB  : std_logic_vector(1 downto 0);\n"
			<< tab << "signal xAB : std_logic_vector(3 downto 0);\n";
	
		beginArchitecture(o);

		o
			<< tab << "eRn <= (nA(wE+wF-1) & nA(wE+wF-1 downto 0)) - (nB(wE+wF-1) & nB(wE+wF-1 downto 0));\n"
			<< tab << "\n"
			<< tab << "sRn <= nA(wE+wF) xor nB(wE+wF);\n"
			<< tab << "xRn <= \"00\" when eRn(wE+wF downto wE+wF-1) = \"10\" else\n"
			<< tab << "	 \"10\" when eRn(wE+wF downto wE+wF-1) = \"01\" else\n"
			<< tab << "	 \"01\";\n"
			<< tab << "nRn <= xRn & sRn & eRn(wE+wF-1 downto 0);\n"
			<< tab << "\n"
			<< tab << "xA <= nA(wE+wF+2 downto wE+wF+1);\n"
			<< tab << "xB <= nB(wE+wF+2 downto wE+wF+1);\n"
			<< tab << "xAB <= xA & xB;\n"
			<< tab << "\n"
			<< tab << "with xAB select\n"
			<< tab << "  nR(wE+wF+2 downto wE+wF+1) <= xRn  when \"0101\",\n"
			<< tab << "                                          \"00\" when \"0001\" | \"0010\" | \"0110\",\n"
			<< tab << "                                          \"10\" when \"0100\" | \"1000\" | \"1001\",\n"
			<< tab << "                                          \"11\" when others;\n"
			<< tab << "\n"
			<< tab << "nR(wE+wF downto 0) <= nRn(wE+wF downto 0);\n"
			<< "end architecture;\n";

	}
}
