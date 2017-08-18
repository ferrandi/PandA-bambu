#ifndef PolynomialTableCorrectRounded_H
#define PolynomialTableCorrectRounded_H

#include "../Table.hpp"

namespace flopoco{

	class PolynomialTableCorrectRounded : public Table
	{
	public:
		PolynomialTableCorrectRounded(Target* target, int p1, int w1);
	
		~PolynomialTableCorrectRounded();
  	
		mpz_class function(int x);
	
		//  int check_accuracy(int wF);
	
		int    double2input(double x);
	
		double input2double(int x);
	
		mpz_class double2output(double x);
	
		double output2double(mpz_class x);
	
		double maxMulOut;
		double minMulOut;
	
	private:
	
	
	};
}
#endif

