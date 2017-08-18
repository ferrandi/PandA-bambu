#ifndef __lut_sr_urng_HPP
#define __lut_sr_urng_HPP
#include <vector>
#include <sstream>
#include <set>

#include "Operator.hpp"

/* This file contains a lot of useful functions to manipulate vhdl */
#include "utils.hpp"



/*  All flopoco operators and utility functions are declared within
  the flopoco namespace.
*/
namespace flopoco{
namespace random{


// new operator class declaration
class LutSrRng : public Operator {
  public:
    /* operatorInfo is a user defined parameter (not a part of Operator class) for
      stocking information about the operator. The user is able to defined any number of parameter in this class, as soon as it does not affect Operator parameters undeliberatly*/
    static string operatorInfo;
    int tr; 		//number of random output bits generated per cycle
    int t;		// XOR input count
    int k;
    std::vector<int> perm;		//output permutation
    int seedtap;

    int n,r;
    uint32_t s;

	std::vector<set<int> > taps;	//XOR connections
	std::vector<int> cycle;		//initial seed cycle

  std::vector<int> cs, ns;


    int i;		//loop counter

  public:

    // constructor, defined there with two parameters (default value 0 for each)
    LutSrRng(Target* target,int tr, int t, int k);

    // destructor
    ~LutSrRng();


    // Below all the functions needed to test the operator
    /* the emulate function is used to simulate in software the operator
      in order to compare this result with those outputed by the vhdl opertator */
    void emulate(TestCase * tc);

    /* function used to create Standard testCase defined by the developper */
  //  void buildStandardTestCases(TestCaseList* tcl);


	void buildStandardTestCases(TestCaseList* tcl);


//    void buildRandomTestCases(TestCaseList* tcl, int n);
//    TestCase* buildRandomTestCase(int i);
};

}; // random
}; // flopoco
#endif
