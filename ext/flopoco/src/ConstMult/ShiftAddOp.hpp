#ifndef SHIFTADDOP_HPP
#define SHIFTADDOP_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>



namespace flopoco{

	class ShiftAddDag;


	typedef enum {X, Add, Sub, RSub, Shift, Neg} ShiftAddOpType;


	/**
		Class ShiftAddOp defines a shift-and-add operation, of one of 5 types:
		Add(z,i, s, y)     Vz  <-   Vi<<s  + Vy 
		Sub(z,i, s, y)     Vz  <-   Vi<<s  - Vy 
		RSub(z,i, s, y)    Vz  <-   -Vi<<s  + Vy 
		Shift(i,s)         Vz  <-   Vi<<s 
		Neg(i,s)        Vz  <-   (-Vi) 
		i, y and z are variable identifiers.
		This is single-assignment code, therefore 
		1/ the ShiftAddOp constructor builds a new variable number for the destination.
		2/ the ShiftAddOp object holds all the information related to its destination variable Vz. 

		You don't have to use all these types. In IntConstMult, we build an initial
		tree out of Add, Neg and Shift, then transform some subtrees into Sub
		and RSub in an optimization phase */


	class ShiftAddOp {
	public:
		ShiftAddDag* impl;
		ShiftAddOpType op;
		ShiftAddOp* i;
		ShiftAddOp* j;

		/**  The shift on i*/
		int s; 

		/** the constant by which this variable multiplies */
		mpz_class n; 

		/** string representation of the constant */
		string name;  

		/** size of the constant */
		int size; 

		/** cost in term of full-adders */
		int cost_in_full_adders;

		/** Has this node already been visited ? (useful when the DAG is not a tree) */
		bool already_visited;


		/** Constructor */
		ShiftAddOp(ShiftAddDag* impl, ShiftAddOpType op, ShiftAddOp* i=NULL, int s=0, ShiftAddOp* j=NULL);
	
		~ShiftAddOp(){};


	
		friend std::ostream& operator<<(std::ostream& o, const ShiftAddOp& sao ) // output
		{    
			o << sao.name << " <-  ";
			switch(sao.op) {
			case X:        o << " X"; break;
			case Add:      o << sao.i->name << "<<" << sao.s << "  + " << sao.j->name;   break;
			case Sub:      o << sao.i->name << "<<" << sao.s << "  - " << sao.j->name;   break;
			case RSub:      o << sao.j->name << "  - " << sao.i->name << "<<" << sao.s ;   break;
			case Shift:    o << " " << sao.i->name << "<<" << sao.s;                     break;
			case Neg:      o << "-" << sao.i->name;   break;
			}   
			return o;
		}

		friend FlopocoStream& operator<<(FlopocoStream& o, const ShiftAddOp& sao ) // output
		{    
			o << sao.name << " <-  ";
			switch(sao.op) {
			case X:        o << " X"; break;
			case Add:      o << sao.i->name << "<<" << sao.s << "  + " << sao.j->name;   break;
			case Sub:      o << sao.i->name << "<<" << sao.s << "  - " << sao.j->name;   break;
			case RSub:      o << sao.j->name << "  - " << sao.i->name << "<<" << sao.s ;   break;
			case Shift:    o << " " << sao.i->name << "<<" << sao.s;                     break;
			case Neg:      o << "-" << sao.i->name;   break;
			}   
			return o;
		}



	};
}
#endif
