/*
  Euclidean division by 3
   
  The input is given as x_{n}x_{n-1}...x_{0}, but the dividend is actually
  x_{n}00x_{n-1}00...x_{1}00x_{0}

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Author : Florent de Dinechin, Matei Istoan

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2013.
  All rights reserved.

*/

// TODOs: remove from d its powers of two .

#include <iostream>
#include <fstream>
#include <sstream>

#include "IntConstDiv3.hpp"


using namespace std;


namespace flopoco{

	IntConstDiv3::EuclideanDiv3Table::EuclideanDiv3Table(Target* target, int d_, int alpha_, int gamma_, int delta_, bool lastTable_) :
		/* input on alpha+gamma bits: alpha from the number, gamma from the pervious remainder */
		/* computations on alpha+gamma+nbZeros bits */
		/* output on alpha+2*gamma*/
		/* maximum value in the table: 101 - 10 from the pervious remainder, 1 from the current digit being processed */
		Table(target, alpha_+gamma_, (lastTable_ ? (alpha_-1) : alpha_)*(1+delta_)+gamma_+(lastTable_ ? 1 : 0), 0, ((d_-1)<<alpha_)+oneGenerator(alpha_), 1),
				d(d_), alpha(alpha_), gamma(gamma_), delta(delta_), lastTable(lastTable_)
	{
		setCopyrightString("Florent de Dinechin, Matei Istoan (2013)");
		
		ostringstream name;
		
		srcFileName = "IntConstDiv3::EuclideanDiv3Table";
		name << "EuclideanDiv3Table_" << d << "_" << alpha << "_" << delta << "zeros" << (lastTable ? "_lastTable" : "");
		setName(name.str());
	};


	mpz_class IntConstDiv3::EuclideanDiv3Table::function(int x)
	{
		// machine integer arithmetic should be safe here
		//	getting r_{1}r_{0}x_{k}x_{k-1}...x_{0} as input, actually will work with r_{1}r_{0}x_{k}00x_{k-1}...00x_{0} (delta zeros)
		if((x < 0) || (x >= (1<<(alpha+gamma))))
		{
			ostringstream e;
			e << "ERROR in IntConstDiv3::EuclideanDiv3Table::function, argument out of range" <<endl;
			throw e.str();
		}
		
		// the vector containing the digits of the number (MSB of number is on position 0)
		list<char> digitVector;
		int copyX, digit, trueX;
		
		copyX = x;
		while(copyX > 0)
		{
			digit = copyX & 1;
			digitVector.push_front(digit);
			copyX  = (copyX >> 1);
		}
		for(int i=digitVector.size(); i<alpha+gamma; i++)
			digitVector.push_front(0);
		
		// get the actual output
		//	first, the two remainder bits
		trueX = 0;
		if(!digitVector.empty())
		{
			trueX = digitVector.front();
			digitVector.pop_front();
		}
		if(!digitVector.empty())
		{
			trueX = (trueX<<1) + digitVector.front();
			digitVector.pop_front();
		}
		//	now the rest of the digits
		while(!digitVector.empty())
		{
			trueX = (trueX<<1) + digitVector.front();
			if(!(lastTable && digitVector.size()==1))
				trueX = (trueX << delta);
			
			digitVector.pop_front();
		}
		
		// perform the computations
		int q = trueX / d;
		int r = trueX - q*d;
		
		int result = (q<<gamma) + r;
				
		return mpz_class(result);
	};
		
	

	int IntConstDiv3::quotientSize()
	{
		return qSize;
	};

	int IntConstDiv3::remainderSize()
	{
		return gamma;
	};



	IntConstDiv3::IntConstDiv3(Target* target, int wIn_, int d_, int alpha_, int nbZeros_,  bool remainderOnly_, map<string, double> inputDelays)
		: Operator(target), wIn(wIn_), d(d_), remainderOnly(remainderOnly_), alpha(alpha_), nbZeros(nbZeros_)
	{
		setCopyrightString("Florent de Dinechin, Matei Istoan (2013)");
		srcFileName="IntConstDiv3";

		//set gamma to the size of the remainder
		gamma = intlog2(d-1);
		
		//check alpha, and set it properly if necessary
		if(alpha == -1)
		{
			alpha = target->lutInputs() - gamma;
						
			if (alpha<1)
			{
				REPORT(LIST, "WARNING: This value of d is too large for the LUTs of this FPGA (alpha=" << alpha << ").");
				REPORT(LIST, " Building an architecture nevertheless, but it may be very large and inefficient.");
				alpha=1;
			}
		}
		REPORT(INFO, "alpha=" << alpha);

		//generate a unique name
		std::ostringstream o;
		if(remainderOnly)
			o << "IntConstRem3_";
		else
			o << "IntConstDiv3_";
		o << d << "_" << vhdlize(wIn) << "_"  << vhdlize(alpha) << "_" << vhdlize(nbZeros) << "zeros_";
		if(target->isPipelined()) 
			o << vhdlize(target->frequencyMHz());
		else
			o << "comb";
		uniqueName_ = o.str();

		//set the quotient size
		qSize = (1+nbZeros)*(wIn-1) + 1;
		
		//manage the pipeline
		setCriticalPath(getMaxInputDelays(inputDelays));

		//create the input
		addInput("X", wIn);

		//create the output for the quotient (if necessary) and the remainder
		if(!remainderOnly)
			addOutput("Q", qSize);
		addOutput("R", gamma);

		int k = wIn/alpha;
		int rem = wIn-k*alpha;
		if (rem!=0)
			k++;

		REPORT(INFO, "Architecture consists of k=" << k  <<  " levels.");
		REPORT(DEBUG, "  d=" << d << "  wIn=" << wIn << "  alpha=" << alpha << "  gamma=" << gamma <<  "  k=" << k  <<  "  qSize=" << qSize);
		
		EuclideanDiv3Table* table;
		table = new EuclideanDiv3Table(target, d, alpha, gamma, nbZeros, false);
		useSoftRAM(table);
		oplist.push_back(table);
		double tableDelay = table->getOutputDelay("Y");

		string ri, xi, ini, outi, qi;
		
		ri = join("r", k);
		vhdl << tab << declare(ri, gamma) << " <= " << zg(gamma, 0) << ";" << endl;

		//all bits except the last
		for (int i=k-1; i>0; i--)
		{
			//manageCriticalPath(tableDelay);
			manageCriticalPath(target->lutDelay());

			xi = join("x", i);
			if(i==k-1 && rem!=0)
				// at the MSB, pad with 0es
				vhdl << tab << declare(xi, alpha, true) << " <= " << zg(alpha-rem, 0) <<  " & X" << range(wIn-1, i*alpha) << ";" << endl; 
			else 
				// normal case
				vhdl << tab << declare(xi, alpha, true) << " <= X" << range((i+1)*alpha-1, i*alpha) << ";" << endl;
			
			ini = join("in", i);
			// This ri is r_{i+1}
			vhdl << tab << declare(ini, alpha+gamma) << " <= " << ri << " & " << xi << ";" << endl;
			
			outi = join("out", i);
			
			inPortMap(table, "X", ini);
			outPortMap(table, "Y", outi);
			vhdl << instance(table, join("table",i));
			
			syncCycleFromSignal(outi);
			
			ri = join("r", i);
			qi = join("q", i);
			vhdl << tab << declare(qi, alpha*(1+nbZeros), true) << " <= " << outi << range(alpha*(1+nbZeros)+gamma-1, gamma) << ";" << endl;
			vhdl << tab << declare(ri, gamma) << " <= " << outi << range(gamma-1, 0) << ";" << endl << endl;
			
			syncCycleFromSignal(qi);
			syncCycleFromSignal(ri);
		}
		
		//handle the last bit
		{
			table = new EuclideanDiv3Table(target, d, alpha, gamma, nbZeros, true);
			useSoftRAM(table);
			oplist.push_back(table);
			tableDelay = table->getOutputDelay("Y");
			
			//manageCriticalPath(tableDelay);
			manageCriticalPath(target->lutDelay());

			xi = join("x", 0);
			vhdl << tab << declare(xi, alpha, true) << " <= X" << range(alpha-1, 0) << ";" << endl;
			
			ini = join("in", 0);
			vhdl << tab << declare(ini, alpha+gamma) << " <= " << ri << " & " << xi << ";" << endl;
			
			outi = join("out", 0);
			
			inPortMap(table, "X", ini);
			outPortMap(table, "Y", outi);
			vhdl << instance(table, join("table", 0));
			
			syncCycleFromSignal(outi);
			
			ri = join("r", 0);
			qi = join("q", 0);
			vhdl << tab << declare(qi, (alpha-1)*(1+nbZeros)+1, true) << " <= " << outi << range((alpha-1)*(1+nbZeros)+1+gamma-1, gamma) << ";" << endl;
			vhdl << tab << declare(ri, gamma) << " <= " << outi << range(gamma-1, 0) << ";" << endl << endl;
			
			syncCycleFromSignal(qi);
			syncCycleFromSignal(ri);
		}

		if(!remainderOnly)
		{
			// build the quotient output
			int tempQSize = (alpha*(nbZeros+1))*k-2;
			vhdl << tab << declare("tempQ", tempQSize) << " <= ";
			for (unsigned int i=k-1; i>=1; i--)
				vhdl << "q" << i << " & ";
			vhdl << "q0;" << endl;
			
			syncCycleFromSignal("tempQ");
			
			vhdl << tab << "Q <= tempQ" << range(qSize-1, 0)  << ";" << endl;
			
			outDelayMap["Q"] = getCriticalPath();
		}

		// This ri is r_0
		vhdl << tab << "R <= " << ri << ";" << endl;
		
		outDelayMap["R"] = getCriticalPath();
	}	

	IntConstDiv3::~IntConstDiv3()
	{
	}



	void IntConstDiv3::emulate(TestCase * tc)
	{
		// Get I/O values
		mpz_class X = tc->getInputValue("X");
		
		// Recreate the true input value (x_{n}00x_{n-1}00...x_{1}00x_{0})
		int shiftSize = 0;
		mpz_class copyX, trueX, digit;
		
		copyX = X;
		trueX = 0;
		while(copyX > 0)
		{
			digit = copyX & 1;
			trueX = trueX + (digit << (3*shiftSize));
			
			copyX = copyX >> 1;
			shiftSize++;
		}
		
		// Compute correct value
		mpz_class Q = trueX / d;
		mpz_class R = trueX - Q*d;		
		
		// Add the output values
		if(!remainderOnly)
			tc->addExpectedOutput("Q", Q);
		tc->addExpectedOutput("R", R);
	}
 

}
