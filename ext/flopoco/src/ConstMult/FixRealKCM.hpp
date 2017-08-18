#ifndef FIXREALKCM_HPP
#define FIXREALKCM_HPP
#include "../Operator.hpp"
#include "../Table.hpp"

#include "../BitHeap.hpp"

namespace flopoco{



	class FixRealKCM : public Operator
	{
	public:

		/** Input size will be msbIn-lsbIn+1 if unsigned, msbIn-lsbIn+2 if signed */
		FixRealKCM(Target* target, int lsbIn, int msbIn, bool signedInput, int lsbOut, string constant,
							 double targetUlpError = 1.0, map<string, double> inputDelays = emptyDelayMap,
							 bool useBitheap = false);
							 
		FixRealKCM(Operator* parentOp, Target* target, Signal* multiplicandX, int lsbIn, int msbIn, bool signedInput, int lsbOut, string constant,
							 BitHeap* bitheap,
							 double targetUlpError = 1.0, map<string, double> inputDelays = emptyDelayMap);
							 
		~FixRealKCM();

		// Overloading the virtual functions of Operator
	  
		void emulate(TestCase* tc);
		
		static int neededGuardBits(Target* target, int wIn, int lsbOut, double targetUlpError);

		int lsbIn;
		int msbIn;
		bool signedInput;
		int wIn;
		int lsbOut;
		int msbOut;
		int wOut;
		string constant;
		float targetUlpError;
		mpfr_t mpC;
		int msbC;
		int g;
		
		bool useBitheap;
		
		bool negativeConstant;
		
		BitHeap*	bitHeap;    			/**< The heap of weighted bits that will be used to do the additions */
		Operator*	parentOp;				/**< The operator which envelops this constant multiplier */

	};
  
	class FixRealKCMTable : public Table
	{
	public:
		FixRealKCMTable(Target* target, FixRealKCM* mother, int i, int weight, int wIn, int wOut, bool signedInput, bool last, int logicTable = 1);
		~FixRealKCMTable();
    
		mpz_class function(int x);
		FixRealKCM* mother;
		int index;
		int weight; 
		bool signedInput;
		bool last;
	};

}


#endif
