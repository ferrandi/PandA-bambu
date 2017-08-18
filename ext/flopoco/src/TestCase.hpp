#ifndef __TESTCASE_HPP
#define __TESTCASE_HPP

#include <string>
#include <vector>
#include <map>
#include <list>
#include <ostream>
#include <sstream>

#include "Signal.hpp"
#include "FPNumber.hpp"
#include "IEEENumber.hpp"

using namespace std;

namespace flopoco{

	/**
		A test case is a mapping between I/O signal names and boolean values
		given as mpz.

		The signal names must be those of Operator->iolist_. Whether several
		possible output values are possible is stored in the
		numberOfPossibleValues_ attribute of the corresponding Signal stored in iolist, and
		only there.

		The emulate() function of Operator takes a partial test case (mapping
		all the inputs) and completes it by mapping the outputs.
		* @see TestBench
		* @see Operator
		*/

	class Operator;
	class TestCaseList;

	class TestCase {
	public:

		/** Creates an empty TestCase for operator op */
		TestCase(Operator* op);
		~TestCase();

		/**
		 * Adds an input for this TestCase
		 * @param s The signal which will value the given value
		 * @param v The value which will be assigned to the signal
		 */
		void addInput(string s, mpz_class v);

		/**
		 * Adds an input in the FloPoCo FP format for this TestCase
		 * @param s The signal which will value the given value
		 * @param v The value which will be assigned to the signal
		 */
		void addFPInput(string s, FPNumber::SpecialValue v);

		/**
		 * Adds an input in the FloPoCo FP format for this TestCase
		 * @param s The signal which will value the given value
		 * @param x the value which will be assigned to the signal, provided as a double.
		 * (use this method with care, typically only for values such as 1.0 which are representable whatever wE or wF) 
		 */
		void addFPInput(string s, double x);

		/**
		 * Adds an input in the FloPoCo FP format for this TestCase
		 * @param s The signal which will value the given value
		 * @param x the value which will be assigned to the signal, provided as an FPNumber.
		 */
		void addFPInput(string s, FPNumber *x);

		/**
		 * Adds an input in the IEEE FP format for this TestCase
		 * @param s The signal which will value the given value
		 * @param v The value which will be assigned to the signal
		 */
		void addIEEEInput(string s, IEEENumber::SpecialValue v);

		/**
		 * Adds an input in the IEEE FP format for this TestCase
		 * @param s The signal which will value the given value
		 * @param x the value which will be assigned to the signal, provided as a double.
		 * (use this method with care) 
		 */
		void addIEEEInput(string s, double x);

		void addIEEEInput(string s, IEEENumber ieeeNumber);

		/**
		 * recover the mpz associated to an input
		 * @param s The name of the input
		 */
		mpz_class getInputValue(string s);

		void setInputValue(string s, mpz_class v);

		/**
		 * Adds an expected output for this signal
		 * @param s The signal for which to assign an expected output
		 * @param v One possible value which the signal might have
		 */
		void addExpectedOutput(string s, mpz_class v);

		/**
		 * Adds a comment to the output VHDL. "--" are automatically prepended.
		 * @param c Comment to add.
		 */
		void addComment(std::string c);

		/**
		 * Generates the VHDL code necessary for assigning the input signals.
		 * @param prepend A string to prepend to each line.
		 * @return A multi-line VHDL code.
		 */
		std::string getInputVHDL(std::string prepend = "");

		/**
		 * Generate the VHDL code necessary to assert the expected output
		 * signals.
		 * @param prepend A string to prepend to each line.
		 * @return A single-line VHDL code.
		 */
		std::string getExpectedOutputVHDL(std::string prepend = "");

		/**
		 * Return the comment string for this test case
		 * @return A single-line VHDL code.
		 */
		std::string getComment();

		/**
		 * Generate one hexadecimal line for each test case
		 * @param prepend A string to prepend to each line.
		 * @return A single line with all the inputs, then all the outputs.
		 */
		std::string getCompactHexa(std::string prepend = "");


                /**
                 * generate a string with each inputs, one by line, and each
                 * expected outputs, one by line too.
                 * and the order for outputing these IO is given by IOorder
                 */
                std::string generateInputString(list<string> IOorderInput, list<string> IOorderOutput);

                /**
                 *    Define the test case integer identifiant
                 */
                void setId(int id);

                int getId();

                string getDescription();

	private:
		Operator *op_;                       /**< The operator for which this test case is being built */

		map<string, mpz_class>          inputs;
		map<string, vector<mpz_class> >   outputs;

		string comment;
                int intId;                      /* integer identifiant of the test case */ 

	};



	/**
	 * Represents a list of test cases that an Operator has to pass.
	 */
	class TestCaseList {
	public:
		/**
		 * Creates an empty TestCaseList
		 * @see TestCase
		 */
		TestCaseList();
	
		/** Destructor */
		~TestCaseList();

		/**
		 * Adds a TestCase to this TestCaseList.
		 * @param t TestCase to add
		 */
		void add(TestCase* t);

		/**
		 * Get the number of TestCase-es contained in this TestCaseList.
		 * @return The number of TestCase-es
		 */
		int getNumberOfTestCases();

		/**
		 * Get a specific TestCase.
		 * @param i Which TestCase to return
		 * @return The i-th TestCase.
		 */
		TestCase * getTestCase(int i);

	private:
		/** Stores the TestCase-es */
		vector<TestCase*>  v;
                map<int,TestCase*> mapCase;
                /* id given to the last registered test case*/

	};

}
#endif
