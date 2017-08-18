#include <iostream>
#include <sstream>
#include <list>
#include <vector>
#include <tr1/memory>

#include "gmp.h"
#include "mpfr.h"

#include "../BitHeap.hpp"
#include "GenericBinaryPolynomial.hpp"
#include "IntMultiAdder.hpp"
#include "IntAddition/NewCompressorTree.hpp"

//std::string GenericBinaryPolynomial::operatorInfo = "UserDefinedInfo param0 param1 <options>";

using namespace flopoco;

static string vhdl_string_of_monomial_option
	(const Option<MonomialOfBits>& o)
{
	ostringstream vhdl;
	if (o.is_empty()) {
		vhdl << "'0'";
		return vhdl.str();
	}
	MonomialOfBits m = o.get_value();
	bool cont = false;
	size_t i;
	for (i = 0; i < m.data.size(); i++) {
		if (m.data[i]) {
			if (cont)
				vhdl << " and ";
			vhdl << "X" << of (m.data.size() - 1 - i);
			// because x_0 is the msb in ProductIR::identity(n)
			cont = true;
		}
	}
	if (!cont)
		vhdl << "'1'";
	return vhdl.str();
}

GenericBinaryPolynomial::GenericBinaryPolynomial(Target* target,
                                                 const Product& p,
						 std::map<std::string,double>
						 	inputDelays)
	:Operator(target,inputDelays), p(p) {

	ostringstream name;
	name << "GenericBinaryPolynomial_" << p.mon_size << "_" << p.data.size()
	     << "_uid" << Operator::getNewUId();
	setName(name.str());
	setCopyrightString("Guillaume Sergent, Florent de Dinechin 2012");

	addInput ("X" , p.mon_size);
	addOutput("R" , p.data.size());

	if (p.data.size() == 0) {
		return;
	}

#if 1 // The new Bit Heap 
	//	shared_ptr<BitHeap> bh(new BitHeap(this, ));
	// The bit heap
	BitHeap * bitHeap = new BitHeap(this, p.data.size());
	
 		
	for (unsigned i = 0; i < p.data.size(); i++) { // i is a weight
		list<MonomialOfBits>::const_iterator it = p.data[i].data.begin();
		for (; it != p.data[i].data.end(); it++) {
			ostringstream rhs;
			rhs << vhdl_string_of_monomial_option (Option<MonomialOfBits>(*it));
			bitHeap -> addBit(i, rhs.str()); 
		}
	}

		bitHeap -> generateCompressorVHDL();			
		vhdl << tab << "R" << " <= " << bitHeap-> getSumName() << range(p.data.size()-1, 0) << ";" << endl;


#else // Guillaume's compressor trees
	vector<unsigned> lengths (p.data.size(), 0);
	for (unsigned i = 0; i < p.data.size(); i++) {
		lengths[i] = p.data[i].data.size();
		if (!lengths[i]) {
			// if nct_input_i is null, just declare the signal
			// (such that inPortMap won't complain)
			declare (join("nct_input_",i),0);
			continue;
		}
		vhdl << declare (join("nct_input_",i),lengths[i]);
		if (lengths[i] == 1)
			// if there's no '&' it'll be a std_logic in rhs
			vhdl << of(0);
		vhdl << " <= (";
		list<MonomialOfBits>::const_iterator
			it = p.data[i].data.begin();
		for (; it != p.data[i].data.end(); it++) {
			if (it != p.data[i].data.begin())
				vhdl << ") & (";
			vhdl << vhdl_string_of_monomial_option (Option<MonomialOfBits>(*it));
		}
		vhdl << ");\n";
	}
	NewCompressorTree* nct = new NewCompressorTree (target, lengths);
	oplist.push_back (nct);
	outPortMap (nct, "R", "R_ima");
	for (int i = p.data.size() - 1; i >= 0; i--) {
		inPortMap (nct, join("X",i), join("nct_input_",i));
	}
	vhdl << instance (nct, "final_adder");
	if(nct->wOut < p.data.size())
		vhdl << "R <= " << zg(p.data.size() - nct->wOut) << "& R_ima;" << endl; 
	else
		vhdl << "R <= R_ima" << range(p.data.size()-1, 0) << ";" << endl; 
#endif

};

	
void GenericBinaryPolynomial::emulate(TestCase * tc) {
}

void GenericBinaryPolynomial::buildStandardTestCases(TestCaseList * tcl) {
}

void GenericBinaryPolynomial::buildRandomTestCases(TestCaseList *  tcl, int n) {
}

TestCase* GenericBinaryPolynomial::buildRandomTestCases(int i) {
  TestCase* tc = new TestCase(this);
  return tc;
}
