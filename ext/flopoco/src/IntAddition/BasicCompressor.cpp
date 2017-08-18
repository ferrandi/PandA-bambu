#include <iostream>
#include <sstream>
#include <string>
#include "gmp.h"
#include "mpfr.h"
#include <vector>
#include <gmpxx.h>
#include <stdio.h>
#include <stdlib.h>
#include "BasicCompressor.hpp"


using namespace std;
namespace flopoco{

// personalized parameter
//string BasicCompressor::operatorInfo = "UserDefinedInfo list parameter;



BasicCompressor::BasicCompressor(Target * target, vector<int> h)
:Operator(target)
{
	ostringstream name;
	stringstream nm;
	/// compressors are always combinational independently by the target technology
	setCombinatorial();
	
	int w=0;
	int param=0;

	while(h[h.size()-1]==0)
	{
		h.erase(h.end()-1);
	}

	for(int i=h.size()-1; i>=0;i--)
		height.push_back(h[i]);
	
	name << "Compressor_";
	
	for(unsigned i=0; i<height.size();i++)
	{
		w=w+height[i];
		param=param+intpow2(height.size()-i-1)*height[i];
		name<<height[i];
	}

	wOut=intlog2(param);
	
	name << "_" << wOut;
	setName(name.str());
	setCopyrightString("Bogdan Popa, Illyes Kinga, 2012");
	
	
	
	stringstream xs;

	
	for(unsigned i=0;i<height.size();i++)
	{
		addInput(join("X",i), h[i]);
		
		if(i!=0)
		{
			xs<<"& X"<<height.size()-i-1<<" ";
		}
		else
		{
			xs<<"X"<<height.size()-1<<" ";	
		}
	}	
	
	xs<<";\n";
	
	addOutput("R", wOut);
	
	vhdl << tab << declare("X", w) << " <=" << xs.str();
	
	vhdl << tab << "with X select R <= \n";
	
	for (mpz_class i = 0; i < (1 << w); i++) 
	{
		
		mpz_class ppcnt=0;//popcnt(i);
		mpz_class ii=i;
		for(unsigned j=0;j<h.size();j++)
		{
			
		
			ppcnt+=popcnt(ii-((ii>>h[j])<<h[j]))*intpow2(j);
			ii=ii>>h[j];
		}
		
		vhdl << tab << tab << "\"" << unsignedBinary(ppcnt,wOut) << "\" when \""
		<< unsignedBinary(i,w) << "\", \n";
		
		
		}
		 
		 
		 
		vhdl << tab << tab << "\"" << std::string (wOut, '-') << "\" when others;\n" << endl;

		REPORT(DEBUG, "Generated " << name.str());
		 
		 
	}
	
	
	
	
	BasicCompressor::~BasicCompressor(){
	}

	unsigned BasicCompressor::getColumnSize(int column)
	{
		if (column>=(signed)height.size())
			return 0;
		else
			return height[height.size()-column-1];
	}

	int BasicCompressor::getOutputSize()
	{
		return wOut;
	}
	
	void BasicCompressor::emulate(TestCase * tc)
	{
		mpz_class r=0;
	
		for(unsigned i=0;i<height.size();i++)
			{
				mpz_class sx = tc->getInputValue(join("X",i));
				mpz_class p= popcnt(sx);
				r += p<<i;
			}	



		tc->addExpectedOutput("R", r);
	}
	
}
	


	
