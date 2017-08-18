#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <mpfr.h>
#include <cstdlib>

#include "urng/LutSrRng.hpp"

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


	//string filename="flopoco.vhdl";
	//string cl_name=""; // used for the -name option
	//Target* target;
	
extern void usage(char *name, string opName);
extern int checkStrictlyPositive(char* s, char* cmd);
extern int checkPositiveOrNull(char* s, char* cmd);
extern bool checkBoolean(char* s, char* cmd);
extern int checkSign(char* s, char* cmd);
extern void addOperator(Operator *op);
	
void random_usage(char *name, string opName = ""){
	bool full = (opName=="");

	if( full || opName=="lut_sr_rng"){
		OP("lut_sr_rng", "r t k");
		cerr << "       uniform RNG using LUTs and Shift Registers\n";
		cerr << "	r - width of output random number\n";
		cerr << "	t - XOR gate input count\n";
		cerr << "	k - Maximum Shift Register length\n";
	}
	/*
	//6.20 bitwise architecture Junfei Yan
	if (full || opName=="bitwise"){
		OP("bitwise", "MSB LSB m lambda");
		cerr << "	exponential distribution bit-wise generator\n";
		cerr << "	MSB, LSB defines the output fixed point random number\n";
		cerr << "	m is the width of each comparator\n";
	}
	*/
}

bool random_parseCommandLine(
	int argc, char* argv[], Target *target,
	std::string opname, int &i){
	/*
	if (opname == "bitwise")
	{
		int nargs = 4;
		if (i+nargs > argc)
			usage(argv[0], opName); // and exit
		int MSB = checkStrictlyPositive(argv[i++], argv[0]);
		int LSB = checkStrictlyPositive(argv[i++], argv[0]);
		string m = argv[i++];
		double lambda   = checkStrictlyPositive(argv[i++], argv[0]);

		cerr << "> bitwise: MSB=" << MSB << " LSB=" << LSB << " m=" << m << " lambda=" << lambda << endl;
		op = new bitwise(target, MSB, LSB, m, lambda);
		addOperator(oplist, op);
		return true;
	}
	else */
	if (opname == "lut_sr_rng")
	{
		int nargs = 3;
		if (i+nargs > argc)
			usage(argv[0], opname); // and exit
		int tr = checkStrictlyPositive(argv[i++], argv[0]);
		int t = checkStrictlyPositive(argv[i++], argv[0]);
		int k = checkStrictlyPositive(argv[i++], argv[0]);


		cerr << "> lut_sr_rng: r=" << tr << "	t= " << t << "	k= " << k <<endl;
		addOperator(new flopoco::random::LutSrRng(target, tr, t, k));
		return true;
	}
	else
	{
		return false;
	}
}



