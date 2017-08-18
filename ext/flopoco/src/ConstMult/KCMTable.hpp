#ifndef KCMTable_HPP
#define KCMTable_HPP

#include "../Table.hpp"

namespace flopoco{

	class KCMTable : public Table
	{
	public:
		KCMTable(Target* target, int wIn, int wOut, mpz_class C, bool inputSigned,  map<string, double> inputDelays = emptyDelayMap);
	
		~KCMTable();

		mpz_class function(int x);

		mpz_class C_; //the constant

		bool inputSigned_;
	};

}
#endif //KCMTable_HPP

