#ifndef FPPipeline_HPP
#define FPPipeline_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include <cstdlib>
#include <iostream>
#include <fstream>

#include "FPAdderSinglePath.hpp"

#include "FPMultiplier.hpp"
#include "FPSquarer.hpp"
#include "FPSqrt.hpp"
#include "FPExp.hpp"
#include "FPLog.hpp"
#include "FPSqrtPoly.hpp"
#include "FPDiv.hpp"
#include "FPSqrt.hpp"
#include "ConstMult/FPRealKCM.hpp"
#include "ConstMult/FPConstMult.hpp"

#include "Operator.hpp"
#include "FPExpressions/ExpressionParserData.h"
// #include "HOTBM/sollya.h"	// Do NOT use libsollya from user's environment
// #include "UtilSollya.hh"

namespace flopoco{
	
	extern vector<Operator*> oplist;
	
	/** The FPPipeline class.  */
	class FPPipeline : public Operator {
		public:
			/** Class that assembles floating-point operators starting with 
			 * an untyped, untimed Python-like description of the computational
			 * datapath
			 * @param[in] target     The target FPGA object
			 * @param[in] filename   The filename containing the datapath
			 * @param[in] wE         Exponent width
			 * @param[in] wF         Fraction width
			**/ 
			FPPipeline(Target* target, string filename, int wE, int wF);
			
			/**
			* FPPipeline destructor
			*/
			~FPPipeline();
			
			/**
			 * Function which generates the VHDL code containing the assembled 
			 * operators starting from the node containg the output variable
			 * @param[in] n    The output variable (one of the output node list)
			 * @param[in] top  Boolean describing if this function is called from
			 * the statement level, or in some recursion level
			*/
		void generateVHDL_c(node* n, bool top);


			
		protected:
			int wE;   /**< Exponent size*/ 
			int wF;  /**< Significand fraction size */
	};
	
}
#endif
