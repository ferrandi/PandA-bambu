#ifndef _EXHAUSTIVE_HH_
#define _EXHAUSTIVE_HH_

#include <list>

#include "Param.hh"
#include "HOTBMInstance.hh"

using namespace std;

namespace flopoco{


	class Exhaustive {
	public:
		Exhaustive(Function &f_, Param &p_);
		~Exhaustive();

		HOTBMInstance *getInstance();
		static double score(const HOTBMInstance &inst);

	private:
		typedef pair<HOTBMInstance *, double> tInstance;
		struct ltInstance {
			bool operator()(const tInstance &i1, const tInstance &i2) const;
		};
		typedef multiset<tInstance, ltInstance> tInstSet;


		int process(list<Param> &pList, tInstSet &instSet, int nMax = -1);
		void skim(tInstSet &instSet, int nMax);

		Function &f;
		Param p;

		HOTBMInstance *instance;
	};
}
#endif // _EXHAUSTIVE_HH_
