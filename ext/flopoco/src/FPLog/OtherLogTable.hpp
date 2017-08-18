#ifndef OTHERLOGTABLE_HPP
#define OTHERLOGTABLE_HPP

#include "../Table.hpp"


namespace flopoco{

	class OtherLogTable : public Table
	{
	public:
		OtherLogTable(Target* target, int wIn, int wOut, int which, int ai, int pi);
	
		~OtherLogTable();


		mpz_class function(int x);
	
		int    double2input(double x);
	
		double input2double(int x);
	
		mpz_class double2output(double x);
	
		double output2double(mpz_class x);
	
	
		/** Which table is that ? useful because the first is different from the others.*/
		int which;

		/** ai is needed to determine epsiloni */
		int ai;

		/** pi is needed to determine epsiloni */
		int pi;
	
		int outputPrecision;
	
	};





}
#endif //OTHERLOGTABLE_HPP

