/*
  LNS Hybrid Cotransformation operator : evaluates log2(1+2^x) or log2(1-2^x)
 
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
#include "CotranHybrid.hpp"
#include "../utils.hpp"
#include <cmath>
#include "LNSAdd.hpp"

using namespace std;

namespace flopoco{
 
	CotranHybrid::CotranHybrid(Target * target, int wE, int wF, int jl, int wECotranl, int o) :
		Operator(target), wE(wE), wF(wF), j(jl), wECotran(wECotranl)
	{
		if(wECotran < 0)
			wECotran = wE;

		if(j < 0)
			select_j();
	
		ostringstream name;
		name<<"Cotran_Hybrid_"<< wE <<"_"<< wF << "_" << j << "_" << wECotran << "_" << o;
		uniqueName_ = name.str();
	setCombinatorial();
	
	
		//	wEssZero = min(wE, log2(wF+1)+1)+wF;
		wEssZero = min(wE, intlog2(wF)) + wF;
	
		// Translated back from VHDL...
		DBMaxInput = (int)rint(double(1 << wF) * log((1.0 - (pow(2.0, (-pow(2.0, -wF)))))) / log(2.));;
	
		addInput("Z", wE + wF + 1);
		addInput("IsSub");

		addOutput("SBDB", wE + wF + 1);
	
		// Cannot specify VHDL type of signals using Operator::add_signal...
		// Output signals manually...

		cotran = new Cotran(target, wE, wF, j, wECotran, o);	// TODO: + guard bits!
	
		oplist.push_back(cotran);
	
		for(int i = 0; i < 4 - wECotran; ++i)
			{
				HOTBM * tb = gen_db_table(i, o);
				oplist.push_back(tb);
				db_tables.push_back(tb);
			}
	}

	CotranHybrid::~CotranHybrid()
	{
	}

	void CotranHybrid::select_j()
	{
		j = (wF + wECotran) / 2;
	}

	int CotranHybrid::getJ()
	{
		return j;
	}

	double db(double z)
	{
		return log(abs(1 - pow(2., z))) / log(2.);
	}

	HOTBM * CotranHybrid::gen_db_table(int i, int order)
	{
		if(i == 0)
			{
				// T0: from EssZero to -8
				// input: wEssZero-1 bits
				// output: wF - 7 bits
				// scale 2^7
				// order 1?
				return new HOTBM(target_, "log2(abs(1-2^x))", uniqueName_, wEssZero-1, wF-7, order, -(1 << (wEssZero - wF)), -8, 1 << 7);
			}
		else
			{
				// Ti: from -2^(4-i) to -2^(3-i)
				// input: wF + 3 - i bits
				// output: wF + ceil(log2(-db(-2^(3-i))))
				// scale: 2^(-ceil(log2(-db(-2^(3-i)))))
				double zmin = -ldexp(1.0, 4-i);	// may be || < 1
				double zmax = -ldexp(1.0, 3-i);
		
				// assumes i <= 3. TODO: generalize
				int out_ebits = rint(ceil(log(-db(-ldexp(1.0, 3-i))) / log(2)));
				double scale = ldexp(1.0, -out_ebits);
		
				//std::cout << "zmin=" << zmin << ", zmax=" << zmax << ", out_ebits=" << out_ebits << std::endl;

				return new HOTBM(target_, "log2(abs(1-2^x))", uniqueName_, wF + 3 - i, wF + out_ebits, order, zmin, zmax, scale);
			}
	}


	void CotranHybrid::outputVHDL(std::ostream& o, std::string name)
	{
		licence(o,"Sylvain Collange, Jérémie Detrey (2008)");
	
		// Output tables

		//Operator::StdLibs(o);
		o
			<< "library ieee;\n"
			<< "use ieee.std_logic_1164.all;\n"
			<< "use ieee.numeric_std.all;\n"
			<< "\n";
	
		outputVHDLEntity(o);
		newArchitecture(o,name);	

		//output_vhdl_signal_declarations(o);

		for(int i = 0; i < 4 - wECotran; ++i)
			{
				db_tables[i]->outputVHDLComponent(o);
	
				o << tab << "signal Out_T" << i << " : std_logic_vector(" << db_tables[i]->wOut() - 1 << " downto 0);\n";
			}

		cotran->outputVHDLComponent(o);
		o << tab << "signal Out_Cotran : std_logic_vector(" << wE + wF << " downto 0);\n";

		beginArchitecture(o);
	
		o
			<< tab << "cotran : " << cotran->getName() << "\n"
			<< tab << "  port map (\n"
			<< tab << "    Z => Z,\n"
			<< tab << "    IsSub => IsSub,\n"
			<< tab << "    SBDB => Out_Cotran);\n"
			<< tab << "\n";
	
		for(int i = 0; i < 4 - wECotran; ++i)
			{
				o
					<< tab << "t" << i << " : " << db_tables[i]->getName() << "\n"
					<< tab << "  port map (\n"
					<< tab << "    x => Z(" << (db_tables[i]->wIn() - 1) << " downto 0),\n"
					<< tab << "    r => Out_T" << i << ");\n"
					<< tab << "\n";
			}
	
	
		o
			<< tab << "SBDB <= \n";
		for(int i = 0; i < 4 - wECotran; ++i)
			{
				o << tab << tab <<
					"(" << (wE + wF) << " downto " << db_tables[i]->wOut() << " => '0') & Out_T" << i << "\n";
		
				int start = wF + 3 - i;
				int end = wF + 3 - i;
				if(i == 0)
					start = wE + wF - 1;

				o << tab << tab << tab <<
					"when Z(" << start << " downto " << end << ") /= (" << start << " downto " << end << " => '1') and IsSub = '1' else\n";
			}
		o << tab << tab << "Out_Cotran;\n";
	
	
		o<< "end architecture;" << endl << endl;
	}

}
#endif// HAVE_SOLLYA
