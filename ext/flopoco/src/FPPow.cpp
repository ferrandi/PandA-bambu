/*
  An FP power function for FloPoCo
 
  Author : Florent de Dinechin, Pedro Echevarria
 
  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved.

*/

#include <fstream>
#include <sstream>
#include <math.h>	// for NaN
#include "FPLog.hpp"
#include "FPExp.hpp"
#include "LZOC.hpp"
#include "FPPow.hpp"
#include "FPMultiplier.hpp"
#include "FPNumber.hpp"
#include "utils.hpp"


using namespace std;


namespace flopoco{


	void FPPow::compute_error(mpfr_t & r, mpfr_t &epsE, mpfr_t& epsM, mpfr_t& epsL ) {
		mpfr_t r1, one;
		mpfr_init2(r1, 1000);
		mpfr_init2(one, 16);
		mpfr_set_d(one,  1.0, GMP_RNDN);
		mpfr_add(r1,one, epsE, GMP_RNDN);
		// r1 is (1+epsE)

		mpfr_init2(r, 1000);
		mpfr_set(r, epsL, GMP_RNDN);
		mpfr_mul(r,r, epsM, GMP_RNDN);
		mpfr_add(r,r, epsM, GMP_RNDN);
		mpfr_add(r,r, epsL, GMP_RNDN);
		mpfr_add(r,r, one, GMP_RNDN);
		// here r is 1 + epsM + epsL + epxMexpL

		// multiply it by max ( y ln(x) )

		FPNumber fpMaxFloat = FPNumber(wE, wF, FPNumber::largestPositive);
		mpfr_t maxExpIn;
		mpfr_init2(maxExpIn, 1000);
		fpMaxFloat.getMPFR(maxExpIn);
		mpfr_log(maxExpIn, maxExpIn, GMP_RNDU);
		mpfr_mul(r,r,maxExpIn, GMP_RNDN);
		if(verbose) mpfr_out_str (stderr, 10, 30, maxExpIn, GMP_RNDN); cerr << " ";

		// then take the exp
		mpfr_exp(r,r, GMP_RNDN);

		// and that's it.
		mpfr_mul(r,r,r1, GMP_RNDN);


		mpfr_clears(r1, one, NULL);

	}






	FPPow::FPPow(Target* target, int wE, int wF, int type, int logTableSize, int expTableSize, int expDegree)
		: Operator(target), wE(wE), wF(wF), type(type)
	{
		
		setCopyrightString("F. de Dinechin, C. Klein  (2008)");
		srcFileName="FPPow";

		ostringstream o;

		o << (type?"FPPowr_":"FPPow_") << wE << "_" << wF << "_";
		if(target->isPipelined()) 
			o << target->frequencyMHz() ;
		else
			o << "comb";
		setName(o.str());

		addFPInput("X", wE, wF);
		addFPInput("Y", wE, wF);
		addFPOutput("R", wE, wF, 1); // 2 because faithfully rounded

		addConstant("wE", "positive", wE);
		addConstant("wF", "positive", wF);
		
		int expG;

		// TODO get rid of the following somehow
		if(wF<=23)
			expG=3;
		else
			expG=4;


		int logwF= wF+expG+wE-1;
		REPORT(INFO, "mantissa width for the Log output: " << logwF);

		vhdl << tab  << declare("flagsX", 2) << " <= X(wE+wF+2 downto wE+wF+1);" << endl;
		vhdl << tab  << declare("signX") << " <= X(wE+wF);" << endl;
		vhdl << tab  << declare("expFieldX", wE) << " <= X(wE+wF-1 downto wF);" << endl;
		vhdl << tab  << declare("fracX", wF) << " <= X(wF-1 downto 0);" << endl;

		vhdl << tab  << declare("flagsY", 2) << " <= Y(wE+wF+2 downto wE+wF+1);" << endl;
		vhdl << tab  << declare("signY") << " <= Y(wE+wF);" << endl;
		vhdl << tab  << declare("expFieldY", wE) << " <= Y(wE+wF-1 downto wF);" << endl;
		vhdl << tab  << declare("fracY", wF) << " <= Y(wF-1 downto 0);" << endl;



		// All the exception detection can take place at the beginning of the pipeline.
		// We have plenty of cycles so we quietely overpipeline
		vhdl<<"-- Inputs analysis  --"<<endl;

		vhdl<<"-- zero inputs--"<<endl;
		vhdl << tab  << declare("zeroX") << " <= '1' when flagsX=\"00\" else '0';"<<endl;
		vhdl << tab  << declare("zeroY") << " <= '1' when flagsY=\"00\" else '0';"<<endl;
		vhdl<<"-- normal inputs--"<<endl;
		vhdl<<tab<<declare("normalX") << " <= '1' when flagsX=\"01\" else '0';"<<endl;
		vhdl<<tab<<declare("normalY") << " <= '1' when flagsY=\"01\" else '0';"<<endl;
		vhdl<<"-- inf input --"<<endl;
		vhdl<<tab<<declare("infX") << " <= '1' when flagsX=\"10\" else '0';"<<endl;
		vhdl<<tab<<declare("infY") << " <= '1' when flagsY=\"10\" else '0';"<<endl;
		vhdl<<"-- NaN inputs  --"<<endl;
		vhdl<<tab<<declare("s_nan_in") << " <= '1' when flagsX=\"11\" or flagsY=\"11\" else '0';"<<endl;		
		
		vhdl<<"-- Comparison of X to 1   --"<<endl;
		vhdl << tab  << declare("OneExpFrac", wE+wF) << " <=  \"0\" & " << rangeAssign(wE-2, 0, "'1'") << " & " << rangeAssign(wF-1, 0, "'0'") << ";" << endl;
		IntAdder *cmpOneAdder = new IntAdder(target, wE+wF+1);
		oplist.push_back(cmpOneAdder);		
		vhdl << tab << declare("ExpFracX",wE+wF+1) << "<= \"0\" & expFieldX & fracX;"<<endl;
		vhdl << tab << declare("OneExpFracCompl",wE+wF+1) << "<=  \"1\" & (not OneExpFrac);"<<endl;
		inPortMap(cmpOneAdder, "X", "ExpFracX");
		inPortMap(cmpOneAdder, "Y", "OneExpFracCompl");
		inPortMapCst(cmpOneAdder, "Cin", "'1'");
		outPortMap (cmpOneAdder, "R", "cmpXOneRes");
		vhdl << instance(cmpOneAdder, "cmpXOne") << endl;
		syncCycleFromSignal("cmpXOneRes");
		nextCycle();
		// setCriticalPath( cmpOneAdder->getOutputDelay("R") );
		vhdl << tab  << declare("XisOneAndNormal") << " <= '1' when X = (\"010\" & OneExpFrac)" << " else '0';" << endl;
		vhdl << tab  << declare("absXgtOneAndNormal") << " <= normalX and (not XisOneAndNormal) and (not cmpXOneRes("<<wE+wF<<"));" << endl;
		vhdl << tab  << declare("absXltOneAndNormal") << " <= normalX and cmpXOneRes("<<wE+wF<<");" << endl;

		
		



		if(type==0){//pow

			setCycle(0);

			// We first have to reverse the fraction, because our LZOC counts leading bits, not trailing ones.
			// There must be some standard VHDL way of doing that
			vhdl << tab  << declare("fracYreverted", wF)<<" <= ";
			for(int i=0; i<wF; i++){
				vhdl << "fracY" << of(i);
				if (i<wF-1)
					vhdl << "&";
			}
			vhdl << ";" <<  endl;

			LZOC* right1counter = new LZOC(target, wF);
			oplist.push_back(right1counter);
			inPortMap(right1counter, "I", "fracYreverted");
			inPortMapCst(right1counter, "OZB", "\'0\'");  
			outPortMap(right1counter, "O", "Z_rightY");
			vhdl << instance(right1counter, "right1counter");
			// Synchronize the datapaths of this LZOC and of the comparator to one
			syncCycleFromSignal("Z_rightY");
			syncCycleFromSignal("cmpXOneRes");
			nextCycle();

			vhdl<<"-- compute the weight of the less significant one of the mantissa"<<endl;
			vhdl << tab  << declare("WeightLSBYpre", wE+1)<<" <= ('0' & expFieldY)- CONV_STD_LOGIC_VECTOR("<< (1<<(wE-1))-1 + wF <<","<<wE+1<<");"<<endl;
			vhdl << tab  << declare("WeightLSBY", wE+1)<<" <= WeightLSBYpre + Z_rightY;"<<endl;
			// No problem tooverpipeline 
			nextCycle();

			vhdl << tab  << declare("oddIntY") <<" <= normalY when WeightLSBY = CONV_STD_LOGIC_VECTOR(0, "<<wE+1<<") else '0'; -- LSB has null weight"<<endl;
			vhdl << tab  << declare("evenIntY")<<" <= normalY when WeightLSBY(wE)='0' and oddIntY='0' else '0'; --LSB has strictly positive weight "<<endl;
			vhdl << tab  << declare("notIntNormalY") <<" <= normalY when WeightLSBY(wE)='1' else '0'; -- LSB has negative weight"<<endl;

			vhdl << endl;

			vhdl<<"-- Pow Exceptions  --"<<endl;

			// TODO case -inf^integer not managed?
			vhdl << tab  << declare("RisInfSpecialCase") << "  <= " << endl
			     << tab << tab << "   (zeroX  and  (oddIntY or evenIntY)  and signY)  -- (+/- 0) ^ (negative int y)"<<endl
			     << tab << tab << "or (zeroX and infY and signY)                      -- (+/- 0) ^ (-inf)"  << endl
			     << tab << tab << "or (absXgtOneAndNormal   and  infY  and not signY) -- (|x|>1) ^ (+inf)"  << endl
			     << tab << tab << "or (absXltOneAndNormal   and  infY  and signY)     -- (|x|<1) ^ (-inf)" << endl
			     << tab << tab << "or (infX and  normalY  and not signY) ;            -- (inf) ^ (y>0)" << endl;

			vhdl << tab  << declare("RisZeroSpecialCase") << " <= " << endl
			     << tab << tab << "   (zeroX and  (oddIntY or evenIntY)  and not signY)  -- (+/- 0) ^ (positive int y)"<<endl
			     << tab << tab << "or (zeroX and  infY  and not signY)                   -- (+/- 0) ^ (+inf)"<<endl
			     << tab << tab << "or (absXltOneAndNormal   and  infY  and not signY)    -- (|x|<1) ^ (+inf)"<<endl
			     << tab << tab << "or (absXgtOneAndNormal   and  infY  and signY)        -- (|x|>1) ^ (-inf)" << endl
			     << tab << tab << "or (infX and  normalY  and signY) ;                   -- (inf) ^ (y<0)" << endl;

			vhdl << tab  << declare("RisOne") << " <= " << endl
			     << tab << tab << "   zeroY                                          -- x^0 = 1 without exception"<<endl
			     << tab << tab << "or (XisOneAndNormal and signX and infY)           -- (-1) ^ (-/-inf)"<<endl          
			     << tab << tab << "or (XisOneAndNormal  and not signX);              -- (+1) ^ (whatever)" << endl ;

			vhdl << tab  << declare("RisNaN") << " <= (s_nan_in and not zeroY) or (normalX and signX and notIntNormalY);"<<endl;

			vhdl << tab  << declare("signR") << " <= signX and (oddIntY);" << endl;
		}
		else{
		
			vhdl<<"-- Powr Exceptions  --"<<endl;
			vhdl << tab  << declare("RisInfSpecialCase") << "  <= " << endl
			     << tab << tab << "   (zeroX  and  normalY and signY)                 -- (+/- 0) ^  (negative finite y)"<<endl
			     << tab << tab << "or (zeroX and infY and signY)                      -- (+/- 0) ^ (-inf)"  << endl
			     << tab << tab << "or (absXgtOneAndNormal   and  infY  and not signY) -- (|x|>1) ^ (+inf)"  << endl
			     << tab << tab << "or (absXltOneAndNormal   and  infY  and signY)     -- (|x|<1) ^ (-inf)" << endl
			     << tab << tab << "or (infX and  normalY  and not signY) ;            -- (inf) ^ (y>0)" << endl;

			vhdl << tab  << declare("RisZeroSpecialCase") << " <= " << endl
			     << tab << tab << "   (zeroX and  normalY and not signY)  -- (+/- 0) ^ (positive int y)"<<endl
			     << tab << tab << "or (zeroX and  infY  and not signY)                   -- (+/- 0) ^ (+inf)"<<endl
			     << tab << tab << "or (absXltOneAndNormal   and  infY  and not signY)    -- (|x|<1) ^ (+inf)"<<endl
			     << tab << tab << "or (absXgtOneAndNormal   and  infY  and signY);        -- (|x|>1) ^ (-inf)" << endl;

			vhdl << tab  << declare("RisOne") << " <= " << endl
			     << tab << tab << "   (normalX and (not signX)   and zeroY)                           -- x^0 = 1 if 0<x<+inf"<<endl
			     << tab << tab << "or (XisOneAndNormal  and (not signX) and normalY and (not signY)); -- (+1) ^ (whatever)" << endl ;

			vhdl << tab  << declare("RisNaN") << " <= s_nan_in"<<endl
			     << tab << tab << "or (signX and not zeroX)        -- (x<0) ^ whatever"<<endl
			     << tab << tab << "or (XisOneAndNormal and infY)   -- (1) ^ (+/-inf)"<<endl
			     << tab << tab << "or (zeroX and zeroY)            -- (+/- 0) ^ (+/- 0)"<<endl
			     << tab << tab << "or (infX and zeroY);    -- (x<0) ^ whatever"<<endl;

			vhdl << tab  << declare("signR") << " <= '0';" << endl;

#if 0
			vhdl << tab  << "-- x^(+/-0)=1" << endl;
			vhdl << tab  << declare("case_x_to_0") << " <= '1' when (normalX='1' and zeroY='1') else '0';" << endl;
		
			vhdl << tab  << "-- -+-0^^-inf=+inf" << endl;
			vhdl << tab  << declare("case_0_to_neginf") << " <= '1' when (zeroX='1' and infY='1' and signY='1')  else '0';" << endl;
		
			vhdl << tab  << "--  -0^^+inf= 0 (p) NaN (pr)" << endl;
			vhdl << tab  << declare("case_neg0_to_posinf") << " <= '1' when (zeroX='1' and signY='1' and infY='1' and signY='0')  else '0';" << endl;
		
			vhdl << tab  << "-- 0^^+inf=0" << endl;
			vhdl << tab  << declare("case_pos0_to_posinf") << " <= '1' when (zeroX='1' and signY='0' and infY='1' and signY='0')  else '0';" << endl;
		
			vhdl << tab  << "--  +-0^+-0= 1 (p) NaN (pr)" << endl;
			vhdl << tab  << declare("case_0_to_0") << " <= zeroX and zeroY;" << endl;

			vhdl << tab  << "--  +inf^+-0= 1 (p) NaN (pr)" << endl;
			vhdl << tab  << declare("case_posinf_to_0") << " <=  '1' when (infX='1' and signX='0' and zeroY='1') else '0';" << endl;

			
			vhdl << tab  << "-- +-0^^negative =+inf" << endl;
			vhdl << tab  << declare("powr_case_0_to_yn") << " <= '1' when (zeroX='1' and normalY='1' and signY='1')  else '0';" << endl;
			
			vhdl << tab  << "-- +-0^^positive =+0" << endl;
			vhdl << tab  << declare("powr_case_0_to_yp") << " <= '1' when (zeroX='1' and normalY='1' and signY='0')  else '0';" << endl;
			
			vhdl << tab  << "--  1^+-inf= NaN (pr)" << endl;
			vhdl << tab  << declare("powr_case_pos1_to_inf") << " <= '1' when (XisOne='1'  and signX='0' and infY='1') else '0';" << endl;
			
			vhdl << tab  << "-- 1^+-fin=1" << endl;
			vhdl << tab  << declare("powr_case_1_to_finite") << " <= '1' when (XisOne='1' and normalY='1') else '0';" << endl;
			
			vhdl << tab  << "-- 1^+-inf=NaN" << endl;
			vhdl << tab  << declare("powr_case_1_to_infinite") << " <= '1' when (XisOne='1' and infY='1') else '0';" << endl;
			
			vhdl << tab  << declare("RisNaN") << " <= s_nan_in or (signX and normalX) or case_neg0_to_posinf or powr_case_1_to_infinite;"<<endl;
			vhdl << tab  << declare("RisInf") << " <= powr_case_0_to_yn or case_0_to_neginf;"<<endl;
			vhdl << tab  << declare("RisZero") << " <= powr_case_0_to_yp or case_pos0_to_posinf;"<<endl;
			vhdl << tab  << declare("RisOne") << " <= case_x_to_0 or powr_case_1_to_finite;"<<endl;
			vhdl << tab  << declare("signR") << " <= '0';"<<endl;
#endif
		}


		// Now the part that takes 99% of the size: computing exp(y*logx). 

		setCycle(0);
		setCriticalPath(0);
		// For the input to the log, take |X| as the case X<0 is managed separately
		vhdl << tab << declare("logIn", 3+wE + logwF) << " <= flagsX & \"0\" & expFieldX & fracX & " << rangeAssign(logwF-wF-1, 0, "'0'") << " ;" << endl; 

		FPLog* log = new FPLog(target,  wE,  logwF, logTableSize );
		oplist.push_back(log);
		inPortMap(log, "X", "logIn");
		outPortMap(log, "R", "lnX");
		vhdl << instance(log, "log");

		syncCycleFromSignal("lnX");
		nextCycle();

#if 0
		// TODO: the following mult could be  truncated
		FPMultiplier* mult = new FPMultiplier(target,   /*X:*/ wE, logwF,   /*Y:*/ wE, wF,  /*R: */  wE,  wF+wE+expG, 
		                                      1 /* norm*/); 
#else
		// truncated
		FPMultiplier* mult = new FPMultiplier(target,   /*X:*/ wE, logwF,   /*Y:*/ wE, wF,  /*R: */  wE,  wF+wE+expG, 
		                                      1, /* norm*/ 
		                                      0 /* faithful only*/); 
#endif
		oplist.push_back(mult);
		inPortMap(mult, "Y", "Y");
		inPortMap(mult, "X", "lnX");
		outPortMap(mult, "R", "P");
		vhdl << instance(mult, "mult");



		syncCycleFromSignal("P");
#if 0 // while fine-tuning the pipeline
		setCriticalPath( mult->getOutputDelay("R") );
		FPExp* exp = new FPExp(target,  wE,  wF, 0/* means default*/, 0, expG, true, inDelayMap("X", getCriticalPath() + 2*target->localWireDelay()) );
#else		
		nextCycle();
		FPExp* exp = new FPExp(target,  wE,  wF, 0/* means default*/, 0, expG, true);
#endif

		oplist.push_back(exp);
		inPortMap(exp, "X", "P");
		outPortMap(exp, "R", "E");
		vhdl << instance(exp, "exp");

		syncCycleFromSignal("E");
		nextCycle();

		vhdl << tab  << declare("flagsE", 2) << " <= E(wE+wF+2 downto wE+wF+1);" << endl;

		vhdl << tab  << declare("RisZeroFromExp") << " <= '1' when flagsE=\"00\" else '0';" << endl;
		vhdl << tab  << declare("RisZero") << " <= RisZeroSpecialCase or RisZeroFromExp;" << endl;
		vhdl << tab  << declare("RisInfFromExp") << "  <= '1' when flagsE=\"10\" else '0';" << endl;
		vhdl << tab  << declare("RisInf") << "  <= RisInfSpecialCase or RisInfFromExp;" << endl;


		vhdl << tab  << declare("flagR", 2) << " <= " << endl
		     << tab  << tab << "     \"11\" when RisNaN='1'" << endl
		     << tab  << tab << "else \"00\" when RisZero='1'" << endl
		     << tab  << tab << "else \"10\" when RisInf='1'" << endl
		     << tab  << tab << "else \"01\";" << endl;

		vhdl << tab << declare("R_expfrac", wE+wF) << " <= CONV_STD_LOGIC_VECTOR("<< (1<<(wE-1))-1<<","<<wE<<") &  CONV_STD_LOGIC_VECTOR(0, "<< wF << ") when RisOne='1'"
		     << endl << tab << tab << " else E" << range(wE+wF-1, 0) << ";" << endl; 
		vhdl << tab << "R <= flagR & signR & R_expfrac;" << endl; 



	}	

	FPPow::~FPPow()
	{
	}







	void FPPow::emulate(TestCase * tc)
	{
		/* Get I/O values */
		mpz_class svX = tc->getInputValue("X");
		mpz_class svY = tc->getInputValue("Y");
		
		/* Compute correct value */
		FPNumber fpx(wE, wF, svX);
		FPNumber fpy(wE, wF, svY);
		mpfr_t x,y,  l, p,  ru,rd;
		mpfr_init2(x,  1+wF);
		mpfr_init2(y,  1+wF);
		mpfr_init2(ru, 1+wF);
		mpfr_init2(rd, 1+wF); 
		mpfr_init2(l,  1+10*wF);
		mpfr_init2(p,  1+10*wF);
		fpx.getMPFR(x);
		fpy.getMPFR(y);

#if 1 // The default test, testing faithful rounding
		if(type==0) {
			// pow is easy, since we have it in mpfr
			mpfr_pow(rd, x, y, GMP_RNDD);
			mpfr_pow(ru, x, y, GMP_RNDU);
		}
		else {
			// powr is not in mpfr. Fortunately it is defined by exp(y*ln(x)), so let's emulate it this way
			// We should use infinite precision but we don't have it, so we approximate it with 10wF. 
			// Statistical arguments tell that 2wF should be enough, so we are a bit on the safe side. 
			// Still, we have no proof 10wF is enough, but then it's the occasion to find a counterexample, isn't t?
			mpfr_log(l, x, GMP_RNDN);
			mpfr_mul(p, l, y, GMP_RNDN);
			// the final exp does the only directed rounding
			mpfr_exp(rd, p, GMP_RNDD);
			mpfr_exp(ru, p, GMP_RNDU);
		}
		FPNumber  fprd(wE, wF, rd);
		FPNumber  fpru(wE, wF, ru);
		mpz_class svRD = fprd.getSignalValue();
		mpz_class svRU = fpru.getSignalValue();
		tc->addExpectedOutput("R", svRD);
		tc->addExpectedOutput("R", svRU);

#else // will generate one error for each value that is not correctly rounded
		// useful to count the percentage of correct rounding
		if(type==0) {
			// pow is easy, since we have it in mpfr
			mpfr_pow(rd, x, y, GMP_RNDN);
		}
		else {
			// powr is not in mpfr. Fortunately it is defined by exp(y*ln(x)), so let's emulate it this way
			// We should use infinite precision but we don't have it, so we approximate it with 10wF. 
			// Statistical arguments tell that 2wF should be enough, so we are a bit on the safe side. 
			// Still, we have no proof 10wF is enough, but then it's the occasion to find a counterexample, isn't t?
			mpfr_log(l, x, GMP_RNDN);
			mpfr_mul(p, l, y, GMP_RNDN);
			// the final exp does the only directed rounding
			mpfr_exp(rd, p, GMP_RNDN);
		}
		FPNumber  fprd(wE, wF, rd);
		mpz_class svRD = fprd.getSignalValue();
		tc->addExpectedOutput("R", svRD);

#endif
		mpfr_clears(x, y, l, p, ru, rd, NULL);
	}
 

	// TEST FUNCTIONS


	void FPPow::buildStandardTestCases(TestCaseList* tcl){
		TestCase *tc;

		// Random testing quickly gets the 2^(3+3) combinations of (exn,sign)
		// I'm not sure it gets all the cases with an integer y
		// Anyway, much TODO here.  
		tc = new TestCase(this); 
		tc->addFPInput("X", FPNumber::plusInfty);
		tc->addFPInput("Y", FPNumber::minusDirtyZero);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addFPInput("X", FPNumber::minusInfty);
		tc->addFPInput("Y", 3.95);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addFPInput("X", 3.0);
		tc->addFPInput("Y", 2.0);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addFPInput("X", 9.0);
		tc->addFPInput("Y", 0.5);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addFPInput("X", 2.0);
		tc->addFPInput("Y", 0.5);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addFPInput("X", 3423.0);
		tc->addFPInput("Y", 0.125234);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addFPInput("X", .23432);
		tc->addFPInput("Y", 0.5342);
		emulate(tc);
		tcl->add(tc);

		//The cases with negative input and integer exponent
		tc = new TestCase(this); 
		tc->addFPInput("X", FPNumber::minusDirtyZero);
		tc->addFPInput("Y", -17); 
		tc->addComment("negative odd integer exponent"); 
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addFPInput("X", FPNumber::minusDirtyZero);
		tc->addFPInput("Y", -42); 
		tc->addComment("negative even integer exponent"); 
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addFPInput("X", FPNumber::minusInfty);
		tc->addFPInput("Y", -17); 
		tc->addComment("negative odd integer exponent"); 
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addFPInput("X", FPNumber::minusInfty);
		tc->addFPInput("Y", -42); 
		tc->addComment("negative even integer exponent"); 
		emulate(tc);
		tcl->add(tc);

#if 0 // keep for cut and paste
		tc = new TestCase(this); 
		tc->addFPInput("X", );
		tc->addFPInput("Y", );
		tc->addComment(""); 
		emulate(tc);
		tcl->add(tc);
#endif

	}



 
	TestCase* FPPow::buildRandomTestCase(int i){
		TestCase *tc;
		tc = new TestCase(this); 
		mpz_class x,y,r;
		mpz_class normalExn = mpz_class(1)<<(wE+wF+1);
		mpz_class expOfOne = (((mpz_class(1))<<(wE-1))-1)<<wF;
		mpz_class expOfOneHalf = (((mpz_class(1))<<(wE-1))-2)<<wF;
		mpz_class bias = ((1<<(wE-1))-1);
		/* Fill inputs */
		int j=i%8;

		if(j==0){
			//fully random
			x = getLargeRandom(wE+wF+3);
			y = getLargeRandom(wE+wF+3);
		}
		else if (j==1) {
			// strictly positive, finite numbers
			x  = getLargeRandom(wF)  +  normalExn;
			r  = getLargeRandom(wF)  +  normalExn;
		}
		else if (j==2 || j==3) {
			// x and y in the binade [1,2]
			x  = getLargeRandom(wF) + expOfOne +  normalExn;
			y  = getLargeRandom(wF) + expOfOne +  normalExn;
		}
		else if (j==4 || j==5) {
			// x random, y small integer
			x = getLargeRandom(wE+wF) +  normalExn;
			y = getLargeRandom(3);
			// convert y to an mpfr_t
			mpfr_t ty;
			mpfr_init2(ty,  1+wF);
			mpfr_set_z(ty, y.get_mpz_t(), GMP_RNDN);
			FPNumber fpy(wE, wF, ty);
			y=fpy.getSignalValue();
			mpfr_clears( ty, NULL);
		}
		else {
			//  r random not too large, x in [1,2], y=exp(log(r)/x)
			x  = getLargeRandom(wF)  + expOfOne +  normalExn;
			int max = 5;
			r  = getLargeRandom(wF) +  (((getLargeRandom(max)-(1<<(max-1))) + (((mpz_class(1))<<(wE-1))-1) )<<wF)+  normalExn;
			// Now all we want is to set y=exp(log(r)/x) 
			// first convert x and r to mpfr_t
			FPNumber fpx(wE, wF, x);
			FPNumber fpr(wE, wF, r);
			mpfr_t tx, ty, tr;
			mpfr_init2(tx,  1+wF);
			mpfr_init2(ty,  4*wF);
			mpfr_init2(tr, 1+wF);
			fpx.getMPFR(tx);
			fpr.getMPFR(tr);

			mpfr_log(ty, tr, GMP_RNDN);
			mpfr_div(ty, ty, tx, GMP_RNDN);
			mpfr_exp(ty, ty, GMP_RNDN);
			FPNumber fpy(wE, wF, ty);
			y=fpy.getSignalValue();
			mpfr_clears(tx, ty, tr, NULL);
		}

		tc->addInput("X", x);
		tc->addInput("Y", y);
		/* Get correct outputs */
		emulate(tc);
		return tc;
	}

}
