#ifndef FIRST_INV_TABLE_H
#define FIRST_INV_TABLE_H

#include "../Table.hpp"


namespace flopoco{

	class FirstInvTable : public Table
	{
	public:
		FirstInvTable(Target* target, int p1, int w1);
	
		~FirstInvTable();
  	
		mpz_class function(int x);
	
		//  int check_accuracy(int wF);
	
		int    double2input(double x);
	
		double input2double(int x);
	
		mpz_class double2output(double x);
	
		double output2double(mpz_class x);
		
		double maxMulOut;
		double minMulOut;
	
	};




}

#endif //FIRST_TABLE_H

