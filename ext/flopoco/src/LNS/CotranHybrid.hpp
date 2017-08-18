#ifndef COTRAN_HYBRID_HPP
#define COTRAN_HYBRID_HPP

#include "../Operator.hpp"
#include <memory>
#include "LNSAdd.hpp"
#include "../FixFunctions/HOTBM.hpp"
#include "Cotran.hpp"
//#include <boost/shared_pointer>

namespace flopoco{

	struct CotranHybrid : Operator
	{
		CotranHybrid(Target * target, int wE, int wF, int j = -1, int wECotran = -1, int o = 1);
		virtual ~CotranHybrid();

		virtual void outputVHDL(std::ostream& o, std::string name);

		//virtual TestIOMap getTestIOMap();
		//virtual void fillTestCase(mpz_class a[]);
	
		int getJ();

	private:
		HOTBM * gen_db_table(int i, int order);
		
		int wE;
		int wF;
		int j;
		int wECotran;
	
		void select_j();
	
		Cotran * cotran;
	
		//	std::vector<boost::shared_pointer<HOTBM> > db_tables;
		// argh, ownership controlled (?) by oplist
		std::vector<HOTBM*> db_tables;

		int wEssZero;
		int DBMaxInput;
	};

}
#endif

