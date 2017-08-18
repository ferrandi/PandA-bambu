#ifndef COTRAN_TABLES_HPP
#define COTRAN_TABLES_HPP

#include "../Table.hpp"
#include "../Operator.hpp"
#include <cmath>
#include <gmpxx.h>
#include <memory>
#include <cstdlib>

using namespace std;

namespace flopoco{

	class CotranF1Table : public Table
	{
	public:
		CotranF1Table(Target* target, int wF, int j, int wE);
		
		virtual ~CotranF1Table() {}

		virtual mpz_class function(int x);

	private:
		static int esszero(int wF, int j, int wE);
		static int addrLen(int wF, int j, int wE);
		static int dataLen(int wF, int j, int wE);
	
		int wF;
		int j;
		int k;
		double dh;
		int wE;

	};

	class CotranF2Table : public Table
	{
	public:
		CotranF2Table(Target* target, int wF, int j);
		
		virtual ~CotranF2Table() {}

		virtual mpz_class function(int x);

	private:
		static int addrLen(int wF, int j);
		static int dataLen(int wF, int j);
	
		int wF;
		int j;
		int k;
		double dh;

	};

	class CotranF3Table : public Table
	{
	public:
		CotranF3Table(Target* target, int wF, int j);
		
		virtual ~CotranF3Table() {}

		virtual mpz_class function(int x);

	private:
		static int addrLen(int wF, int j);
		static int dataLen(int wF, int j);
		static int begin(int wF, int j);
		static int end(int wF, int j);
		double SbArg(int z);
	
		int wF;
		int j;
		int k;
		double dh;

	};


}
#endif
