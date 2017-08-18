#ifndef _HOTBMINSTANCE_HH_
#define _HOTBMINSTANCE_HH_

#include <list>

#include "../Function.hpp"
#include "Param.hh"
#include "PWPolynomial.hh"
#include "Term.hh"
#include "Util.hh"
#include "Operator.hpp"
#include "../HOTBM.hpp"

#define HOTBM_DUMP_X           1
#define HOTBM_DUMP_FUNCTION    2
#define HOTBM_DUMP_APPROX      3
#define HOTBM_DUMP_METHOD      4
#define HOTBM_DUMP_ROUND       5
#define HOTBM_DUMP_ERR_POLY    6
#define HOTBM_DUMP_ERR_METHOD  7
#define HOTBM_DUMP_ERR_ROUND   8
#define HOTBM_DUMP_ERR_MAX     9
#define HOTBM_DUMP_INPUT_WORD  10
#define HOTBM_DUMP_OUTPUT_WORD 11

using namespace std;


namespace flopoco{

	class HOTBMInstance {
	public:
		HOTBMInstance(Function &f_, Param &p_);
		~HOTBMInstance();

		Param &getParam();

		static void freeApproxCache();
		static void freeTermCache(bool full = false);

		void roundTables(bool full = true);
		void tune();
		void dump(flopoco::FlopocoStream& vhdl, list<int> &dumpList);

		double estimArea(int t = -1) const;
		double estimDelay(int t = -1) const;

		friend void HOTBM::genVHDL();

		static bool verbose;
		static bool force;

	protected:
		typedef pair<string, Param> tParam;
		typedef pair<PWPolynomial, double> tApproxError;
		typedef pair<double **, tApproxError> tApprox;
		struct ltParam {
			bool operator()(const tParam &p1, const tParam &p2) const;
		};
		typedef map<tParam, tApprox, ltParam> tApproxCache;

		typedef pair<tParam, int> tTermParam;
		typedef Term *tTerm;
		struct ltTermParam {
			bool operator()(const tTermParam &p1, const tTermParam &p2) const;
		};
		typedef map<tTermParam, tTerm, ltTermParam> tTermCache;

		static tApproxCache approxCache;
		static tTermCache termCache;

		void computeApprox();
		void buildTerms();
		bool checkError();

		double eval(long long int x);
		long long int evalRound(long long int x);

		Function &f;
		Param p;

		double **kList;
		Term **tList;
		PWPolynomial errPoly, errMethod[2];
		double errBound[2];
		double *kAdjust;
		int nM, nT, gMax;
	};
}
#endif // _HOTBMINSTANCE_HH_
