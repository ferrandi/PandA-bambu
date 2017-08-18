#ifndef __SIGNAL_HPP
#define __SIGNAL_HPP

#include <iostream>
#include <sstream>
#include <gmpxx.h>

#ifdef _WIN32
  #include "pstdint.h"
#else
  #include <inttypes.h>
#endif

namespace flopoco{

	/**
	 * A class representing a signal. This is the basic block that operators use
	 */
	class Signal
	{
	public:
		/** The possible types of a signal*/
		typedef enum {
			in,                        /**< if the signal is an input signal */
			out,                       /**< if the signal is an output signal */
			wire,                      /**< if the signal is a wire (not registered) */
			registeredWithoutReset,    /**< if the signal is registered, but does not have a reset */
			registeredWithAsyncReset,  /**< if the signal is registered, and has an asynchronous reset */
			registeredWithSyncReset,    /**< if the signal is registered, and has an synchronous reset */
			registeredWithZeroInitialiser    /**< if the signal is registered, the it has a zero initialiser (but no reset) */
		} SignalType;

		/** Signal constructor.
		 * The standard constructor for signals which are not floating-point.
		 * @param name      the name of the signal
		 * @param type      the type of the signal, @see SignalType
		 * @param width     the width of the signal
		 * @param isBus     the flag which signals if the signal is a bus (std_logic_vector)
		 */
		Signal(const std::string name, const Signal::SignalType type, const int width = 1, const bool isBus=false);
		/** Signal constructor.
		 * The standard constructor for signals which are floating-point.
		 * @param name      the name of the signal
		 * @param type      the type of the signal, @see SignalType
		 * @param wE        the exponent width
		 * @param wF        the significand width
		 */
		Signal(const std::string name, const SignalType type, const int wE, const int wF, const bool ieeeFormat=false);

		/** Signal destructor.
		 */		
		~Signal();
	
		/** Returns the name of the signal
		 * @return the name of the signal
		 */	
		const std::string& getName() const;


		/** Returns the width of the signal
		 * @return the width of the signal
		 */	
		int width() const;

	
		/** Returns the exponent width of the signal
		 * @return the width of the exponent if signal is isFP_
		 */	
		int wE() const;

		/** Returns the fraction width of the signal
		 * @return the width of the fraction if signal is isFP_
		 */	
		int wF() const;
	
		/** Reports if the signal is a FloPoCo floating-point signal
		 * @return if the signal is a FP signal
		 */	
		bool isFP() const;
	
		/** Reports if the signal is an IEEE floating-point signal
		 * @return if the signal is a FP signal
		 */	
		bool isIEEE() const;

		/** Reports if the signal has the bus flag active
		 * @return true if the signal is of bus type (std_logic_vector)
		 */		
		bool isBus() const;

		/** Returns the type of the signal
		 * @return type of signal, @see SignalType
		 */	
		SignalType type() const;
	
		/** outputs the VHDL code for declaring this signal 
		 * @return the VHDL for this signal. 
		 */	
		std::string toVHDL();

		/** obtain the name of a signal delayed by delay 
		 * @param delay*/
		std::string delayedName(int delay);


		/** outputs the VHDL code for declaring a signal with all its delayed versions
		 * This is the 2.0 equivalent of toVHDL()
		 * @return the VHDL for the declarations. 
		 */	
		std::string toVHDLDeclaration();


		/** sets the cycle at which the signal is active
		 */	
		void setCycle(int cycle) ;


		/** obtain the declared cycle of this signal
		 * @return the cycle
		 */	
		int getCycle();


		/** Updates the max delay associated to a signal
		 */	
		void updateLifeSpan(int delay) ;


		/** obtain max delay that has been applied to this signal
		 * @return the max delay 
		 */	
		int getLifeSpan() ;

		/** obtain the delay of this signal
		 * @return the delay
		 */	
		double getDelay();

		/** sets the delay of this signal
		 * @param[in] delay the delay
		 */	
		void setDelay(double delay);


		/** Set the number of possible output values. */
		void  setNumberOfPossibleValues(int n);


		/** Get the number of possible output values. */
		int getNumberOfPossibleValues(); 

		/**
		 * Converts the value of the signal into a nicely formated VHDL expression,
		 * including padding and putting quot or apostrophe.
		 * @param v value
		 * @param quot also put quotes around the value
		 * @return a string holding the value in binary
		 */
		std::string valueToVHDL(mpz_class v, bool quot = true);
	
		/**
		 * Converts the value of the signal into a nicely formated VHDL expression,
		 * including padding and putting quot or apostrophe. (Hex version.)
		 * @param v value
		 * @param quot also put quotes around the value
		 * @return a string holding the value in hexa
		 */
		std::string valueToVHDLHex(mpz_class v, bool quot = true);


	private:
		std::string   name_;        /**< The name of the signal */
		SignalType    type_;        /**< The type of the signal, see SignalType */
		int           width_;       /**< The width of the signal */
		
		int           numberOfPossibleValues_; /**< For signals of type out, indicates how many values will be acceptable. Typically 1 for correct rounding, and 2 for faithful rounding */

		int           lifeSpan_;    /**< The max delay that will be applied to this signal; */
		int           cycle_;       /**<  the cycle at which this signal is active in a pipelined operator. 0 means synchronized with the inputs */


	
		bool          isFP_;        /**< If the signal is of the FloPoCo floating-point type */  
		bool          isIEEE_;      /**< If the signal is of the IEEE floating-point type */  
		int           wE_;          /**< The width of the exponent. Used for FP signals */
		int           wF_;          /**< The width of the fraction. Used for FP signals */
		
		bool          isBus_;       /**< True is the signal is a bus (std_logic_vector)*/
		double        delay_;       /**<  the delay of the signal, starting from a previous register level */

		
	};

}

#endif

