#ifndef _GENERIC_BINARY_POLYNOMIAL
#define _GENERIC_BINARY_POLYNOMIAL

#include "Operator.hpp"
#include "utils.hpp"
#include "IntMultiAdder.hpp"
#include <tr1/memory>
#include "FormalBinaryProduct.hpp"

namespace flopoco {

// TODO: it's a generic class, it should be put in a separate file
// or eliminated completely in favor of using directly std::auto_ptr
template<typename T> class Option {
	public:
		Option (T x)
			:empty(false),value(new T(x))
		{
		}
		Option ()
			:empty(true),value((T*) 0)
		{
		}
		bool is_empty () const {
			return empty;
		}
		T const& get_value () const {
			if (empty)
				throw "Option object is empty";
			return *value;
		}
	protected:
		bool empty;
		std::tr1::shared_ptr<T> value;
};

class GenericBinaryPolynomial : public Operator {
  public:
    //static std::string operatorInfo;
    Product p;

  public:
    GenericBinaryPolynomial(Target* target, const Product& p, std::map<string,double> inputDelays = emptyDelayMap);

    ~GenericBinaryPolynomial() {};

    void emulate(TestCase * tc);
    void buildStandardTestCases(TestCaseList* tcl);
    void buildRandomTestCases(TestCaseList* tcl, int n);
    TestCase* buildRandomTestCases(int i);
  protected:
    IntMultiAdder* ima;
};

}

#endif //include guard

