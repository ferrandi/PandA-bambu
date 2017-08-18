#ifndef __HOTBM_HPP
#define __HOTBM_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>

#include "../Operator.hpp"


namespace flopoco{

	class HOTBMInstance;
	class Function;

	/**
	 * Implements an Operator around HOTBM. Acts like a wrapper around
	 * HOTBM classes.
	 */
	class HOTBM : public Operator
	{
	public:
		HOTBM(Target* target, string func, string namebase, int wI, int wO, int n, double xmin = 0, double xmax = 1, double scale = 1);
		~HOTBM();

		// Overloading the virtual functions of Operator
		void emulate(TestCase* tc);
	
		int wIn() const { return wI; }
		int wOut() const { return wO + 1; }

		// defined in HOTBMInstance.cc
		void genVHDL();

	private:
		HOTBMInstance *inst;
		Function &f;

		int wI, wO;
	};

}
#endif
