#ifndef _FUNCTION_HH_
#define _FUNCTION_HH_

#include <string>
#include <iostream>

#include "HOTBM/Util.hh"

#include "../sollya.h"

using namespace std;


namespace flopoco{

	class Function {
	public:
		Function(string name_, double xmin = 0, double xmax = 1, double scale = 1);
		virtual ~Function();

		string getName() const;
		double eval(double x) const;
		void eval(mpfr_t r, mpfr_t x) const;
		sollya_node_t getSollyaNode() const;

	private:
		string name;
		sollya_node_t node;
	
	}
		;
}
#endif // _FUNCTION_HH_
