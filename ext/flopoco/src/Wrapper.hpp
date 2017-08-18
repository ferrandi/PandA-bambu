#ifndef WRAPPER_HPP
#define WRAPPER_HPP
#include "Wrapper.hpp"

/** 
 * A wrapper is a VHDL entity that places registers before and after
 * an operator, so that you can synthesize it and get delay and area,
 * without the synthesis tools optimizing out your design because it
 * is connected to nothing.  
 **/

namespace flopoco{

	class Wrapper : public Operator
	{
	public:
		/** 
		 * The Wrapper constructor
		 * @param[in] target the target device
		 * @param[in] op the operator to be wrapped
		 **/
		Wrapper(Target* target, Operator* op);

		/** The destructor */
		~Wrapper();
	
	
	private:
		Operator* op_; /**< The operator to wrap */
	};
}
#endif
