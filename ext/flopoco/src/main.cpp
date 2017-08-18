/*
  the FloPoCo command-line interface
  
  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Authors : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr
            Bogdan Pasca, Bogdan.Pasca@ens-lyon.org

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved.

*/

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <mpfr.h>
#include <cstdlib>



#include "FloPoCo.hpp"


#define BRIGHT 1
#define RED 31
#define OPER 32
#define NEWOPER 32
#define PARAM 34
#define OP(op,paramList)             {cerr << "    "; printf("%c[%d;%dm",27,1,OPER); cerr <<  op; printf("%c[%dm",27,0); cerr<< " "; printf("%c[%d;%dm",27,1,PARAM); cerr << paramList; printf("%c[%dm\n",27,0); } 
#define NEWOP(op,paramList)          {cerr << "    "; printf("%c[%d;%dm",27,1,NEWOPER); cerr <<  op; printf("%c[%dm",27,0); cerr<< " "; printf("%c[%d;%dm",27,1,PARAM); cerr << paramList; printf("%c[%dm\n",27,0); } 


using namespace std;
using namespace flopoco;

// Global variables, useful in this main to avoid parameter passing


	string filename="flopoco.vhdl";
	string cl_name=""; // used for the -name option
	Target* target;
	
	//------------ Resource Estimation --------------------------------
	int reLevel;
	bool resourceEstimationDebug = false;
	//-----------------------------------------------------------------
	
	//------------------ Floorplanning --------------------------------
	bool floorplanning = false;
	bool floorplanningDebug = false;
	ostringstream floorplanMessages;
	//-----------------------------------------------------------------


extern void random_usage(char *name, string opName);
	
extern bool random_parseCommandLine(
	int argc, char *argv[], Target *target,
	std::string opname, int &i
);



void usage(char *name, string opName = ""){
	bool full = (opName=="");

	if ( full ){
		cerr << "\nUsage: "<<name<<" <operator specification list>\n" ;
		cerr << "Each operator specification is one of: \n";
	}
	if ( full || opName == "UserDefinedOperator")
	    OP( "UserDefinedOperator","param0 param1");
	if ( full )
		cerr << "    ____________ SHIFTERS/LZOC _________________________________________________\n";

	if ( full || opName == "LeftShifter") 
		OP("LeftShifter","wIn MaxShift");
	if ( full || opName == "RightShifter") 
		OP("RightShifter","wIn MaxShift");
	if ( full || opName == "LZOC") 	
		OP("LZOC","wIn");
	if ( full || opName == "LZOCShifter") 	
		OP("LZOCShifter","wIn wOut");
	if ( full || opName == "LZCShifter") 	
		OP("LZCShifter","wIn wOut");
	if ( full || opName == "LOCShifter") 	
		OP("LOCShifter","wIn wOut");
	if ( full || opName == "LZOCShifterSticky") 		
		OP("LZOCShifterSticky","wIn wOut");
	if ( full || opName == "LZCShifterSticky") 		
		OP("LZCShifterSticky","wIn wOut");
	if ( full || opName == "LOCShifterSticky") 			
		OP("LOCShifterSticky","wIn wOut");

	if ( full )
		cerr << "    ____________ ADDERS/SUBTRACTERS ____________________________________________\n";

	if ( full || opName == "IntAdder"){ 	
		OP("IntAdder","wIn");
		cerr << "      Integer adder, possibly pipelined\n";
	}
	if ( full || opName == "IntAdderExpert" || opName == "IntAdder"){ 		
		OP("IntAdderExpert","wIn optimizeType srl implementation bufferedInputs inputDelay");
		cerr << "      Integer adder, multple parameters, possibly pipelined\n";
		cerr << "      optimizeType=<0,1,2,3> 0=LUT 1=REG 2=SLICE 3=LATENCY\n";
		cerr << "      srl=<0,1> Allow SRLs\n";
		cerr << "      implementation=<-1,0,1,2> -1=optimizeType dependent,\n";  
		cerr << "                                 0=Classical, 1=Alternative, 2=Short-Latency\n";
		cerr << "      bufferedInputs=<0,1>\n";
		cerr << "      inputDelay\n";
	}
	if ( full || opName == "IntAdder" || opName == "LongIntAdderAddAddMux")
		OP("LongIntAdderAddAddMux","wIn generation");
	if ( full || opName == "IntAdder" || opName == "LongIntAdderCmpAddInc")
		OP("LongIntAdderCmpAddInc","wIn generation");
	if ( full || opName == "IntAdder" || opName == "LongIntAdderCmpCmpAdd")	
		OP("LongIntAdderCmpCmpAdd","wIn generation");
	if ( full || opName == "IntAdder" || opName == "LongIntAdderAddAddMux" || opName == "LongIntAdderCmpAddInc" || opName == "LongIntAdderCmpCmpAdd")
		cerr << "      generation 1 are the 2010 ver., 2 are the 2011 ver.\n";

	if ( full || opName == "IntAdder" || opName == "IntDualSub"){
		OP("IntDualSub","wIn opType");
		cerr << "      Integer adder/subtracter or dual subtracter, possibly pipelined\n";
		cerr << "      opType: if 1, compute X-Y and X+Y; if 0, compute X-Y and Y-X \n";
	}

	if ( full || opName == "IntAdder" || opName == "IntNAdder" || opName == "IntMultiAdder") {
		OP("IntMultiAdder","wIn N");
		cerr << "      Multi-operand addition, possibly pipelined.\n"; 
		cerr << "        Either IntNAdder or IntCompressorTree are generated \n";
	}
	
	if ( full || opName == "IntAdder" || opName == "IntNAdder"){
		OP("IntNAdder","wIn N");
		cerr << "      Multi-operand addition, possibly pipelined\n";
	}

	if ( full || opName == "IntAdder" || opName == "IntCompressorTree" || opName == "NewCompressorTree"){
		OP("IntCompressorTree","wIn N");
		cerr << "      Multi-operand addition using compressor trees, possibly pipelined\n";
		OP("NewCompressorTree","wOut [height list]");
		cerr << "      Multi-operand addition using compressor trees, the operands being\n";
		cerr << "      packed putting bits of same significance together; the i-th\n";
		cerr << "      element of the height list is the numbers of bits of signifance i\n";
		cerr << "      to be added together.\n";
	}

	if ( full || opName == "PopCount"){
		OP("PopCount","wIn");
		cerr << "      Population count using a table\n";
	}
	if ( full || opName == "BasicCompressor"){
		OP("BasicCompressor","[height list]");
		cerr << "      Population count using a table\n";
	}


	if ( full )	
		cerr << "    ____________ INTEGER MULTIPLIERS/SQUARER/KARATSUBA _________________________\n";

	 if ( full || opName == "IntMultiplier"){
	 	OP("IntMultiplier","wInX wInY wOut signed DSP_threshold enableSupertiles");
	 	cerr << "      Integer multiplier of two integers X and Y of sizes wInX and wInY \n";
	 	cerr << "      Result is faithfully truncated to wOut bits  (wOut=0 means: full multiplier)\n";
	 	cerr << "      signed=0: unsigned multiplier;     signed=1: signed inputs, signed outputs \n";
	 	cerr << "      0 <= DSP_threshold <= 1;  proportion of a DSP block's area that may be left unused\n";
		cerr << "      enableSuperTiles=0 =>  lower latency, higher logic cost; enableSuperTiles=1=> lower logic cost, longer latency \n";
	 }
	 
	 if ( full || opName == "FixSinPoly"){
	 	OP("FixSinPoly","msbIn lsbIn truncated msbOut lsbOut signed");
	 	cerr << "      Operator computing X-X^3/6 for an integer X of size given by msbIn and lsbIn \n";
	 	cerr << "      Result is faithfully truncated to the format msbOut lsbOut\n";
	 	cerr << "      signed=0: unsigned;     signed=1: signed inputs, signed outputs \n";
	 	cerr << "      truncated=0: full result;     truncated=1: truncated result \n";
	 }
	 
	 if ( full || opName == "FixXPow3Div6"){
	 	OP("FixXPow3Div6","msbIn lsbIn msbOut lsbOut signed");
	 	cerr << "      Operator computing X^3/6 for an integer X of size given by msbIn and lsbIn \n";
	 	cerr << "      Result is faithfully truncated to the format msbOut lsbOut\n";
	 	cerr << "      signed=0: unsigned;     signed=1: signed inputs, signed outputs \n";
	 }
	 
	 if ( full || opName == "IntConstDiv3"){
	 	OP("IntConstDiv3", "wIn d alpha nbZeros");
	 	cerr << "      Operator computing X/3 for an integer X of size wIn \n";
	 	cerr << "      The division is performed on X_new=x_{n}00x_{n-1}00...x_{1}00x_{0}, where x_{i}, are the bits of the number inputted\n";
	 	cerr << "      Result is faithfully truncated to 3*wIn\n";
	 	cerr << "      d=the integer constant to divide by\n";
	 	cerr << "      alpha=the size of the chunks of numbers to take from X and to tabulate (if unsure about the size, set to -1)\n";
		cerr << "      nbZeros=the number of zeros to interleave in the original number\n";
	 }

	 if ( full  || opName == "IntMultiplier" || opName == "IntMultAdd"){
	 	OP("IntMultAdd","w signedIO DSPThreshold");
	 	cerr << "      integer  R=A+X*Y where X and Y are of size w, A and R are of size 2w \n";
	 	cerr << "      signedIO: if 0, unsigned IO; if 1, signedIO \n";
		cerr << "      0 <= DSPThreshold <= 1; shows how much of a DSP block's area is acceptable to be left unused\n";
	 }


	if ( full || opName == "IntMultiplier" || opName == "IntKaratsuba"){			
		OP ("IntKaratsuba","wIn");
		cerr << "      integer multiplier of two integers X and Y of sizes wIn. 17 < wIn <= 68\n";	
	}
	if ( full || opName == "IntMultiplier" || opName == "IntSquarer"){			
		OP ("IntSquarer","wIn");
		cerr << "      integer squarer. For now wIn <=68 \n";		
	}
	if ( full || opName == "IntMultiplier" || opName == "IntPower"){
		OP ("IntPower","wIn n");
		cerr << "      Integer power of n, unsigned, with precision wIn (NPY)\n";
	}
	if ( full || opName == "IntMultiplier" || opName == "IntConstMult"){				
		OP( "IntConstMult","w c");
		cerr << "      Integer constant multiplier using shift-and-add: w - input size, c - the constant\n";
	}
	if ( full || opName == "IntMultiplier" || opName == "IntIntKCM"){					
		OP( "IntIntKCM","w c signedInput useBitheap");
		cerr << "      Integer constant multiplier using KCM: w - input size, c - the constant\n";
		cerr << "      		useBitheap - if true, operator is uses bit heaps\n";
	}
#ifdef HAVE_SOLLYA
	if ( full || opName == "FixRealKCM"){					
		OP( "FixRealKCM","lsbIn msbIn signedInput lsbOut constant useBitheap");
		cerr << "      Faithful multiplier of a fixed-point input by a real constant\n";
		cerr << "      The constant is provided as a Sollya expression, e.g \"log(2)\"\n";
		cerr << "      The multiplier might or might not us bit heaps, based on the value of the useBitheap parameter\n";
	}
	
	if ( full || opName == "FixFIR"){
		OP("FixFIR","p useBitheap taps [coeff list]");
		cerr << "      A faithful FIR on an (1,p) fixed-point format\n";
		cerr << "      The filter may, or may not use bit heaps\n";
	}
	
	if ( full || opName == "FixDCT2"){
		OP("FixDCT2","p taps current_index");
		cerr << "      A DCT2 on a (1,p) fixed-point format\n";
		cerr << "      The filter uses bit heaps\n";
	}

	if ( full || opName == "FixHalfSine"){
		OP("FixHalfSine","p taps");
		cerr << "      An half sine filter on a (1,p) fixed-point format\n";
		cerr << "      The filter uses bit heaps\n";
	}

	if ( full || opName == "FixRCF"){
		OP("FixRCF","p taps alpha");
		cerr << "      A Raised Cosine filter on a (1,p) fixed-point format, with a roll-off factor alpha\n";
		cerr << "	   alpha=1 by default";
		cerr << "      The filter uses bit heaps\n";
	}

	if ( full || opName == "FixRRCF"){
		OP("FixRRCF","p taps alpha");
		cerr << "      A Root Raised Cosine filter on a (1,p) fixed-point format, with a roll-off factor alpha\n";
		cerr << "	   alpha=1 by default";
		cerr << "      The filter uses bit heaps\n";
	}


#endif // HAVE_SOLLYA


	if ( full || opName == "IntConstDiv"){					
		OP( "IntConstDiv","n d alpha");
		cerr << "      Euclidean division of input of size n by d (returning q and r)\n";
		cerr << "      Algorithm uses radix 2^alpha,   alpha=-1 means a sensible default.\n";
	}

	if ( full || opName == "IntConstRem"){					
		OP( "IntConstDiv","n d alpha");
		cerr << "      Remainder of Euclidean division of input of size n by d\n";
		cerr << "      Algorithm uses radix 2^alpha,   alpha=-1 means a sensible default.\n";
	}

	if ( full || opName == "FPConstDiv"){					
		OP( "FPConstDiv","wE wF d");
		cerr << "      Floating-point division by the (small) integer d\n";
		OP( "FPConstDivExpert","wE wF d e alpha");
		cerr << "      Floating-point division by  d.2^e, where d is a small integer\n";
		cerr << "      Algorithm uses radix 2^alpha,   alpha=-1 means a sensible default. \n";  // 
	}



	if ( full )
	cerr << "    ____________ FLOATING-POINT OPERATORS ______________________________________\n";

	if ( full || opName == "FPPipeline"){					
	NEWOP("FPPipeline", "filename wE wF");
		cerr << "      Assembles the computational datapath described by the input file.\n";
		cerr << "      The precision for all operators is given wE wF\n";
		cerr << "      The datapath is described in an untyped and untimed Python-like syntax\n";
		cerr << "      Supported operations: +,-,*,\\,sqr(),sqrt(),exp(),log() \n";
		cerr << "      Examples:\n";
		cerr << "      Jacobi1D:              |  Horner:              | 2D Norm:\n";
		cerr << "      j = (a0+a1+a2)*0.333;  |  p = a0+x*(a2+x*a2);  | r = sqrt(sqr(x0-x1)+sqr(y0-y1));\n";
		cerr << "      output j;              |  output p;            | output r;\n";
	}

	if ( full || opName == "Fix2FP"){					
		OP("Fix2FP","LSB MSB Signed wE wF");
		cerr << "      Convert a 2's complement fixed-point number in the bit range MSB...LSB \n";
		cerr << "      into floating-point\n";
	}	

	if ( full || opName == "FP2Fix"){					
		OP("FP2Fix","wE wF LSB MSB Signed trunc");
		cerr << "      Convert a floating point number into a 2's complement fixed-point number in \n";
		cerr << "      the bit range MSB...LSB, truncated or not\n";
	}	

	if ( full || opName == "FPAdder"){					
		NEWOP( "FPAdder","wE wF");
		cerr << "      Floating-point adder (default architecture is now single-path) \n";
	}	
	if ( full || opName == "FPAdder" || opName == "FPAdderDualPath"){					
		OP( "FPAdderDualPath","wE wF");
		cerr << "      Floating-point adder with dual-path architecture (shorter latency, larger area)\n";
	}
	if ( full || opName == "FPAdder" || opName == "FPAdder3Input"){					
		NEWOP( "FPAdder3Input","wE wF");
		cerr << "      A 3-operand floating-point adder\n";
	}
	if ( full || opName == "FPAdder" || opName == "FPAddSub"){					
		NEWOP( "FPAddSub","wE wF");
		cerr << "      A floating-point adder/subtractor, useful e.g for butterfly circuits\n";
	}

	if ( full || opName == "FPMultiplier"){					
		OP( "FPMultiplier","wE wF_in wF_out");
		cerr << "      Standard floating-point multiplier, supporting different in/out precision  \n";
	}
	if ( full  || opName == "FPMultiplier" || opName == "FPMultiplierFaithful"){					
		OP( "FPMultiplierFaithful","wE wF_in wF_out");
		cerr << "      Faithfully rounded floating-point multiplier, (saves resources for large mantissa sizes) \n";
	}
	if ( full || opName == "FPMultiplier" || opName == "FPMultiplierKaratsuba"){						
		OP( "FPMultiplierKaratsuba","wE wF_in wF_out");
		cerr << "      Floating-point multiplier, supporting different in/out precision. \n";
		cerr << "      Mantissa multiplier uses Karatsuba\n";
	}
	if ( full || opName == "FPMultiplier" || opName == "FPMultiplierExpert"){						
		OP( "FPMultiplierExpert","wE wFX_in wFY_in wF_out correctRounding DSPThreshold optTimeInMinutes");
		cerr << "      Floating-point multiplier, supporting different in/out precision. \n";
	}
	if ( full || opName == "FPMultiplier" || opName == "FPSquarer"){					
		OP( "FPSquarer","wE wFin wFout");
		cerr << "      Floating-point squarer \n";
	}
	if ( full || opName == "FPDiv"){					
		OP( "FPDiv","wE wF");
		cerr << "      Floating-point divider \n";
	}
	if ( full || opName == "FPSqrt"){					
		OP("FPSqrt","wE wF");
		cerr << "      Floating-point square root, implemented using digit recurrence\n";
		cerr << "      (no DSP, long latency)\n";
	}
#if 0
	if ( full || opName == "FP2DNorm"){					
		OP("FP2DNorm","wE wF");
		cerr << "      Floating-point 2D norm\n";
	}
#endif
#ifdef HAVE_SOLLYA
	if ( full || opName == "FPSqrt" || opName == "FPSqrtPoly"){					
		OP( "FPSqrtPoly","wE wF degree");
		cerr << "      Floating-point square root, using polynomial approximation \n";
		cerr << "      (DSP-based, shorter latency and higher frequency for large wF than FPSqrt)\n";
		cerr << "      degree (1,...k) polynomial degree. Higher degree => more DSP, less BRAM\n";
		cerr << "      Warning: faithful rounding (last-bit accurate) but not correctly rounded\n";
	}
#endif // HAVE_SOLLYA
#ifdef HAVE_SOLLYA
	if ( full || opName == "FPConstMult"){					
		OP( "FPConstMult","wE_in wF_in wE_out wF_out wC constant_expr");
		cerr << "      Faithful floating-point constant multiplier\n";
		cerr << "      last argument is a Sollya expression between double quotes,e.g.\"exp(pi/2)\".\n";
		cerr << "      If wC>1, it is the size in bits on which the constant must be evaluated.\n";
		cerr << "      If wC=0 the size is computed for a faithful result.\n";
	}
	if ( full || opName == "FPConstMult" || opName == "CRFPConstMult"){					
		OP( "CRFPConstMult","wE_in wF_in wE_out wF_out constant_expr");
		cerr << "      Correctly-rounded floating-point constant multiplier\n";
		cerr << "      The constant is provided as a Sollya expression, between double quotes.\n";
	}	
#endif // HAVE_SOLLYA
	if ( full || opName == "FPConstMult" || opName == "FPConstMultRational"){					
		OP( "FPConstMultRational","wE_in wF_in wE_out wF_out a b");
		cerr << "      Floating-point constant multiplier by a rational a/b\n";
		cerr << "      Useful for multiplications by simple rational constants such as 2/3 or 1/9\n";
	}
	if ( full || opName == "FPConstMult" || opName == "FPConstMultExpert"){					
		OP("FPConstMultExpert","wE_in wF_in wE_out wF_out cst_sgn cst_exp cst_int_sig");
		cerr << "      Floating-point constant multiplier\n";
		cerr << "      The constant is provided as integral significand and integral exponent.\n";
	}
	if ( full || opName == "FPConstMult" || opName == "FPRealKCM"){					
		NEWOP("FPRealKCM","wE wF constantExpression");
		cerr << "      Floating-point constant multiplier using the KCM algorithm\n";
		cerr << "      last argument is a Sollya expression between double quotes,e.g.\"exp(pi/2)\".\n";
	}
	if ( full || opName == "LongAcc"){					
		OP( "LongAcc","wE_in wF_in MaxMSB_in LSB_acc MSB_acc");
		cerr << "      Long fixed-point accumulator\n";
	}		
	if ( full || opName == "LongAcc" || opName == "LongAcc2FP"){					
		OP( "LongAcc2FP","LSB_acc MSB_acc wE_out wF_out");
		cerr << "      Post-normalisation unit for LongAcc \n";
	}
	if ( full || opName == "DotProduct"){					
		OP( "DotProduct","wE wFX wFY MaxMSB_in LSB_acc MSB_acc DSPThreshold");
		cerr << "      Floating-point dot product unit.\n";
		cerr << "      0 <= DSPThreshold <= 1; shows how much of a DSP block's area is acceptable to be left unused\n";
	}
#ifdef HAVE_SOLLYA
	if ( full || opName == "FPExp"){					
		OP( "FPExp","wE wF");
		cerr << "      Floating-point exponential function. For expert mode, use FPExpExpert.\n";
	}
	if( full  || opName == "FPExpExpert") {
		OP( "FPExpExpert","wE wF k d g fullInput");
		cerr << "      Floating-point exponential function, expert mode\n";
		cerr << "      k: number of bits addressing the table;   d: degree of the polynomial;\n";
		cerr << "      g: number of guard bits\n";
		cerr << "      fullInput (boolean): if 1, accepts extended (typically unrounded) input\n";
		cerr << "      DSP_threshold (float): between 0 and 1, 1 meaning that all small multipliers go to DSPs, 0 meaning that only multipliers filling DSPs go to DSP\n";
	}
	if ( full || opName == "FPLog"){					
		OP( "FPLog","wE wF InTableSize");
		cerr << "      Floating-point logarithm function;\n";
		cerr << "      InTableSize is the numbers of bits to input to the tables. \n";
		cerr << "      O defaults to something sensible\n";
	}
	if ( full || opName == "FPPowr" || opName == "FPPow"  || opName == "FPPower"|| opName == "FPPowerExpert" ){					
		OP( "FPPow","wE wF");
		cerr << "      Floating-point pow from C99 and IEEE-754-2008; \n";
		OP( "FPPowr","wE wF");
		cerr << "      Floating-point powr from IEEE-754-2008; \n";
		OP( "FPPowerExpert","wE wF  type LogTableSize ExpTableSize ExpDegree");
		cerr << "      Floating-point pow (if type=0) or powr (if type=1) function from IEEE-754-2008; \n";
		cerr << "      Example of parameters:  8 23 0 10 10 2 3 3 (simple), 11 52 0 12 12 2 33  (double)\n";
		cerr << "      For each parameter, 0 should default to something sensible\n";
	}
#endif // HAVE_SOLLYA
	if ( full || opName == "OutputIEEE"){					
		OP( "OutputIEEE","wEI wFI wEO wFO");
		cerr << "      Conversion from FloPoCo to IEEE-754-like floating-point formats\n";
	}
	if ( full || opName == "InputIEEE"){					
		OP( "InputIEEE","wEI wFI wEO wFO");
		cerr << "      Conversion from IEEE-754-like to FloPoCo floating-point formats\n";
	}
	if ( full ){
		cerr << "    ____________ FIXED-POINT OPERATORS __________________________________________\n";
	}
#ifdef HAVE_SOLLYA
	if ( full || opName == "CordicAtan2"){
		NEWOP( "CordicAtan2","w");
		cerr << "      Computes atan(x/y) as a=(angle in radian)/pi so a in [-1,1[;\n";
		cerr << "      w is the size of both inputs and outputs, all being two's complement signals\n";
	}
	if ( full || opName == "CordicSinCos" || opName == "FixSinOrCos" || opName == "SinCos"){
		NEWOP( "CordicSinCos","wIn wOut reduced");
		cerr << "      Computes (1-2^(-w)) sin(pi*x) and (1-2^(-w)) cos(pi*x) for x in [-1,1[, ;\n";
		cerr << "      wIn and wOut are the fixed-point precision of inputs and outputs (not counting the sign bit)\n";
		cerr << "      reduced : if 1,  reduced number of iterations at the cost of two multiplications \n";
	}
	if ( full || opName == "FixSinCos" || opName == "FixSinOrCos" || opName == "SinCos"){
		NEWOP( "FixSinCos","w");
		cerr << "      For a fixed-point 2's complement input x in [-1,1[, calculates\n";
		cerr << "      (1-2^(-w))*{sin,cos}(pi*x); w is the precision not counting the sign bit\n";
	}
	if ( full || opName == "CordicSinCos" || opName == "FixSinOrCos" || opName == "SinCos"){
		NEWOP( "FixSinOrCos","w d");
		cerr << "      Computes (1-2^(-w)) sin(pi*x) or (1-2^(-w)) cos(pi*x) for x in -[1,1[, ;\n";
		cerr << "      w is the fixed-point precision of inputs and outputs, not counting the sign bit\n";
		cerr << "      d: degree of the polynomial-based method (-1 should default to something sensible)\n";
	}
	if ( full ){
		cerr << "    ____________ GENERIC FUNCTION EVALUATORS ____________________________________\n";
		cerr << "      We provide several methods to evaluate a fixed-point function\n";
	}
	if ( full || opName == "FunctionTable" || opName == "FixFunction"){					
		OP( "FunctionTable","function wI lsbO msbO");
		cerr << "      Simple tabulation of a function defined on [0,1)\n";
		cerr << "      wI: input width (also weight of input LSB), \n";
		cerr << "      lsbO and msbO: weights of output LSB and MSB,\n";
		cerr << "      function: sollya-syntaxed function to implement, e.g. \"sin(x*Pi/2)\" \n";
	}
	if ( full || opName == "FunctionEvaluator" || opName == "FixFunction"){					
		OP( "FunctionEvaluator","function wI lsbO degree");
		cerr << "      Horner polynomial approximation, DSP based if available\n";
		cerr << "      wI - input width (also weight of input LSB), lsbO - weight of output LSB,\n";
		cerr << "      degree - degree of polynomial approximation (typically 2 to 5),\n";
		cerr << "      function - sollya-syntaxed function to implement, between double quotes\n";
		cerr << "      example: flopoco FunctionEvaluator \"sin(x*Pi/2)\" 16 16 3\n";
	}
	if ( full || opName == "HOTBM" || opName == "FixFunction"){					
		OP( "HOTBM","function wI wO degree");
		cerr << "      High-Order Table-Based Method for fixed-point functions (NPY)\n";
		cerr << "      wI - input width, wO - output width, degree - degree of polynomial approx\n";
		cerr << "      function - sollya-syntaxed function to implement, between double quotes\n";
	}
	if ( full || opName == "HOTBM" || opName == "HOTBMFX" || opName == "FixFunction"){					
		OP( "HOTBMFX","function wE_in wF_in wE_out wF_out degree");
		cerr << "      Same as HOTBM, with explicit fixed-point formats (NPY)\n";
		cerr << "      Note: input is unsigned, output is signed.\n";
	}
	if ( full || opName == "HOTBM" || opName == "HOTBMRange" || opName == "FixFunction"){					
		OP( "HOTBMRange","function wI wO degree xmin xmax scale");
		cerr << "      Same as HOTBM, with explicit range and scale (NPY)\n";
		cerr << "      xmin xmax - bounds of the input range, mapped to [0,1[\n";
		cerr << "      scale - scaling factor to apply to the function output\n";
	}
#endif // HAVE_SOLLYA

	if ( full )
	cerr << "    ____________ COMPLEX OPERATORS ______________________________________\n";
	if ( full || opName == "Complex" || opName == "FixComplexAdder"){					
		OP( "FixComplexAdder","wI wO");
		cerr << "   Complex adder for two's complement fixed-point numbers\n";
		cerr << "      wI: number of integer bits\n";
		cerr << "      wO: number of output bits\n";
	}
	if ( full || opName == "Complex" || opName == "FixComplexMultiplier"){					
		OP( "FixComplexMultiplier","wI wO threshold");
		cerr << "   Complex multiplier for two's complement fixed-point numbers\n";
		cerr << "      wI: number of input bits\n";
		cerr << "      wO: number of output bits\n";
		cerr << "      threshold: between 0 and 1.  the closer to 0, the less DSP-consuming\n";
	}



#ifdef HAVE_LNS
	if ( full )
	cerr << "    ____________ LNS OPERATORS _________________________________________________\n";

	if ( full || opName == "LNSAddSub" || opName == "LNSMul" || opName == "LNSDiv" || opName == "LNSSqrt"){
		cerr << "    Common parameters:\n";
		cerr << "      wE - width of integral part of exponent. Typically from 4 to 8.\n";
		cerr << "           Negative values allowed at your own risk.\n";
		cerr << "      wF - width of fractional part of exponent. Typically from 8 to 20.\n";
	}

	if ( full || opName == "LNSAddSub"){
		OP( "LNSAddSub","wE wF");
		cerr << "      Addition in Logarithmic Number System.\n";
	}	
	if ( full || opName == "LNSMul"){
		OP( "LNSMul","wE wF");
		cerr << "      LNS multiplication.\n";
	}
	if ( full || opName == "LNSDiv"){
		OP( "LNSDiv","wE wF");
		cerr << "      LNS division.\n";
	}
	if ( full || opName == "LNSSqrt"){
		OP( "LNSSqrt","wE wF");
		cerr << "      LNS square root.\n";
	}
	if ( full || opName == "AtanPow"){
		OP("AtanPow", "wE wF o");
		cerr << "      (4/pi)*atan(2^x) function.\n";
	}	
#endif // HAVE_LNS
	if ( full )
		cerr << "    ____________ Pseudo-Random Number Generation ______________________________\n";

	// Delegate to operators from random
	random_usage(name, opName);

	if ( full )
	cerr << "    ____________ APPLICATIONS __________________________________________________\n";

	if ( full || opName == "Collision"){
		OP("Collision","wE wF opt");;
		cerr << "       A collision detection operator, computes the predicate X²+Y²+Z²<R2\n";
		cerr << "       opt: assemble FP operators if 0, optimized architecture if 1 \n";
	}
	if ( full || opName == "FPSumOfSquares"){
		OP("FPSumOfSquares","wE wF opt");;
		cerr << "       A floating-point sum-of-3 squares operator\n";
		cerr << "       opt: assemble FP operators if 0, optimized architecture if 1 \n";
	}	
	// cerr << "  Applications: \n";
	// cerr << "    CoilInductance LSBI MSBI wEIn wFIn MaxMSBO LSBO MSBO FilePath\n";
	// cerr << "       \n";
	//To be removed from command line interface
	// cerr << "    CoordinatesTableX wIn LSB MSB FilePath\n";
	// cerr << "    CoordinatesTableY wIn LSB MSB FilePath\n";
	// cerr << "    CoordinatesTableZ wIn LSB MSB FilePath\n";
	//=====================================================

#if 0

	if ( full || opName == "TaMaDiModule" || opName == "TaMaDi"){
		OP("TaMaDiModule","wP degree nbOfIterations IntervalIDWidth widthComp nbOfPE inFiFoDepth PEFiFoDepth outFiFoDepth");;
		cerr << "       Assembles a TaMaDiMultimodule formed of multiple TaMaDi cores\n";
		cerr << "       wP             : internal fixed-point working precision \n";
		cerr << "       degree         : polyonomial degree used in the TaMaDiCore \n";
		cerr << "       nbOfIterations : the number of allowed iterations per TaMaDiCore\n";
		cerr << "       IntervalIDWidth: the number of bits required for storing the interval ID\n";
		cerr << "       widthComp      : the width of the pattern detector \n";
		cerr << "       nbOfPE         : the number of TaMaDiCores instantiated in the multi-module \n";
		cerr << "       inFiFoDepth    : the multi-module's input FIFO depth \n";
		cerr << "       PEFiFoDepth    : each TaMaDiCore is associated with its own output FIFO; \n";
		cerr << "                        this argument sets its depth; \n";
		cerr << "       outFiFoDepth   : the multi-module's output FIFO depth \n";
	}	
	if ( full || opName == "TaMaDiModuleDummyWrapper"|| opName == "TaMaDi"){
		OP("TaMaDiModuleDummyWrapper","wP degree nbOfIterations IntervalIDWidth widthComp nbOfPE inFiFoDepth PEFiFoDepth outFiFoDepth");;
		cerr << "       Assembles a TaMaDiMultimodule and adds a serial interface\n";
	}	
	if ( full || opName == "TaMaDiDeserializer"|| opName == "TaMaDi"){
		OP("TaMaDiDeserializer","wP degree nbOfIterations IntervalIDWidth widthComp nbOfPE inFiFoDepth PEFiFoDepth outFiFoDepth");;
		cerr << "       Module used to deserialize the data from the DMA FIFO into initialization packets\n";
	}	
	if ( full || opName == "TaMaDiModuleWrapperInterface"|| opName == "TaMaDi"){
		OP("TaMaDiModuleWrapperInterface","wP degree nbOfIterations IntervalIDWidth widthComp nbOfPE inFiFoDepth PEFiFoDepth outFiFoDepth");;
		cerr << "       Adds the counter-based interface for the credit-based dispatch scheme\n";
	}	
	if ( full || opName == "TaMaDiModuleDispatcherInterface"|| opName == "TaMaDi"){
		OP("TaMaDiModuleDispatcherInterface","wP degree nbOfIterations IntervalIDWidth widthComp nbOfPE inFiFoDepth PEFiFoDepth outFiFoDepth InterfaceInFiFoDepth InterfaceOutFiFoDepth");;
		cerr << "       The Dispatcher module which dispatches initialization packets to the interface-wrapped TaMaDiMultiModules\n";
		cerr << "       InterfaceInFiFoDepth    : the Dispatcher's input FIFO depth (will be filled-in by the Deserializer) \n";
		cerr << "       InterfaceOutFiFoDepth   : the Dispatcher's output FIFO depth (will be filled by the TaMaDiMultimodules with HR cases\n";
	}	
	if ( full || opName == "TaMaDiSystem"|| opName == "TaMaDi"){
		OP("TaMaDiSystem","wP degree nbOfIterations IntervalIDWidth widthComp nbOfPE inFiFoDepth PEFiFoDepth outFiFoDepth InterfaceInFiFoDepth InterfaceOutFiFoDepth nbOfModules");;
		cerr << "       Assembles a full system with a Deserializer, Dispatcher and several multi-modules\n";
		cerr << "       wP             : internal fixed-point working precision \n";
		cerr << "       degree         : polyonomial degree used in the TaMaDiCore \n";
		cerr << "       nbOfIterations : the number of allowed iterations per TaMaDiCore\n";
		cerr << "       IntervalIDWidth: the number of bits required for storing the interval ID\n";
		cerr << "       widthComp      : the width of the pattern detector \n";
		cerr << "       nbOfPE         : the number of TaMaDiCores instantiated in the multi-module \n";
		cerr << "       inFiFoDepth    : the multi-module's input FIFO depth \n";
		cerr << "       PEFiFoDepth    : each TaMaDiCore is associated with its own output FIFO; \n";
		cerr << "                        this argument sets its depth; \n";
		cerr << "       outFiFoDepth   : the multi-module's output FIFO depth \n";
		cerr << "       InterfaceInFiFoDepth    : the Dispatcher's input FIFO depth (will be filled-in by the Deserializer) \n";
		cerr << "       InterfaceOutFiFoDepth   : the Dispatcher's output FIFO depth (will be filled by the TaMaDiMultimodules with HR cases\n";
		cerr << "       nbOfModules             : the number of multi-modules instantiated in the TaMaDiSystem"<<endl;
	}	
	if ( full || opName == "TaMaDiCore" || opName == "TaMaDi"){
		OP("TaMaDiCore","wP degree nbOfIterations IntervalIDWidth widthComp");;
		cerr << "       The TaMaDiCore performing the polynomial evaluation based on the tabulated differences\n";
	}	

#endif

	if ( full || opName=="TestBench" ){
		cerr << "    ____________ TEST-BENCH ____________________________________________________\n";
		OP ("TestBench","n");
		cerr << "       Behavorial test bench for the preceding operator\n";
		cerr << "       This test bench will include standard tests, plus n random tests.\n";
		OP( "TestBenchFile","n");
		cerr << "       Behavorial test bench for the preceding operator\n";
		cerr << "       This test bench will include standard tests, plus n random tests.\n";
		cerr << "       Inputs and outputs are stored in a file to reduce VHDL compilation time.\n";
		cerr << "       if n=-2, an exhaustive test is generated (use only for small operators).\n";
	}
	
	if ( full || opName=="Wrapper"){
		cerr << "    ____________ WRAPPER _______________________________________________________\n";
		OP ("Wrapper","");
		cerr << "       Wraps the preceding operator between registers\n";
		cerr << "(NPY) Not pipelined yet\n";
	}
	if ( full || opName=="options"){
		cerr << "________________________________________________________________________________\n";
		cerr << "________________ OPTIONS________________________________________________________\n";
		cerr << "General options that should only appear before any operator specification:\n";
		cerr << "   -outputfile=<output file name>           (default=flopoco.vhdl)\n";
		cerr << "   -target=<Spartan3|Virtex4|Virtex5|Virtex6|StratixII|StratixIII|StratixIV|StratixV|CycloneII|CycloneIII|CycloneIV|CycloneV>      (default=Virtex5)\n";
		cerr << "Options affecting the operators that follow them:\n";
		cerr << "   -pipeline=<yes|no>                       (default=yes)\n";
		cerr << "   -frequency=<target frequency in MHz>     (default=400)\n";
		cerr << "   -clockenable=<yes|no>                    (default is no)\n";
		cerr << "   -DSP_blocks=<yes|no>\n";
		cerr << "       optimize for the use of DSP blocks   (default=yes)\n";
		cerr << "   -name=<entity name>\n";
		cerr << "       defines the name of the VHDL entity of the next operator\n";
		cerr << "   -resourceEstimation=level\n";
		cerr << "       level=0 disables resource estimation (default)\n";
		cerr << "       level=1..3 larger number means more details\n";
		cerr << "   -floorplanning=<yes|no>\n";
		cerr << "       generate a floorplan (experimental, Xilinx only)\n";
		cerr << "Debugging options, affecting the operators that follow them:\n";
		cerr << "   -verbose=<0|1|2|3>     verbosity level. 0: no output (default), 1: basic info, 3: full debug \n";
		cerr << "   -reDebugging=<yes|no>  debug output for resource estimation (default=no)\n";
		cerr << "   -flpDebugging=<yes|no> debug output for floorplanning (default=no)\n";
	}
	exit (EXIT_FAILURE);
}

int checkStrictlyPositive(char* s, char* cmd) {
	int n=atoi(s);
	if (n<=0){
		cerr<<"ERROR: got "<<s<<", expected strictly positive number."<<endl;
		usage(cmd);
	}
	return n;
}

int checkPositiveOrNull(char* s, char* cmd) {
	int n=atoi(s);
	if (n<0){
		cerr<<"ERROR: got "<<s<<", expected positive-or-null number."<<endl;
		usage(cmd);
	}
	return n;
}

bool checkBoolean(char* s, char* cmd) {
	int n=atoi(s);
	if (n!=0 && n!=1) {
		cerr<<"ERROR: got "<<s<<", expected a boolean (0 or 1)."<<endl;
		usage(cmd);
	}
	return (n==1);
}


int checkSign(char* s, char* cmd) {
	int n=atoi(s);
	if (n!=0 && n!=1) {
		cerr<<"ERROR: got "<<s<<", expected a sign bit (0 or 1)."<<endl;
		usage(cmd);
	}
	return n;
}

void addOperator(Operator *op) {
	if(cl_name!="")	{
		cerr << "Updating entity name to: " << cl_name << endl;
		op->changeName(cl_name);
		cl_name="";
	}
	op->addToGlobalOpList();
}



bool parseCommandLine(int argc, char* argv[]){
	if (argc<2) {
		usage(argv[0]); 
		return false;
	}

	Operator* op;
	int i=1;
	cl_name="";
	do {
		string opname(argv[i++]);
		if(opname[0]=='-'){
			string::size_type p = opname.find('=');
			if (p == string::npos) {
				cerr << "ERROR: Option missing an = : "<<opname<<endl; 
				return false;
			} else {
				string o = opname.substr(1, p - 1), v = opname.substr(p + 1);
				if (o == "outputfile") {
					if(!target->getGlobalOpListRef()->empty())
						cerr << "WARNING: global option "<<o<<" should come before any operator specification" << endl; 
					filename=v;
				}
				else if (o == "verbose") {
					verbose = atoi(v.c_str()); // there must be a more direct method of string
					if (verbose<0 || verbose>4) {
						cerr<<"ERROR: verbose should be 1, 2 or 3,    got "<<v<<"."<<endl;
						usage(argv[0], "options");
					}
				}
				else if (o == "target") {
					Target* oldTarget=target;
					if(!target->getGlobalOpListRef()->empty()){
								cerr<<"ERROR: target should be changed before any component is defined"<<endl; 
								usage(argv[0],"options");
					}
					if(v=="Virtex4") target=new Virtex4();
					else if (v=="Virtex5") target=new Virtex5();
					else if (v=="Virtex6") target=new Virtex6();
					else if (v=="Spartan3") target=new Spartan3();
					else if (v=="StratixII") target=new StratixII();
					else if (v=="StratixIII") target=new StratixIII();
					else if (v=="StratixIV") target=new StratixIV();
					else if (v=="StratixV") target=new StratixV();
					else if (v=="CycloneII") target=new CycloneII();
					else if (v=="CycloneIII") target=new CycloneIII();
					else if (v=="CycloneIV") target=new CycloneIV();
					else if (v=="CycloneV") target=new CycloneV();
					else {
						cerr<<"ERROR: unknown target: "<<v<<endl;
						usage(argv[0],"options");
					}
					// if previous options had changed it
					target->setFrequency(oldTarget->frequency());
					target->setUseHardMultipliers(oldTarget->useHardMultipliers());
					if (oldTarget->isPipelined()) 
						target->setPipelined();
					else 
						target->setNotPipelined();
					target->setClockEnable(oldTarget->useClockEnable());
					delete(oldTarget);
				}

				else if (o == "pipeline") {
					if(v=="yes") target->setPipelined();
					else if(v=="no")  target->setNotPipelined();
					else {
						cerr<<"ERROR: pipeline option should be yes or no,    got "<<v<<"."<<endl; 
						usage(argv[0],"options");
					}
				}
				else if (o == "clockenable") {
					if(v=="yes") target->setClockEnable(true);
					else if(v=="no")  target->setClockEnable(false);
					else {
						cerr<<"ERROR: clockenable option should be yes or no,    got "<<v<<"."<<endl; 
						usage(argv[0],"options");
					}
				}
				else if (o == "frequency") {
					int freq = atoi(v.c_str());
					if (freq>1 && freq<10000) {
						target->setFrequency(1e6*(double)freq);
						if(verbose) 
							cerr << "Frequency set to "<<target->frequency()<< " Hz" <<endl; 
					}
					else {
						cerr<<"WARNING: frequency out of reasonible range, ignoring it."<<endl; 
					}
				}
				else if (o == "DSP_blocks") {
					if(v=="yes") target->setUseHardMultipliers(true);
					else if(v=="no")  target->setUseHardMultipliers(false);
					else {
						cerr<<"ERROR: DSP_blocks option should be yes or no,    got "<<v<<"."<<endl; 
						usage(argv[0],"options");
					}
				}
				else if (o == "name") {
					cl_name=v; // TODO?  check it is a valid VHDL entity name 
				}
				//------------ Resource Estimation ---------------------
				else if (o == "resourceEstimation") {
					int estimation = atoi(v.c_str());
					if (estimation>=0 && estimation<=3) {
						reLevel = estimation;
						
						if(verbose) 
							cerr << ((estimation!=0) ? "Using resource estimation" : "Not using resource estimation") <<endl; 
					}
					else {
						cerr<<"WARNING: not a valid option for resource estimation."<<endl; 
					}
				}
				//------------------------------------------------------
				//--------- Resource Estimation Degugging --------------
				else if (o == "reDebugging") {
					if(v=="yes") resourceEstimationDebug = true;
					else if(v=="no")  resourceEstimationDebug = false;
					else {
						cerr<<"ERROR: resource estimation debugging option should be yes or no, got "<<v<<"."<<endl; 
						usage(argv[0],"options");
					}
				}
				//------------------------------------------------------
				//------------------------------------------------------
				//------------------ Floorplanning ---------------------
				else if (o == "floorplanning") {
					if(v=="yes") floorplanning = true;
					else if(v=="no")  floorplanning = false;
					else {
						cerr<<"ERROR: floorplanning option should be yes or no, got "<<v<<"."<<endl; 
						usage(argv[0],"options");
					}
				}
				//------------------------------------------------------
				//--------------- Floorplanning Debugging --------------
				else if (o == "flpDebugging") {
					if(v=="yes") floorplanningDebug = true;
					else if(v=="no")  floorplanningDebug = false;
					else {
						cerr<<"ERROR: floorplanning debugging option should be yes or no, got "<<v<<"."<<endl; 
						usage(argv[0],"options");
					}
				}
				//------------------------------------------------------
				else { 	
					cerr << "Unknown option "<<o<<endl; 
					return false;
				}
			}
		}
		
		else if(opname=="IntIntKCM"){
			int nargs = 4;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int w = atoi(argv[i++]);
				mpz_class mpc(argv[i++]);
				int signedInput = checkBoolean(argv[i++], argv[0]);
				int useBitheap = checkBoolean(argv[i++], argv[0]);

				op = new IntIntKCM(target, w, mpc, signedInput, useBitheap);
				addOperator(op);
			}        
		}
		
		else if(opname=="IntConstMult"){
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int w = atoi(argv[i++]);
				mpz_class mpc(argv[i++]);
				op = new IntConstMult(target, w, mpc);
				addOperator(op);
			}        
		}

		else if(opname=="IntConstDiv"){
			int nargs = 3;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int n = checkStrictlyPositive(argv[i++], argv[0]);
				int d = checkStrictlyPositive(argv[i++], argv[0]);
				int alpha = atoi(argv[i++]);
				op = new IntConstDiv(target, n, d, alpha);
				addOperator(op);
			} 
		}

		else if(opname=="IntConstRem"){
			int nargs = 3;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int n = checkStrictlyPositive(argv[i++], argv[0]);
				int d = checkStrictlyPositive(argv[i++], argv[0]);
				int alpha = atoi(argv[i++]);
				op = new IntConstDiv(target, n, d, alpha, true);
				addOperator(op);
			} 
		}


		else if(opname=="FPConstMultRational"){
			int nargs = 6;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wE_in = checkStrictlyPositive(argv[i++], argv[0]);
				int wF_in = checkStrictlyPositive(argv[i++], argv[0]);
				int wE_out = checkStrictlyPositive(argv[i++], argv[0]);
				int wF_out = checkStrictlyPositive(argv[i++], argv[0]);
				int a  = atoi(argv[i++]); 
				int b  = checkStrictlyPositive(argv[i++], argv[0]); 
				op = new FPConstMult(target, wE_in, wF_in, wE_out, wF_out, a, b);
				addOperator(op);
			}        
		} 	

		else if(opname=="FPConstMultExpert"){
			int nargs = 7;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wE_in = checkStrictlyPositive(argv[i++], argv[0]);
				int wF_in = checkStrictlyPositive(argv[i++], argv[0]);
				int wE_out = checkStrictlyPositive(argv[i++], argv[0]);
				int wF_out = checkStrictlyPositive(argv[i++], argv[0]);
				int cst_sgn  = checkSign(argv[i++], argv[0]); 
				int cst_exp  = atoi(argv[i++]); // TODO no check on this arg
				mpz_class cst_sig(argv[i++]);
				op = new FPConstMult(target, wE_in, wF_in, wE_out, wF_out, cst_sgn, cst_exp, cst_sig);
				addOperator(op);
			}        
		} 	
		else if(opname=="FPConstDiv"){
			int nargs = 3;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wE = checkStrictlyPositive(argv[i++], argv[0]);
				int wF = checkStrictlyPositive(argv[i++], argv[0]);
				int d = checkStrictlyPositive(argv[i++], argv[0]);
				op = new FPConstDiv(target, wE, wF, wE, wF, d, 0, -1); // exponent 0, alpha = default
				addOperator(op);
			}        
		}
		else if(opname=="FPConstDivExpert"){
			int nargs = 4;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wE = checkStrictlyPositive(argv[i++], argv[0]);
				int wF = checkStrictlyPositive(argv[i++], argv[0]);
				int d = checkStrictlyPositive(argv[i++], argv[0]);
				int e = atoi(argv[i++]);
				int alpha = atoi(argv[i++]);
				op = new FPConstDiv(target, wE, wF, wE, wF, d, e, alpha);
				addOperator(op);
			}        
		} 	


#ifdef HAVE_SOLLYA
		else if(opname=="FixRealKCM"){
			int nargs = 6;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int lsbIn = atoi(argv[i++]);
				int msbIn = atoi(argv[i++]);
				int signedInput = checkBoolean(argv[i++], argv[0]);
				int lsbOut = atoi(argv[i++]);
				string constant = argv[i++];
				int useBitheap = checkBoolean(argv[i++], argv[0]);
				op = new FixRealKCM(target, lsbIn, msbIn, signedInput, lsbOut, constant, 1.0, emptyDelayMap, useBitheap);
				addOperator(op);
			}
		}
		
		else if(opname=="FixRealKCMExpert"){ // hidden, for debug
			int nargs = 6;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int lsbIn = atoi(argv[i++]);
				int msbIn = atoi(argv[i++]);
				int signedInput = checkBoolean(argv[i++], argv[0]);
				int lsbOut = atoi(argv[i++]);
				string constant = argv[i++];
				float targetUlpError = atof(argv[i++]);
				op = new FixRealKCM(target, lsbIn, msbIn, signedInput, lsbOut, constant, targetUlpError);
				addOperator(op);
			}        
		}
		else if(opname=="FPRealKCM"){
			int nargs = 3;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wE = atoi(argv[i++]);
				int wF = atoi(argv[i++]);
				string constant = argv[i++];
				op = new FPRealKCM(target, wE, wF, constant);
				addOperator(op);
			}        
		}
		else if(opname=="CRFPConstMult"){ 
			int nargs = 5;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else { 
				int wE_in = checkStrictlyPositive(argv[i++], argv[0]);
				int wF_in = checkStrictlyPositive(argv[i++], argv[0]);
				int wE_out = checkStrictlyPositive(argv[i++], argv[0]);
				int wF_out = checkStrictlyPositive(argv[i++], argv[0]);
				string constant = argv[i++];
				op = new CRFPConstMult(target, wE_in, wF_in, wE_out, wF_out, constant);
				addOperator(op);
			}        
		} 	
		else if(opname=="FPConstMult"){ 
			int nargs = 6;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else { 
				int wE_in = checkStrictlyPositive(argv[i++], argv[0]);
				int wF_in = checkStrictlyPositive(argv[i++], argv[0]);
				int wE_out = checkStrictlyPositive(argv[i++], argv[0]);
				int wF_out = checkStrictlyPositive(argv[i++], argv[0]);
				int wF_C = atoi(argv[i++]);
				string constant = argv[i++];
				op = new FPConstMult(target, wE_in, wF_in, wE_out, wF_out, wF_C, constant);
				addOperator(op);
			}        
		} 	
#endif // HAVE_SOLLYA
		else if(opname=="LeftShifter"){
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn = checkStrictlyPositive(argv[i++], argv[0]);
				int maxShift = checkStrictlyPositive(argv[i++], argv[0]);
				map<string, double> inputDelays;
				inputDelays["X"]=0;
				inputDelays["S"]=0;
				op = new Shifter(target, wIn, maxShift, Shifter::Left);
				addOperator(op);
			}    
		}
		else if(opname=="RightShifter"){
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn = checkStrictlyPositive(argv[i++], argv[0]);
				int maxShift = checkStrictlyPositive(argv[i++], argv[0]);
				map<string, double> inputDelays;
				inputDelays["X"]=0;
				inputDelays["S"]=0;
				op = new Shifter(target, wIn, maxShift, Shifter::Right, inputDelays);
				addOperator(op);
			}
		}
		else if(opname=="LZOC"){
			int nargs = 1;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn = checkStrictlyPositive(argv[i++], argv[0]);
				op = new LZOC(target, wIn);
				addOperator(op);
			}
		}
		else if(opname=="LZOCShifter"){
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn  = checkStrictlyPositive(argv[i++], argv[0]);
				int wOut = checkStrictlyPositive(argv[i++], argv[0]);
				if (wIn > 1){
					op = new LZOCShifterSticky(target, wIn, wOut, intlog2(wIn), 0, -1);
					addOperator(op);
				}else
					cerr << "wIn must be > 1"<<endl;
			}
		}
		else if(opname=="LZCShifter"){
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn  = checkStrictlyPositive(argv[i++], argv[0]);
				int wOut = checkStrictlyPositive(argv[i++], argv[0]);
				if (wIn > 1){
					op = new LZOCShifterSticky(target, wIn, wOut, intlog2(wIn), 0, 0);
					addOperator(op);
				}else
					cerr << "wIn must be > 1"<<endl;
			}
		}
		else if(opname=="LOCShifter"){
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn  = checkStrictlyPositive(argv[i++], argv[0]);
				int wOut = checkStrictlyPositive(argv[i++], argv[0]);
				if (wIn > 1){
					op = new LZOCShifterSticky(target, wIn, wOut, intlog2(wIn), 0, 1);
					addOperator(op);
				}else
					cerr << "wIn must be > 1"<<endl;
			}
		}
		else if(opname=="LZOCShifterSticky"){
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn  = checkStrictlyPositive(argv[i++], argv[0]);
				int wOut = checkStrictlyPositive(argv[i++], argv[0]);
				if (wIn > 1){
					op = new LZOCShifterSticky(target, wIn, wOut, intlog2(wIn), 1, -1);
					addOperator(op);
				}else
					cerr << "wIn must be > 1"<<endl;
			}
		}
		else if(opname=="LZCShifterSticky"){
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn  = checkStrictlyPositive(argv[i++], argv[0]);
				int wOut = checkStrictlyPositive(argv[i++], argv[0]);
				if (wIn > 1){
					op = new LZOCShifterSticky(target, wIn, wOut, intlog2(wIn), 1, 0);
					addOperator(op);
				}else
					cerr << "wIn must be > 1"<<endl;
			}
		}
		else if(opname=="LOCShifterSticky"){
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn  = checkStrictlyPositive(argv[i++], argv[0]);
				int wOut = checkStrictlyPositive(argv[i++], argv[0]);
				if (wIn > 1){
					op = new LZOCShifterSticky(target, wIn, wOut, intlog2(wIn), 1, 1);
					addOperator(op);
				}else
					cerr << "wIn must be > 1"<<endl;
			}
		}
		else if(opname=="UserDefinedOperator"){
                        // the UserDefinedOperator expects 2 parameters
			int nargs = 2;
			if (i+nargs > argc)
                            /* if there is less than 2 parameters, we output 
                              the help information for Flopoco */
				usage(argv[0],opname);
			else {
				int param0 = checkStrictlyPositive(argv[i++], argv[0]);
				int param1 =  atoi(argv[i++]);
				op = new UserDefinedOperator(target,param0,param1);
				addOperator(op);
			}    
		}
		else if(opname=="IntAdder"){
			int nargs = 1;
			if (i+nargs > argc)
				usage(argv[0], opname);
			else {
				int wIn = checkStrictlyPositive(argv[i++], argv[0]);
				op = new IntAdder(target,wIn, inDelayMap("X",target->ffDelay() + target->localWireDelay()) );
				addOperator(op);
			}    
		}
		else if(opname=="IntComparator"){
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn = checkStrictlyPositive(argv[i++], argv[0]);
				int criteria = atoi(argv[i++]);
				
				op = new IntComparator(target,wIn,criteria,false,0);
				addOperator(op);
			}    
		}
		else if(opname=="IntConstComparator"){
			int nargs = 3;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn = checkStrictlyPositive(argv[i++], argv[0]);
				int criteria = atoi(argv[i++]);
				int constant = atoi(argv[i++]);
				
				op = new IntComparator(target,wIn,criteria, true, constant);
				addOperator(op);
			}    
		}
	
		else if(opname=="IntAdderExpert"){
			int nargs = 5;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn = checkStrictlyPositive(argv[i++], argv[0]);
				int type = atoi(argv[i++]);
				int srl = atoi(argv[i++]);
				int implementation = atoi(argv[i++]);
				int bufferedIn = atoi(argv[i++]);
				double inputDelay = atof(argv[i++]);
				map <string, double> delayMap;

				if (bufferedIn){
					delayMap["X"] = target->ffDelay() + target->localWireDelay() + 1.0e-25;
				}else{
					delayMap["X"] = inputDelay;
				}
				
				switch (type) {
					case 0: op = new IntAdder(target, wIn, delayMap, 0, srl, implementation); break; //lut optimized
					case 1: op = new IntAdder(target, wIn, delayMap, 1, srl, implementation); break; //reg
					case 2: op = new IntAdder(target, wIn, delayMap, 2, srl, implementation); break; //slice
					case 3: op = new IntAdder(target, wIn, delayMap, 3, srl, implementation); break; //latency
					default: op = new IntAdder(target,wIn, delayMap, 2, srl, implementation); break;
				}
				addOperator(op);
			}    
		}

		/* Multioperand adders */

		else if(opname=="IntMultiAdder"){
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn = checkStrictlyPositive(argv[i++], argv[0]);
				int N   = checkStrictlyPositive(argv[i++], argv[0]);
				op = new IntMultiAdder(target,wIn,N);
				addOperator(op);
			}    
		}

		else if(opname=="IntNAdder"){
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn = checkStrictlyPositive(argv[i++], argv[0]);
				int N   = checkStrictlyPositive(argv[i++], argv[0]);
				op = new IntNAdder(target, wIn, N, inDelayMap("X0",target->localWireDelay()));
				addOperator(op);
			}    
		}

		else if(opname=="IntCompressorTree"){
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn = checkStrictlyPositive(argv[i++], argv[0]);
				int N   = checkStrictlyPositive(argv[i++], argv[0]);
				op = new IntCompressorTree(target,wIn,N);
				addOperator(op);
			}    
		}

		else if(opname=="NewCompressorTree"){
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wOut = checkStrictlyPositive(argv[i++], argv[0]);
				if (i+wOut > argc)
					usage(argv[0],opname);
				else {
					std::vector<unsigned> heights (wOut, 0);
					for (int j = 0; j < wOut; j++) {
						heights[wOut-1-j] = atoi(argv[i++]);
					}
					op = new NewCompressorTree(target,heights);
					addOperator(op);
				}
			}
		}

		else if(opname=="PopCount"){
			int nargs = 1;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn = checkStrictlyPositive(argv[i++], argv[0]);
				op = new PopCount(target, wIn);
				addOperator(op);
			}
		}
		
		else if(opname=="BasicCompressor")
		{
			int wOut = checkStrictlyPositive(argv[i++], argv[0]);
			if (i+wOut > argc)
				usage(argv[0],opname);
			else 
			{
				std::vector<int> height (wOut, 0);
				for (int j = 0; j < wOut; j++) 
				{
					height[wOut-1-j] = atoi(argv[i++]);
				}
				op = new BasicCompressor(target,height);
				addOperator(op);
			}
		}
		

#ifdef HAVE_SOLLYA
		else if(opname=="FixFIR")
		{
			if (i+3 > argc)
				usage(argv[0],opname);
			else {
				int p = checkStrictlyPositive(argv[i++], argv[0]);
				int useBitheap = checkBoolean(argv[i++], argv[0]);
				int taps = checkStrictlyPositive(argv[i++], argv[0]);
				if (i+taps > argc)
					usage(argv[0],opname);
				else {
					std::vector<string> coeff;
					for (int j = 0; j < taps; j++) 
						{
							coeff.push_back(argv[i++]);
						}
					op = new FixFIR(target, p, coeff, useBitheap);
					addOperator(op);
				}
			}
		}
		
		else if(opname=="FixDCT2")
		{
			int nargs = 3;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int p 		= checkStrictlyPositive(argv[i++], argv[0]);
				int taps 	= checkStrictlyPositive(argv[i++], argv[0]);
				int k 		= checkPositiveOrNull(argv[i++], argv[0]);
				
				op = new FixDCT2(target, p, taps, k, true);
				addOperator(op);
			}
		}
		
		else if(opname=="FixHalfSine")
		{
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int p 		= checkStrictlyPositive(argv[i++], argv[0]);
				int taps 	= checkStrictlyPositive(argv[i++], argv[0]);

				op = new FixHalfSine(target, p, taps, true);
				addOperator(op);
			}
		}

		else if(opname=="FixRCF")
		{
			int nargs = 3;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int p 			= checkStrictlyPositive(argv[i++], argv[0]);
				int taps 		= checkStrictlyPositive(argv[i++], argv[0]);
				double alpha	= atof(argv[i++]);

				op = new FixRCF(target, p, taps, alpha, true);
				addOperator(op);
			}
		}

		else if(opname=="FixRRCF")
		{
			int nargs = 3;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int p 			= checkStrictlyPositive(argv[i++], argv[0]);
				int taps 		= checkStrictlyPositive(argv[i++], argv[0]);
				double alpha	= atof(argv[i++]);

				op = new FixRRCF(target, p, taps, alpha, true);
				addOperator(op);
			}
		}


#endif

		/* Exploration of other fast adders */
		else if(opname=="IntAdderSpecific"){ //Hidden
			int nargs = 1;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn = checkStrictlyPositive(argv[i++], argv[0]);
				op = new IntAdderSpecific(target,wIn);
				addOperator(op);
			}    
		}
		else if(opname=="IntComparatorSpecific"){ //Hidden
			int nargs = 1;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn = checkStrictlyPositive(argv[i++], argv[0]);
				int type = atoi(argv[i++]);
				op = new IntComparatorSpecific(target,wIn,type);
				addOperator(op);
			}    
		}
		else if(opname=="CarryGenerationCircuit"){ //Hidden
			int nargs = 1;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn = checkStrictlyPositive(argv[i++], argv[0]);
				op = new CarryGenerationCircuit(target,wIn);
				addOperator(op);
			}    
		}
		/* interface */
		else if(opname=="LongIntAdderAddAddMux"){ //AAM
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn = checkStrictlyPositive(argv[i++], argv[0]);
				int g   = checkStrictlyPositive(argv[i++], argv[0]);
				if (g==1)
					op = new LongIntAdderAddAddMuxGen1(target,wIn);
				else if (g==2)
					op = new LongIntAdderAddAddMuxGen2(target,wIn, inDelayMap("X",target->ffDelay() + target->localWireDelay() ));
				else 
					throw "Generation parameter is either 1 or 2";
				addOperator(op);
			}    
		}
		else if(opname=="LongIntAdderMuxNetwork"){ //Mux network
			int nargs = 1;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn = checkStrictlyPositive(argv[i++], argv[0]);
				op = new LongIntAdderMuxNetwork(target,wIn);
				addOperator(op);
			}    
		}
		else if(opname=="LongIntAdderCmpCmpAdd"){ //CCA
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn = checkStrictlyPositive(argv[i++], argv[0]);
				int g   = checkStrictlyPositive(argv[i++], argv[0]);
				if (g==1)
					op = new LongIntAdderCmpCmpAddGen1(target,wIn);
				else if (g==2)
					op = new LongIntAdderCmpCmpAddGen2(target,wIn, inDelayMap("X",target->ffDelay() + target->localWireDelay() ));
				else 
					throw "Generation parameter is either 1 or 2";
				addOperator(op);
			}    
		}
		else if(opname=="LongIntAdderCmpAddInc"){ //CAI
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn = checkStrictlyPositive(argv[i++], argv[0]);
				int g   = checkStrictlyPositive(argv[i++], argv[0]);
				if (g==1)
					op = new LongIntAdderCmpAddIncGen1(target,wIn);
				else if (g==2)
					op = new LongIntAdderCmpAddIncGen2(target,wIn, inDelayMap("X",target->ffDelay() + target->localWireDelay() ));
				else 
					throw "Generation parameter is either 1 or 2";
				addOperator(op);
			}    
		}
		/*---------------------------------------------------------*/
		
		//HIDDEN
		else if(opname=="IntDualSub"){
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn = checkStrictlyPositive(argv[i++], argv[0]);
				int opType = checkBoolean(argv[i++], argv[0]);
				op = new IntDualSub(target,wIn,opType);
				addOperator(op);
			}    
		}
		else if(opname=="IntMultiplier"){
			int nargs = 6;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wInX		    = checkStrictlyPositive(argv[i++], argv[0]);
				int wInY		    = checkStrictlyPositive(argv[i++], argv[0]);
				int wOut		    = atoi(argv[i++]);
				int signedIO	    =  checkBoolean(argv[i++], argv[0]);
				float DSPThreshold			= atof(argv[i++]);
				int buildSuperTiles =  checkBoolean(argv[i++], argv[0]);
				IntMultiplier* mul=new IntMultiplier(target, wInX, wInY, wOut, signedIO, DSPThreshold, emptyDelayMap,buildSuperTiles);
				op = mul;
				addOperator(op);
			}
		}

		else if(opname=="IntMultAdd"){
			int nargs = 3;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn    = checkStrictlyPositive(argv[i++], argv[0]);
				int signedIO    = checkBoolean(argv[i++], argv[0]);
				float DSPThreshold = atof(argv[i++]);
				int wInY=wIn;
				int wInX=wIn;
				int wA=2*wIn;
#if 0
				int wInY    = checkStrictlyPositive(argv[i++], argv[0]);
				int wInA    = checkStrictlyPositive(argv[i++], argv[0]);
				int wOut    = atoi(argv[i++]);
				int buildSuperTiles = false ;// checkBoolean(argv[i++], argv[0]);
#endif
				FixMultAdd* op=new FixMultAdd(target, wInX /*wX*/, wInY /*wY*/, wA /*wA*/, wA /*wOut*/, wA-1 /*msbP*/, 0 /*lsbA*/, signedIO, DSPThreshold);
				addOperator(op);
			}
		}
		
		else if(opname=="FixSinPoly"){
			int nargs = 6;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int msbIn    	= atoi(argv[i++]);
				int lsbIn    	= atoi(argv[i++]);
				int truncated   = checkBoolean(argv[i++], argv[0]);
				int msbOut   	= atoi(argv[i++]);
				int lsbOut   	= atoi(argv[i++]);
				int signedIO    = checkBoolean(argv[i++], argv[0]);
				
				FixSinPoly* mul = new FixSinPoly(target, msbIn, lsbIn, (truncated==0 ? false : true), msbOut, lsbOut, (signedIO==0 ? false : true));
				op = mul;
				addOperator(op);
			}
		}
		
		else if(opname=="FixXPow3Div6"){
			int nargs = 5;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int msbIn    	= atoi(argv[i++]);
				int lsbIn    	= atoi(argv[i++]);
				int msbOut   	= atoi(argv[i++]);
				int lsbOut   	= atoi(argv[i++]);
				int signedIO    =  checkBoolean(argv[i++], argv[0]);
				
				FixXPow3Div6* mul = new FixXPow3Div6(target, msbIn, lsbIn, msbOut, lsbOut, signedIO);
				op = mul;
				addOperator(op);
			}
		}
		
		else if(opname=="IntConstDiv3"){
			int nargs = 4;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn    	= atoi(argv[i++]);
				int d	    	= atoi(argv[i++]);
				int alpha   	= atoi(argv[i++]);
				int nbZeros   	= checkPositiveOrNull(argv[i++], argv[0]);
				
				IntConstDiv3* div = new IntConstDiv3(target, wIn, d, alpha, nbZeros);
				op = div;
				addOperator(op);
			}
		}

		else if(opname=="IntKaratsuba"){
			int nargs = 1;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wIn = checkStrictlyPositive(argv[i++], argv[0]);
				op = new IntKaratsuba(target, wIn);
				addOperator(op);
			}    
		}   
		// For the FPAdder the default is the single-path design
		else if(opname=="FPAdder"){ 
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wE = checkStrictlyPositive(argv[i++], argv[0]);
				int wF = checkStrictlyPositive(argv[i++], argv[0]);
				
				op = new FPAdderSinglePath(target, wE, wF, wE, wF, wE, wF);
				addOperator(op);
			}
		}		
		else if(opname=="FPAdderDualPath"){
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wE = checkStrictlyPositive(argv[i++], argv[0]);
				int wF = checkStrictlyPositive(argv[i++], argv[0]);
				
				op = new FPAdderDualPath(target, wE, wF, wE, wF, wE, wF);
				addOperator(op);
			}
		}
		else if(opname=="FPAdder3Input"){
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wE = checkStrictlyPositive(argv[i++], argv[0]);
				int wF = checkStrictlyPositive(argv[i++], argv[0]);
				
				op = new FPAdder3Input(target, wE, wF);
				addOperator(op);
			}
		}	
		else if(opname=="FPAddSub"){
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wE = checkStrictlyPositive(argv[i++], argv[0]);
				int wF = checkStrictlyPositive(argv[i++], argv[0]);
				
				op = new FPAddSub(target, wE, wF, wE, wF, wE, wF);
				addOperator(op);
			}
		}	
		else if(opname=="FPFMAcc"){
			int nargs = 3;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wE = checkStrictlyPositive(argv[i++], argv[0]);
				int wF = checkStrictlyPositive(argv[i++], argv[0]);
				int adderLatency = checkStrictlyPositive(argv[i++], argv[0]);
				op = new FPFMAcc(target, wE, wF, adderLatency);
				addOperator(op);
			}
		}		
#ifdef HAVE_SOLLYA
		else if(opname=="FPJacobi"){
			int nargs = 5;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wE = checkStrictlyPositive(argv[i++], argv[0]);
				int wF = checkStrictlyPositive(argv[i++], argv[0]);
				int l0 = checkStrictlyPositive(argv[i++], argv[0]);
				int l1 = checkStrictlyPositive(argv[i++], argv[0]);
				int l2 = checkStrictlyPositive(argv[i++], argv[0]);
				int ver = atoi(argv[i++]);
				op = new FPJacobi(target, wE, wF, l0, l1, l2,ver);
				addOperator(op);
			}
	}	
#endif // HAVE_SOLLYA

		else if(opname=="FixComplexAdder"){
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wI = checkStrictlyPositive(argv[i++], argv[0]);
				int wF = checkStrictlyPositive(argv[i++], argv[0]);
				op = new FixedComplexAdder(target, wI, wF, true); // signed
				addOperator(op);
			}
		}


		else if(opname=="FixComplexMultiplier"){
			int nargs = 3;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wI = checkStrictlyPositive(argv[i++], argv[0]);
				int wO = checkStrictlyPositive(argv[i++], argv[0]);
				float DSPThreshold = atof(argv[i++]);
				op = new FixedComplexMultiplier(target, wI, wO, DSPThreshold, true);
				addOperator(op);
			}
		}


#if 0
		else if(opname=="TaMaDiModule"){
			int nargs = 9;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wP = checkStrictlyPositive(argv[i++], argv[0]);
				int degree = checkStrictlyPositive(argv[i++], argv[0]);
				int numberOfIterations = checkStrictlyPositive(argv[i++], argv[0]);
				int widthOfIntervalID  =  checkStrictlyPositive(argv[i++], argv[0]);
				int widthComp          =  checkStrictlyPositive(argv[i++], argv[0]);
				int n = checkStrictlyPositive(argv[i++], argv[0]); //number of pe
				int inFifoDepth = checkStrictlyPositive(argv[i++], argv[0]);
				int peFifoDepth = checkStrictlyPositive(argv[i++], argv[0]);
				int outFifoDepth = checkStrictlyPositive(argv[i++], argv[0]);
				
				op = new TaMaDiModule(target, wP, degree, numberOfIterations, widthOfIntervalID, widthComp, n, inFifoDepth, peFifoDepth, outFifoDepth);
				addOperator(op);
			}
		}	
		else if(opname=="TaMaDiModuleDummyWrapper"){
			int nargs = 9;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wP = checkStrictlyPositive(argv[i++], argv[0]);
				int degree = checkStrictlyPositive(argv[i++], argv[0]);
				int numberOfIterations = checkStrictlyPositive(argv[i++], argv[0]);
				int widthOfIntervalID =  checkStrictlyPositive(argv[i++], argv[0]);
				int widthComp          =  checkStrictlyPositive(argv[i++], argv[0]);
				int n = checkStrictlyPositive(argv[i++], argv[0]); //number of pe
				int inFifoDepth = checkStrictlyPositive(argv[i++], argv[0]);
				int peFifoDepth = checkStrictlyPositive(argv[i++], argv[0]);
				int outFifoDepth = checkStrictlyPositive(argv[i++], argv[0]);
				
				op = new TaMaDiModuleDummyWrapper(target, wP, degree, numberOfIterations, widthOfIntervalID, widthComp, n, inFifoDepth, peFifoDepth, outFifoDepth);
				addOperator(op);
			}
		}
		else if(opname=="TaMaDiDeserializer"){
			int nargs = 9;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wP = checkStrictlyPositive(argv[i++], argv[0]);
				int degree = checkStrictlyPositive(argv[i++], argv[0]);
				int numberOfIterations = checkStrictlyPositive(argv[i++], argv[0]);
				int widthOfIntervalID =  checkStrictlyPositive(argv[i++], argv[0]);
				int widthComp          =  checkStrictlyPositive(argv[i++], argv[0]);
				int n = checkStrictlyPositive(argv[i++], argv[0]); //number of pe
				int inFifoDepth = checkStrictlyPositive(argv[i++], argv[0]);
				int peFifoDepth = checkStrictlyPositive(argv[i++], argv[0]);
				int outFifoDepth = checkStrictlyPositive(argv[i++], argv[0]);
				
				op = new TaMaDiDeserializer(target, wP, degree, numberOfIterations, widthOfIntervalID, n, inFifoDepth, peFifoDepth, outFifoDepth);
				addOperator(op);
			}
		}		
		else if(opname=="TaMaDiModuleWrapperInterface"){
			int nargs = 9;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wP = checkStrictlyPositive(argv[i++], argv[0]);
				int degree = checkStrictlyPositive(argv[i++], argv[0]);
				int numberOfIterations = checkStrictlyPositive(argv[i++], argv[0]);
				int widthOfIntervalID =  checkStrictlyPositive(argv[i++], argv[0]);
				int widthComp          =  checkStrictlyPositive(argv[i++], argv[0]);
				int n = checkStrictlyPositive(argv[i++], argv[0]); //number of pe
				int inFifoDepth = checkStrictlyPositive(argv[i++], argv[0]);
				int peFifoDepth = checkStrictlyPositive(argv[i++], argv[0]);
				int outFifoDepth = checkStrictlyPositive(argv[i++], argv[0]);
				
				op = new TaMaDiModuleWrapperInterface(target, wP, degree, numberOfIterations, widthOfIntervalID, widthComp, n, inFifoDepth, peFifoDepth, outFifoDepth);
				addOperator(op);
			}
		}
		else if(opname=="TaMaDiDispatcherInterface"){
			int nargs = 11;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wP = checkStrictlyPositive(argv[i++], argv[0]);
				int degree = checkStrictlyPositive(argv[i++], argv[0]);
				int numberOfIterations = checkStrictlyPositive(argv[i++], argv[0]);
				int widthOfIntervalID =  checkStrictlyPositive(argv[i++], argv[0]);
				int widthComp          =  checkStrictlyPositive(argv[i++], argv[0]);
				int n = checkStrictlyPositive(argv[i++], argv[0]); //number of pe
				int inFifoDepth = checkStrictlyPositive(argv[i++], argv[0]);
				int peFifoDepth = checkStrictlyPositive(argv[i++], argv[0]);
				int outFifoDepth = checkStrictlyPositive(argv[i++], argv[0]);
				int InterfaceInFifoDepth = checkStrictlyPositive(argv[i++], argv[0]);
				int InterfaceOutFifoDepth = checkStrictlyPositive(argv[i++], argv[0]);
				
				op = new TaMaDiDispatcherInterface(target, wP, degree, numberOfIterations, widthOfIntervalID, widthComp,  n, inFifoDepth, peFifoDepth, outFifoDepth, InterfaceInFifoDepth, InterfaceOutFifoDepth);
				addOperator(op);
			}
		}
		else if(opname=="TaMaDiSystem"){
			int nargs = 12;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wP = checkStrictlyPositive(argv[i++], argv[0]);
				int degree = checkStrictlyPositive(argv[i++], argv[0]);
				int numberOfIterations = checkStrictlyPositive(argv[i++], argv[0]);
				int widthOfIntervalID =  checkStrictlyPositive(argv[i++], argv[0]);
				int widthComp          =  checkStrictlyPositive(argv[i++], argv[0]);
				int n = checkStrictlyPositive(argv[i++], argv[0]); //number of pe
				int inFifoDepth = checkStrictlyPositive(argv[i++], argv[0]);
				int peFifoDepth = checkStrictlyPositive(argv[i++], argv[0]);
				int outFifoDepth = checkStrictlyPositive(argv[i++], argv[0]);
				int InterfaceInFifoDepth = checkStrictlyPositive(argv[i++], argv[0]);
				int InterfaceOutFifoDepth = checkStrictlyPositive(argv[i++], argv[0]);
				int moduleCount = checkStrictlyPositive(argv[i++], argv[0]);
				
				op = new TaMaDiSystem(target, wP, degree, numberOfIterations, widthOfIntervalID, widthComp, n, inFifoDepth, peFifoDepth, outFifoDepth, InterfaceInFifoDepth, InterfaceOutFifoDepth, moduleCount );
				addOperator(op);
			}
		}
		else if(opname=="TaMaDiCore"){
			int nargs = 5;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wP = checkStrictlyPositive(argv[i++], argv[0]);
				int degree = checkStrictlyPositive(argv[i++], argv[0]);
				int numberOfIterations = checkStrictlyPositive(argv[i++], argv[0]);
				int widthOfIntervalID =  checkStrictlyPositive(argv[i++], argv[0]);
				int widthComp          =  checkStrictlyPositive(argv[i++], argv[0]);
				op = new TaMaDiCore(target, wP, degree, numberOfIterations, widthOfIntervalID, widthComp);
				addOperator(op);
			}
		}	
		else if(opname=="TaMaDiPriorityEncoder"){
			int nargs = 1;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int n = checkStrictlyPositive(argv[i++], argv[0]);
				op = new TaMaDiPriorityEncoder(target, n);
				addOperator(op);
			}
		}	
		else if(opname=="TaMaDiDecoder"){
			int nargs = 1;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int n = checkStrictlyPositive(argv[i++], argv[0]);
				op = new TaMaDiDecoder(target, n);
				addOperator(op);
			}
		}	
		else if(opname=="TaMaDiFIFO"){
			int nargs = 3;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int w = checkStrictlyPositive(argv[i++], argv[0]);
				int n = checkStrictlyPositive(argv[i++], argv[0]);
				int limit = atoi(argv[i++]);
				op = new TaMaDiFIFO(target, w, n, limit);
				addOperator(op);
			}
		}
		
		else if(opname=="TaMaDiShiftRegister"){
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int w = checkStrictlyPositive(argv[i++], argv[0]);
				int n = checkStrictlyPositive(argv[i++], argv[0]);
				op = new TaMaDiShiftRegister(target, w, n);
				addOperator(op);
			}
		}		
#endif

		else if(opname=="Fix2FP"){
			int nargs = 5;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int LSB = atoi(argv[i++]);
				int MSB = atoi(argv[i++]);
				int sign = atoi(argv[i++]);
				int wE = checkStrictlyPositive(argv[i++], argv[0]);
				int wF = checkStrictlyPositive(argv[i++], argv[0]);
				
				op = new Fix2FP(target, LSB, MSB, sign,wE, wF);
				addOperator(op);
			}
		}
		else if(opname=="FP2Fix"){
			int nargs = 6;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wE = checkStrictlyPositive(argv[i++], argv[0]);
				int wF = checkStrictlyPositive(argv[i++], argv[0]);
				int LSB = atoi(argv[i++]);
				int MSB = atoi(argv[i++]);
				int sign = atoi(argv[i++]);
				int trunc_p = checkBoolean(argv[i++], argv[0]);
				
				op = new FP2Fix(target, LSB, MSB, sign,wE, wF, trunc_p);
				addOperator(op);
			}
		}
		// else if(opname=="CoilInductance"){
		// 	int nargs = 7;
		// 	if (i+nargs > argc)
		// 		usage(argv[0],opname);
		// 	else {
		// 		int LSBI = atoi(argv[i++]);
		// 		int MSBI = atoi(argv[i++]);
		// 		int wE = checkStrictlyPositive(argv[i++], argv[0]);
		// 		int wF = checkStrictlyPositive(argv[i++], argv[0]);				
		// 		int MaxMSBO= atoi(argv[i++]);
		// 		int LSBO = atoi(argv[i++]);
		// 		int MSBO = atoi(argv[i++]);
		// 		char *pa=argv[i++];
		// 		cerr << "> CoilInductance  LSBI="<<LSBI<<", MSBI="<<MSBI<<",wEIn="<<wE<<",wFIn"<<wF<<", MaxMSBO="<<MaxMSBO<<", LSBO="<<LSBO<<", MSBO="<<MSBO<<" \n";
		// 		op = new CoilInductance(target, LSBI, MSBI,wE,wF,MaxMSBO,LSBO,MSBO,pa);
		// 		addOperator(op);
		// 	}
		// }
		// else if(opname=="CoordinatesTableX"){
		// 	int nargs = 4;
		// 	if (i+nargs > argc)
		// 		usage(argv[0],opname);
		// 	else {
		// 		int wIn = checkStrictlyPositive(argv[i++], argv[0]);
		// 		int LSB = atoi(argv[i++]);
		// 		int MSB = atoi(argv[i++]);
		// 		char *pa=argv[i++];
		// 		cerr << "> CoordinatesTableX, wIn="<<wIn<<", LSB="<<LSB<<", MSB="<<MSB<<" \n";
		// 		op = new CoordinatesTableX(target, wIn,LSB, MSB,pa);
		// 		addOperator(op);
		// 	}
		// }
		// else if(opname=="CoordinatesTableZ"){
		// 	int nargs = 4;
		// 	if (i+nargs > argc)
		// 		usage(argv[0],opname);
		// 	else {
		// 		int wIn = checkStrictlyPositive(argv[i++], argv[0]);
		// 		int LSB = atoi(argv[i++]);
		// 		int MSB = atoi(argv[i++]);
		// 		char *pa=argv[i++];
		// 		cerr << "> CoordinatesTableZ, wIn="<<wIn<<", LSB="<<LSB<<", MSB="<<MSB<<" \n";
		// 		op = new CoordinatesTableZ(target, wIn,LSB, MSB,pa);
		// 		addOperator(op);
		// 	}
		// }
		// else if(opname=="CoordinatesTableY"){
		// 	int nargs = 4;
		// 	if (i+nargs > argc)
		// 		usage(argv[0],opname);
		// 	else {
		// 		int wIn = checkStrictlyPositive(argv[i++], argv[0]);
		// 		int LSB = atoi(argv[i++]);
		// 		int MSB = atoi(argv[i++]);
		// 		char *pa=argv[i++];
		// 		cerr << "> CoordinatesTableY, wIn="<<wIn<<", LSB="<<LSB<<", MSB="<<MSB<<" \n";
		// 		op = new CoordinatesTableY(target, wIn,LSB, MSB,pa);
		// 		addOperator(op);
		// 	}
		// }
		else if(opname=="FPMultiplier"){
			int nargs = 3; 
			if (i+nargs > argc)
				usage(argv[0],opname);
			int wE = checkStrictlyPositive(argv[i++], argv[0]);
			int wFIn = checkStrictlyPositive(argv[i++], argv[0]);
			int wFOut = checkStrictlyPositive(argv[i++], argv[0]);
			op = new FPMultiplier(target, wE, wFIn, wE, wFIn, wE, wFOut, true /*normd*/, true /*CR*/);
			addOperator(op);
		} 
		else if(opname=="FPMultiplierFaithful"){
			int nargs = 3; 
			if (i+nargs > argc)
				usage(argv[0],opname);
			int wE = checkStrictlyPositive(argv[i++], argv[0]);
			int wFIn = checkStrictlyPositive(argv[i++], argv[0]);
			int wFOut = checkStrictlyPositive(argv[i++], argv[0]);
			op = new FPMultiplier(target, wE, wFIn, wE, wFIn, wE, wFOut, true, false);
			addOperator(op);
		}
		else if(opname=="FPMultiplierKaratsuba"){
			int nargs = 3; 
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wE = checkStrictlyPositive(argv[i++], argv[0]);
				int wFIn = checkStrictlyPositive(argv[i++], argv[0]);
				int wFOut = checkStrictlyPositive(argv[i++], argv[0]);
				op = new FPMultiplierKaratsuba(target, wE, wFIn, wE, wFIn, wE, wFOut, 1);
				addOperator(op);
			}
		}  
		else if(opname=="FPMultiplierExpert"){
			int nargs = 7; 
			if (i+nargs > argc)
				usage(argv[0],opname);
			int wE = checkStrictlyPositive(argv[i++], argv[0]);
			int wFXIn = checkStrictlyPositive(argv[i++], argv[0]);
			int wFYIn = checkStrictlyPositive(argv[i++], argv[0]);
			int wFOut = checkStrictlyPositive(argv[i++], argv[0]);
			int correctRounding = checkBoolean(argv[i++], argv[0]);
			float r = atof(argv[i++]);
			int maxTimeInMinutes = atoi(argv[i++]);

			op = new FPMultiplier(target, wE, wFXIn, wE, wFYIn, wE, wFOut, true, correctRounding, r, maxTimeInMinutes);
			addOperator(op);
		}  
		else if(opname=="FPSquarer"){
			int nargs = 3; 
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wE = checkStrictlyPositive(argv[i++], argv[0]);
				int wFX = checkStrictlyPositive(argv[i++], argv[0]);
				int wFR = checkStrictlyPositive(argv[i++], argv[0]);
				op = new FPSquarer(target, wE, wFX, wFR);
				addOperator(op);
			}
		} 
		else if (opname == "FPDiv")
		{
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wE = checkStrictlyPositive(argv[i++], argv[0]);
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			op = new FPDiv(target, wE, wF);
			addOperator(op);
		}
		else if (opname == "FPSqrt")
		{
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wE = checkStrictlyPositive(argv[i++], argv[0]);
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			op = new FPSqrt(target, wE, wF);
			addOperator(op);
		}
#if 0
		else if (opname == "FP2DNorm")
		{
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wE = checkStrictlyPositive(argv[i++], argv[0]);
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			op = new FP2DNorm(target, wE, wF);
			addOperator(op);
		}
#endif
#ifdef HAVE_SOLLYA
		else if (opname == "FPSqrtPoly")
		{
			int nargs = 3;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wE = checkStrictlyPositive(argv[i++], argv[0]);
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			int degree = checkStrictlyPositive(argv[i++], argv[0]);
			op = new FPSqrtPoly(target, wE, wF, false, degree);
			addOperator(op);
		}
#endif // HAVE_SOLLYA

		else if(opname=="LongAcc"){
			int nargs = 5;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wEX = checkStrictlyPositive(argv[i++], argv[0]);
				int wFX = checkStrictlyPositive(argv[i++], argv[0]);
				int MaxMSBX = atoi(argv[i++]); // may be negative
				int LSBA = atoi(argv[i++]); // may be negative
				int MSBA = atoi(argv[i++]); // may be negative
				op = new LongAcc(target, wEX, wFX, MaxMSBX, LSBA, MSBA);
				addOperator(op);
			}
		}


		else if(opname=="LongAcc2FP"){
			int nargs = 4;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int LSBA = atoi(argv[i++]); // may be negative
				int MSBA = atoi(argv[i++]); // may be negative
				int wE_out = checkStrictlyPositive(argv[i++], argv[0]);
				int wF_out = checkStrictlyPositive(argv[i++], argv[0]);
				op = new LongAcc2FP(target, LSBA, MSBA, wE_out, wF_out);
				addOperator(op);
			}
		}
		// hidden and undocumented
		else if(opname=="DotProdPrecTest"){
			int nargs = 7; // same as LongAcc, plus an iteration count
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wE = checkStrictlyPositive(argv[i++], argv[0]);
				int wFX = checkStrictlyPositive(argv[i++], argv[0]);
				int wFY = checkStrictlyPositive(argv[i++], argv[0]);
				int MaxMSBX = atoi(argv[i++]);
				int LSBA = atoi(argv[i++]);
				int MSBA = atoi(argv[i++]);
				int n = atoi(argv[i++]);
				DotProduct * op = new DotProduct(target, wE, wFX, wFY, MaxMSBX, LSBA, MSBA);
				op->test_precision(n);
			}    
		}
		else if(opname=="DotProduct"){
			int nargs = 6;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				int wE = checkStrictlyPositive(argv[i++], argv[0]);
				int wFX = checkStrictlyPositive(argv[i++], argv[0]);
				int wFY = checkStrictlyPositive(argv[i++], argv[0]);
				int MaxMSBX = atoi(argv[i++]); // may be negative
				int LSBA = atoi(argv[i++]); // may be negative
				int MSBA = atoi(argv[i++]); // may be negative
				double ratio = atof(argv[i++]); // may be negative
				op = new DotProduct(target, wE, wFX, wFY, MaxMSBX, LSBA, MSBA, ratio);
				addOperator(op);
			}
		}
//		else if(opname=="PolynomialEvaluator"){
//			int nargs = 1;
//			if (i+nargs > argc)
//				usage(argv[0],opname);
//			else {
//				int prec = atoi(argv[i++]); // may be negative
//				FixedPointCoefficient* f0 = new FixedPointCoefficient( 27, 0);
//				FixedPointCoefficient* f1 = new FixedPointCoefficient( 17,-1);
//				FixedPointCoefficient* f2 = new FixedPointCoefficient( 9, -3);

//				YVar* y = new YVar(16, -6);
//				
//				vector<FixedPointCoefficient*> coef;
//				coef.push_back(f0);
//				coef.push_back(f1);
//				coef.push_back(f2);

//				op = new PolynomialEvaluator(target, coef, y, prec);
//				addOperator(op);
//			}
//		}
				
#ifdef HAVE_SOLLYA
		else if (opname == "HOTBM") {
			int nargs = 4;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			string func = argv[i++];
			int wI = checkStrictlyPositive(argv[i++], argv[0]);
			int wO = checkStrictlyPositive(argv[i++], argv[0]);
			int n  = checkStrictlyPositive(argv[i++], argv[0]);
			op = new HOTBM(target, func, "", wI, wO, n);
			addOperator(op);
		}

		else if (opname == "HOTBMFX") {
			int nargs = 6;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			string func = argv[i++];
			int wE_in = atoi(argv[i++]); // may be negative
			int wF_in = atoi(argv[i++]); // may be negative
			int wE_out = atoi(argv[i++]); // may be negative
			int wF_out = atoi(argv[i++]); // may be negative
			int n  = checkStrictlyPositive(argv[i++], argv[0]);
			
			// Input of HOTBM is unsigned, but output is in 2's-complement!
			// Use 1 more bit for the output sign
			int wI = wE_in + wF_in;
			int wO = wE_out + wF_out + 1;
			double xmin = 0;
			double xmax = ldexp(1.0, wE_in);
			double scale = ldexp(1.0, -wE_out);
			op = new HOTBM(target, func, "", wI, wO, n, xmin, xmax, scale);
			addOperator(op);
		}
		
		else if (opname == "HOTBMRange") {
			int nargs = 7;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			string func = argv[i++];
			int wI = checkStrictlyPositive(argv[i++], argv[0]);
			int wO = checkStrictlyPositive(argv[i++], argv[0]);
			int n  = checkStrictlyPositive(argv[i++], argv[0]);
			double xmin = atof(argv[i++]);
			double xmax = atof(argv[i++]);

			// xmax < xmin is a valid use case...
			double scale = atof(argv[i++]);
			op = new HOTBM(target, func, "", wI, wO, n, xmin, xmax, scale);
			addOperator(op);
		}
		
#endif // HAVE_SOLLYA

		else if (opname == "FPExp")
		{
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wE = checkStrictlyPositive(argv[i++], argv[0]);
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			op = new FPExp(target, wE, wF, 0, 0);
			addOperator(op);
		}


		else if (opname == "FPExpExpert")
		{
			int nargs = 7;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wE = checkStrictlyPositive(argv[i++], argv[0]);
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			int k=atoi(argv[i++]);
			int d=atoi(argv[i++]);
			int g=atoi(argv[i++]);
			int fullInput=checkBoolean(argv[i++],  argv[0]);
			float DSPThreshold = atof(argv[i++]);
			op = new FPExp(target, wE, wF, k, d, g, fullInput, DSPThreshold);
			addOperator(op);
		}

		else if (opname == "FPLog")
		{
			int nargs = 3;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wE = checkStrictlyPositive(argv[i++], argv[0]);
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			int inTableSize=atoi(argv[i++]);
			op = new FPLog(target, wE, wF, inTableSize);
			addOperator(op);
		}

		else if (opname == "FPPowerExpert")
		{
			int nargs = 6;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit 

			//int logTableSize, int expTableSize, int expDegree, int expG, int logG int type
			int wE = checkStrictlyPositive(argv[i++], argv[0]);
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			int type=atoi(argv[i++]);
			int logTableSize=atoi(argv[i++]);
			int expTableSize=atoi(argv[i++]);
			int expDegree=atoi(argv[i++]);
			op = new FPPow(target, wE, wF, type, logTableSize, expTableSize, expDegree);
			addOperator(op);
		}

		else if (opname == "FPPow")
		{
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit 

			//int logTableSize, int expTableSize, int expDegree, int expG, int logG
			int wE = checkStrictlyPositive(argv[i++], argv[0]);
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			op = new FPPow(target, wE, wF, 0);
			addOperator(op);
		}

		else if (opname == "FPPowr")
		{
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit 

			//int logTableSize, int expTableSize, int expDegree, int expG, int logG
			int wE = checkStrictlyPositive(argv[i++], argv[0]);
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			op = new FPPow(target, wE, wF, 1);
			addOperator(op);
		}

		else if (opname == "InputIEEE")
		{
			int nargs = 4;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wEI = checkStrictlyPositive(argv[i++], argv[0]);
			int wFI = checkStrictlyPositive(argv[i++], argv[0]);
			int wEO = checkStrictlyPositive(argv[i++], argv[0]);
			int wFO = checkStrictlyPositive(argv[i++], argv[0]);
			op = new InputIEEE(target, wEI, wFI, wEO, wFO);
			addOperator(op);
		}

		else if (opname == "OutputIEEE")
		{
			int nargs = 4;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wEI = checkStrictlyPositive(argv[i++], argv[0]);
			int wFI = checkStrictlyPositive(argv[i++], argv[0]);
			int wEO = checkStrictlyPositive(argv[i++], argv[0]);
			int wFO = checkStrictlyPositive(argv[i++], argv[0]);
			op = new OutputIEEE(target, wEI, wFI, wEO, wFO);
			addOperator(op);
		}

		else if (opname == "Collision")
		{
			int nargs = 3;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wE = checkStrictlyPositive(argv[i++], argv[0]);
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			int optimize = checkBoolean(argv[i++], argv[0]);
			op = new Collision(target, wE, wF, optimize);
			addOperator(op);
		}
		else if (opname == "FPSumOfSquares")
		{
			int nargs = 3;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wE = checkStrictlyPositive(argv[i++], argv[0]);
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			int optimize = checkBoolean(argv[i++], argv[0]);
			op = new FPSumOfSquares(target, wE, wF, optimize);
			addOperator(op);
		}
		else if (opname == "IntSquarer")
		{
			int nargs = 1;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wIn = checkStrictlyPositive(argv[i++], argv[0]);
			op = new IntSquarer(target, wIn);
			addOperator(op);
		}
		else if (opname == "IntPower")
		{
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wIn = checkStrictlyPositive(argv[i++], argv[0]);
			int n = checkStrictlyPositive(argv[i++], argv[0]);
			op = new IntPower(target, wIn, n);
			addOperator(op);
		}

#ifdef HAVE_LNS
		else if (opname == "LNSAddSub")
		{
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wE = atoi(argv[i++]);	// can be null or negative
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			op = new LNSAddSub(target, wE, wF);
			if(cl_name!="")	op->setName(cl_name);
			addOperator(op);
		}
		else if (opname == "LNSMul")
		{
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wE = atoi(argv[i++]);	// can be null or negative
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			op = new LNSMul(target, wE, wF);
			if(cl_name!="")	op->setName(cl_name);
			addOperator(op);
		}
		else if (opname == "LNSDiv")
		{
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wE = atoi(argv[i++]);	// can be null or negative
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			op = new LNSDiv(target, wE, wF);
			addOperator(op);
		}
		else if (opname == "LNSSqrt")
		{
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wE = atoi(argv[i++]);	// can be null or negative
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			op = new LNSSqrt(target, wE, wF);
			if(cl_name!="")	op->setName(cl_name);
			addOperator(op);
		}
		else if (opname == "LNSAdd")
		{
			int nargs = 3;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wE = atoi(argv[i++]);	// can be null or negative
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			int o = checkStrictlyPositive(argv[i++], argv[0]);
			op = new LNSAdd(target, wE, wF, o);
			addOperator(op);
		}
		// Undocumented LNS operators, for debugging purposes
#if 0
		else if (opname == "CotranF1")
		{
			int nargs = 3;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			int j = checkStrictlyPositive(argv[i++], argv[0]);
			int wE = atoi(argv[i++]);
			op = new TableOp(target, new CotranF1Table(wF, j, wE));
			addOperator(op);
		}
		else if (opname == "CotranF2")
		{
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			int j = checkStrictlyPositive(argv[i++], argv[0]);
			op = new TableOp(target, new CotranF2Table(wF, j));
			addOperator(op);
		}
		else if (opname == "CotranF3")
		{
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			int j = checkStrictlyPositive(argv[i++], argv[0]);
			op = new TableOp(target, new CotranF3Table(wF, j));
			addOperator(op);
		}
#endif // #if 0
		else if (opname == "Cotran")
		{
			int nargs = 4;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wE = atoi(argv[i++]);	// can be null or negative
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			int j = checkStrictlyPositive(argv[i++], argv[0]);
			int wECotran = atoi(argv[i++]);
			// int o = atoi(argv[i++]); commented by F2D to suppress one warning
			op = new Cotran(target, wE, wF, j, wECotran);
			addOperator(op);
		}
		else if (opname == "CotranHybrid")
		{
			int nargs = 4;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wE = atoi(argv[i++]);	// can be null or negative
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			int j = checkStrictlyPositive(argv[i++], argv[0]);
			int wECotran = atoi(argv[i++]);
			// int o = atoi(argv[i++]); commented by F2D to suppress one warning
			op = new CotranHybrid(target, wE, wF, j, wECotran);
			addOperator(op);
		}
		else if (opname == "AtanPow")
		{
			int nargs = 3;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wE = checkStrictlyPositive(argv[i++], argv[0]);
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			int o = checkStrictlyPositive(argv[i++], argv[0]);
			op = new AtanPow(target, wE, wF, o);
			addOperator(op);
		}
		else if (opname == "LogSinCos")
		{
			int nargs = 3;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int fL = checkStrictlyPositive(argv[i++], argv[0]);
			int fTheta = checkStrictlyPositive(argv[i++], argv[0]);
			int o = checkStrictlyPositive(argv[i++], argv[0]);
			op = new LogSinCos(target, fL, fTheta, o);
			addOperator(op);
		}
#endif
		else if (opname == "Wrapper") {
			int nargs = 0;
			if (i+nargs > argc)
				usage(argv[0],opname);
			else {
				if(target->getGlobalOpListRef()->empty()){
					cerr<<"ERROR: Wrapper has no operator to wrap (it should come after the operator it wraps)"<<endl;
					usage(argv[0],opname);
				}
				Operator* toWrap = target->getGlobalOpListRef()->back();
				op =new Wrapper(target, toWrap);
				addOperator(op);
			}
		}
#if 0
		else if (opname == "PolyTableGenerator") {
			int nargs = 4;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			string func = argv[i++];
			int wO = atoi(argv[i++]);
			int n  = checkStrictlyPositive(argv[i++], argv[0]);
			Operator* tg = new PolyTableGenerator(target, func,  wO, n);
			addOperator(tg);
			
		}
#endif		


#ifdef HAVE_SOLLYA

		else if (opname == "FunctionTable") {
			int nargs = 4;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			string func = argv[i++];
			int wI = checkStrictlyPositive(argv[i++], argv[0]);
			int lsbO = atoi(argv[i++]);
			int msbO = atoi(argv[i++]);
			Operator* tg = new FunctionTable(target, func, wI, lsbO, msbO);
			addOperator(tg);
		}

		else if (opname == "FunctionEvaluator") {
			int nargs = 4;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			string func = argv[i++];
			int wI = checkStrictlyPositive(argv[i++], argv[0]);
			int wO = atoi(argv[i++]);
			int n  = checkStrictlyPositive(argv[i++], argv[0]);
			string arg=func+",0,1,1"; // we are not sure it works for other values
			Operator* tg = new FunctionEvaluator(target, arg, wI, wO, n);
			addOperator(tg);
		}

		else if (opname == "FPPipeline") {
			int nargs = 3;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			string filename = argv[i++];
			int wE = checkStrictlyPositive(argv[i++], argv[0]);
			int wF = checkStrictlyPositive(argv[i++], argv[0]);
			Operator* tg = new FPPipeline(target, filename, wE, wF);
			addOperator(tg);
		}
#endif

#ifdef HAVE_SOLLYA
		else if (opname == "FixSinCos") {
			int nargs = 1;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int w = checkStrictlyPositive(argv[i++], argv[0]); // must be >=2 actually
			Operator* tg = new FixSinCos(target, w);
			addOperator(tg);
		}

		else if (opname == "FixSinCosExpert") {
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int w = checkStrictlyPositive(argv[i++], argv[0]); // must be >=2 actually
			float DSPThreshold = atof(argv[i++]);
			Operator* tg = new FixSinCos(target, w,DSPThreshold);
			addOperator(tg);
		}

		else if (opname == "FixSinOrCos") {
			int nargs = 2;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int w = checkStrictlyPositive(argv[i++], argv[0]); // must be >=2 actually
			int degree = atoi(argv[i++]); 
			Operator* tg = new FixSinOrCos(target, w, degree);
			addOperator(tg);
		}

		
		else if (opname == "CordicSinCos") {
			int nargs = 3;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int wIn = checkStrictlyPositive(argv[i++], argv[0]); // must be >=2 actually
			int wOut = checkStrictlyPositive(argv[i++], argv[0]); // must be >=2 actually
			int reducedIterations = checkPositiveOrNull(argv[i++], argv[0]); 
			Operator* tg = new CordicSinCos(target, wIn, wOut, reducedIterations);
			addOperator(tg);
		}

		else if (opname == "CordicAtan2") {
			int nargs = 1;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			int w = checkStrictlyPositive(argv[i++], argv[0]); // must be >=2 actually
			Operator* tg = new CordicAtan2(target, w);
			addOperator(tg);
		}

#endif

		else if(random_parseCommandLine(argc, argv, target, opname, i)){
			// we actually do nothing, the work is already done if it returned true
		}
		else if (opname == "TestBench") {
			int nargs = 1;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			if(target->getGlobalOpListRef()->empty()){
				cerr<<"ERROR: TestBench has no operator to wrap (it should come after the operator it wraps)"<<endl;
				usage(argv[0],opname); // and exit
			}
			int n = checkPositiveOrNull(argv[i++], argv[0]);
			Operator* toWrap = target->getGlobalOpListRef()->back();
			Operator* op = new TestBench(target, toWrap, n);
			addOperator(op);
			cerr << "To run the simulation using ModelSim, type the following in 'vsim -c':" <<endl;
			cerr << tab << "vdel -all -lib work" <<endl;
			cerr << tab << "vlib work" <<endl;
			cerr << tab << "vcom " << filename <<endl;
			cerr << tab << "vsim " << op->getName() <<endl;
			cerr << tab << "add wave -r *" <<endl;
			cerr << tab << "run " << ((TestBench*)op)->getSimulationTime() <<"ns" << endl;
			cerr << "To run the simulation using gHDL, type the following in a shell prompt:" <<endl;
			string simlibs;
			if(op->getStdLibType()==0 || op->getStdLibType()==-1)
				simlibs="--ieee=synopsys ";
			if(op->getStdLibType()==1)
				simlibs="--ieee=standard ";
			cerr <<  "ghdl -a " << simlibs << "-fexplicit "<< filename <<endl;
			cerr <<  "ghdl -e " << simlibs << "-fexplicit " << op->getName() <<endl;
			cerr <<  "ghdl -r " << simlibs << op->getName() << " --vcd=" << op->getName() << ".vcd" <<endl;
			cerr <<  "gtkwave " << op->getName() << ".vcd" << endl;
		}
		
		else if (opname == "TestBenchFile") {
			/* Using a file to store IO */
			int nargs = 1;
			if (i+nargs > argc)
				usage(argv[0],opname); // and exit
			if(target->getGlobalOpListRef()->empty()){
				cerr<<"ERROR: TestBench has no operator to wrap (it should come after the operator it wraps)"<<endl;
				usage(argv[0],opname); // and exit
			}
			int n = atoi(argv[i++]);//checkPositiveOrNull(argv[i++], argv[0]);
			Operator* toWrap = target->getGlobalOpListRef()->back();
			Operator* op = new TestBench(target, toWrap, n, true);
			cerr << "> TestBench for " << toWrap->getName()<<endl;
			addOperator(op);
			cerr << "To run the simulation using ModelSim, type the following in 'vsim -c':" <<endl;
			cerr << tab << "vdel -all -lib work" <<endl;
			cerr << tab << "vlib work" <<endl;
			cerr << tab << "vcom " << filename <<endl;
			cerr << tab << "vsim " << op->getName() <<endl;
			cerr << tab << "add wave -r *" <<endl;
			cerr << tab << "run " << ((TestBench*)op)->getSimulationTime() << "ns" << endl;
			cerr << "To run the simulation using gHDL, type the following in a shell prompt:" <<endl;
			string simlibs;
			if(op->getStdLibType()==0 || op->getStdLibType()==-1)
				simlibs="--ieee=synopsys ";
			if(op->getStdLibType()==1)
				simlibs="--ieee=standard ";
			cerr <<  "ghdl -a " << simlibs << "-fexplicit "<< filename <<endl;
			cerr <<  "ghdl -e " << simlibs << "-fexplicit " << op->getName() <<endl;
			cerr <<  "ghdl -r " << simlibs << op->getName() << " --vcd=" << op->getName() << ".vcd" <<endl;
			cerr <<  "gtkwave " << op->getName() << ".vcd" << endl;
		}
		else  {
			cerr << "ERROR: Problem parsing input line, exiting";
			usage(argv[0]);
		}
	} while (i<argc);
	return true;
}

















int main(int argc, char* argv[] )
{
#ifdef HAVE_SOLLYA
	/// sollya initialization
	jmp_buf recover;

	initTool();
	if (setjmp(recover)) {
	/* If we are here, we have come back from an error in the library */
		std::cerr << "An error occurred somewhere in the Sollya library.\n";
		exit(1);
	}
	setRecoverEnvironment(&recover);
	extern int recoverEnvironmentReady;
	recoverEnvironmentReady=1;
#endif
	uint32_t i;
	

	target = new Virtex5(); // this also creates a global operator list

	// for historical reasons, to get rid of some day

	try {
		parseCommandLine(argc, argv);
	} catch (char const * s) {
		cerr << "Exception while parsing command line: " << s << endl;
		return 1;
	} catch (std::string s){
		cerr << "Exception while parsing command line: " << s << endl;
		return 1;
	}

	 vector<Operator*>* oplist=target->getGlobalOpListRef();




	ofstream file;
	file.open(filename.c_str(), ios::out);
	Operator::outputVHDLToFile(*oplist, file); 
	file.close();
	
	cerr << endl<<"Final report:"<<endl;
	for(i=0; i<oplist->size(); i++) {
		(*oplist)[i]->outputFinalReport(0);
	}
	
	cerr<< "Output file: " << filename <<endl;
	

	//------------------------ Resource Estimation ---------------------
	for (vector<Operator*>::iterator it = oplist->begin(); it!=oplist->end(); ++it) {
		Operator* op = *it;
		
		if(reLevel!=0){
			if(op->reActive)
				cerr << op->generateStatistics(reLevel);
			else{
				cerr << "Resource estimation option active for an operator that has NO estimations in place." << endl;
			}
		}
	}
	//------------------------------------------------------------------
	
	//------------------ Resource Estimation Debugging -----------------
	if(resourceEstimationDebug){
		ofstream file;
		file.open("flopoco.debug");
		for (vector<Operator*>::iterator it = oplist->begin(); it!=oplist->end(); ++it) {
			Operator* op = *it;
			
			if(op->reActive)
				file << op->resourceEstimate.str();
			else{
				cerr << "Resource estimation debugging option active for an operator that has NO estimations in place." << endl;
			}
		}
		file.close();
		cerr << "Resource estimation log written to the \'flopoco.debug\' file" << endl;
	}
	//------------------------------------------------------------------
	
	//------------------------ Floorplanning ---------------------------
	if(floorplanning){
		for (vector<Operator*>::iterator it = oplist->begin(); it!=oplist->end(); ++it) {
			Operator* op = *it;
			
			if(op->reActive == false){
				cerr << "Floorplanning option can be used only when the resource estimations have been performed.\n" 
						<< " Please reconsider your strategy.\n" << endl;
				exit(1);
			}
			if(floorplanning)
				floorplanMessages << op->createFloorplan();
		}
	}
	//------------------------------------------------------------------
	
	//------------------ Floorplanning Debugging -----------------------
	if(floorplanningDebug){
		if(!(reLevel>0 && floorplanning)){
			cerr << "Debugging Floorplanning option can be used only when there are resource estimations and floorplanning is enabled." 
						<< " Please rerun the program with the appropriate options" << endl;
				exit(1);
		}else{
			ofstream file;
			file.open("flopoco.floorplan.debug");
			for (vector<Operator*>::iterator it = oplist->begin(); it!=oplist->end(); ++it) {
				Operator* op = *it;
				
				file << op->floorplan.str();
				file << floorplanMessages.str();
			}
			file.close();
			cerr << "Floorplanning log (for debugging purposes) written to the \'flopoco.floorplanning.debug\' file" << endl;
		}
	}
	//------------------------------------------------------------------
	return 0;
}



