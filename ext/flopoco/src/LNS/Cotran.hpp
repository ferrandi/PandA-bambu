#ifndef COTRAN_HPP
#define COTRAN_HPP

#include "../Operator.hpp"
#include <memory>
#include "CotranTables.hpp"
#include "LNSAdd.hpp"

namespace flopoco{

	class Cotran : public Operator
	{
	public:
		Cotran(Target * target, int wE, int wF, int j = -1, int wECotran = -1, int o = 1);
		virtual ~Cotran();

		virtual void outputVHDL(std::ostream& o, std::string name);

		//virtual TestIOMap getTestIOMap();
		//virtual void fillTestCase(mpz_class a[]);

	private:
		int wE;
		int wF;
		int j;
		int wECotran;
	
		void select_j();
	
		CotranF1Table* f1;
		CotranF2Table* f2;
		CotranF3Table* f3;
		LNSAdd * sb;
	
		int wEssZero;
		int DBMaxInput;
	};

}
#endif

