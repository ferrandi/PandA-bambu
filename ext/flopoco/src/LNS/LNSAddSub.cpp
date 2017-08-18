/*
  LNS addition/subtraction operator
 
  Author : Sylvain Collange
 

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved.
 */

// works only with sollya
#ifdef HAVE_SOLLYA

#include "LNSAddSub.hpp"
#include "../utils.hpp"
#include <cmath>

using namespace std;

namespace flopoco{


	LNSAddSub::LNSAddSub(Target * target, int wE, int wF) :
		Operator(target), wE(wE), wF(wF)
	{
		setCombinatorial();	
		ostringstream name;
		/* The name has the format: LNSAddSub_wE_wF where: 
			wE = width of the integral part of the exponent
			wF = width of the fractional part of the exponent */
		name << "LNSAddSub_" << wE << "_" << wF; 
		setName(name.str());

		addInput ("nA", wE + wF + 3);
		addInput ("nB", wE + wF + 3);

		addOutput("nR", wE + wF + 3);
	

		addsub = new CotranHybrid(target, wE, wF);
		oplist.push_back(addsub);
		j = addsub->getJ();
	}

	LNSAddSub::~LNSAddSub()
	{
	}

	void LNSAddSub::outputVHDL(std::ostream& o, std::string name)
	{
		int wEssZero = min(wE, intlog2(wF)) + wF;
	
		// Translated back from VHDL...
		int DBMaxInput = (int)rint(double(1 << wF) * log((1.0 - (pow(2.0, (-pow(2.0, -wF)))))) / log(2.));

		licence(o,"Jérémie Detrey, Florent de Dinechin (2003-2004), Sylvain Collange (2008)");
		o << 
			"library ieee;\n"
			"use ieee.std_logic_1164.all;\n"
			"use ieee.numeric_std.all;\n"
			"\n";

		outputVHDLEntity(o);
		newArchitecture(o,name);
		addsub->outputVHDLComponent(o);

		o
			<< tab << "constant wE : positive := " << wE <<";\n"
			<< tab << "constant wF : positive := " << wF <<";\n"
			<< tab << "constant wEssZero : positive := " << wEssZero << ";\n"
			<< tab << "constant j : positive := 5;\n"
			<< tab << "constant DB_Max_Input : integer := " << DBMaxInput << ";\n"
			<< tab << "\n"
			<< tab << "signal X, Y, R : std_logic_vector(wE + wF + 1 downto 0);\n"
			<< tab << "signal Ov : std_logic;\n"
			<< tab << "signal Zero : std_logic;\n"
			<< tab << "signal xR : std_logic_vector(1 downto 0);\n"
			<< tab << "signal xAB : std_logic_vector(3 downto 0);\n"
			<< tab << "signal xA, xB : std_logic_vector(1 downto 0);\n"
			<< tab << "signal sAB : std_logic;\n"
			<< tab << "\n"
			<< tab << "signal nA_r, nB_r : std_logic_vector(wE + wF + 2 downto 0);\n"
			<< tab << "\n"
			<< tab << "signal Z : std_logic_vector(wE + wF downto 0);\n"
			<< tab << "signal Xv, Yv : std_logic_vector(wE + wF downto 0);\n"
			<< tab << "signal SBDB : std_logic_vector(wE + wF downto 0);\n"
			<< tab << "signal Za, Zb : std_logic_vector(wE + wF downto 0);\n"
			<< tab << "signal IsEssZero, Special, SelMuxA : std_logic;\n"
			<< tab << "signal R0_1 : std_logic_vector(wE + wF downto 0);\n";
	
		beginArchitecture(o);

		o
			<< tab << "nA_r <= nA;\n"
			<< tab << "nB_r <= nB;\n"
			<< tab << "\n"
			<< tab << "X(wE+wF-1 downto 0) <= nA(wE+wF-1 downto 0);\n"
			<< tab << "X(wE+wF) <= nA(wE+wF-1);\n"
			<< tab << "X(wE+wF+1) <= nA(wE+wF);	-- sign\n"
			<< tab << "\n"
			<< tab << "Y(wE+wF-1 downto 0) <= nB(wE+wF-1 downto 0);\n"
			<< tab << "Y(wE+wF) <= nB(wE+wF-1);\n"
			<< tab << "Y(wE+wF+1) <= nB(wE+wF);\n"
			<< tab << "\n"
			<< tab << "xR <=	\"00\"	when Zero = '1' else\n"
			<< tab << "		\"10\"	when Ov = '1' else\n"
			<< tab << "		\"01\";\n"
			<< tab << "\n"
			<< tab << "sAB <= nA_r(wE+wF) xor nB_r(wE+wF);\n"
			<< tab << "\n"
			<< tab << "Xv <= X(wE+wF downto 0);\n"
			<< tab << "Yv <= Y(wE+wF downto 0);\n"
			<< tab << "\n"
			<< tab << "Za <= std_logic_vector(signed(Xv) - signed(Yv));\n"
			<< tab << "Zb <= std_logic_vector(signed(Yv) - signed(Xv));\n"
			<< tab << "\n"
			<< tab << "with Za(wE+wF) select                   -- MUX for the negative\n"
			<< tab << "	Z <=\n"
			<< tab << "		Za when '1',                        -- Y > X\n"
			<< tab << "		Zb when others;                     -- X > Y\n"
			<< tab << "\n"
			<< tab << "addsub : " << addsub->getName() << "\n"
			<< tab << "	port map (\n"
			<< tab << "		Z => Z,\n"
			<< tab << "		IsSub => sAB,\n"
			<< tab << "		SBDB => SBDB);\n"
			<< tab << "\n"
			<< tab << "IsEssZero <= '1' when signed(Z(wE+wF downto j)) < to_signed(DB_Max_Input, wE + wF - j + 1) else '0';\n"
			<< tab << "\n"
			<< tab << "Special <=\n"
			<< tab << "	'1' when Z(wEssZero-1 downto j) = (wEssZero-1 downto j => '1') else\n"
			<< tab << "	'0';\n"
			<< tab << "\n"
			<< tab << "SelMuxA <= (Za(wE+wF) and ((not sAB) or IsEssZero or Special) ) or\n"
			<< tab << "	       (Zb(wE+wF) and (sAB and (not IsEssZero) and (not Special)) );\n"
			<< tab << "\n"
			<< tab << "with SelMuxA select\n"
			<< tab << "	R0_1 <=\n"
			<< tab << "		Yv when '1',\n"
			<< tab << "		Xv when others;\n"
			<< tab << "\n"
			<< tab << "R <= std_logic_vector(resize(signed(SBDB), wE+wF+2) + resize(signed(R0_1), wE+wF+2));\n"
			<< tab << "\n"
			<< tab << "\n"
			<< tab << "xA <= nA_r(wE+wF+2 downto wE+wF+1);\n"
			<< tab << "xB <= nB_r(wE+wF+2 downto wE+wF+1);\n"
			<< tab << "xAB <= nA_r(wE+wF+2 downto wE+wF+1) & nB_r(wE+wF+2 downto wE+wF+1);\n"
			<< tab << "\n"
			<< tab << "nR(wE+wF+2 downto wE+wF+1) <=		\"11\"	when xA = \"11\" or xB = \"11\" else\n"
			<< tab << "									xR		when xAB = \"0101\" else\n"
			<< tab << "									\"1\" & sAB when xAB = \"1010\" else\n"
			<< tab << "									xB		when xA = \"00\" else\n"
			<< tab << "									xA;\n"
			<< tab << "\n"
			<< tab << "\n"
			<< tab << "with xAB select\n"
			<< tab << "	nR(wE+wF) <=	R(wE+wF+1)						when \"0101\",\n"
			<< tab << "					nA_r(wE+wF) and nB_r(wE+wF)	when \"0000\",\n"
			<< tab << "					nB_r(wE+wF)					when \"0001\",\n"
			<< tab << "					nA_r(wE+wF)					when others;\n"
			<< tab << "	\n"
			<< tab << "with xAB select\n"
			<< tab << "	nR(wE+wF-1 downto 0) <=	R(wE+wF-1 downto 0)	when \"0101\",\n"
			<< tab << "								nA_r(wE+wF-1 downto 0)	when \"0100\",\n"
			<< tab << "								nB_r(wE+wF-1 downto 0)	when others;\n"
			<< tab << "\n"
			<< "end architecture;\n";

	}
}
#endif// HAVE_SOLLYA
