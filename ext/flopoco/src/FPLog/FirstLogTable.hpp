#ifndef FIRSTLOGTABLE_HPP
#define FIRSTLOGTABLE_HPP

#include "FirstInvTable.hpp"
#include "../FPLog.hpp"



namespace flopoco{


	class FirstLogTable : public Table
	{
	public:
		FirstLogTable(Target *target, int p1, int w1,  FirstInvTable* fit, FPLog* op_);
	
		~FirstLogTable();
	
	
		FirstInvTable* fit;
	
		mpz_class function(int x);
	
		int    double2input(double x);
	
		double input2double(int x);
	
		mpz_class double2output(double x);
	
		double output2double(mpz_class x);
	
	private:
	
		FPLog* op;
	
	};



}


#endif //FIRSTLOGTABLE_HPP

