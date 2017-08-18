#ifndef DSP_HPP
#define DSP_HPP


namespace flopoco{

	class DSP
	{
	public:
		/** The default constructor. Creates a DSP configuration valid for all targets: 
		 * 18x18 multipliers, no shifter, using one adder inside the block and is
		 * not configured to do multiply-accumulate
		 */ 
		DSP()   {
			maxMultiplierWidth_  = 18;
			maxMultiplierHeight_ = 18;
			multiplierWidth_     = 0;
			multiplierHeight_    = 0;
			fixedShift_          = 0;
			nrAdders_            = 1;
			multAccumulate_      = false;
			posPop = posPush = max_pos=availablepos=0;
			nrOfPrimitiveDSPs=1;
		}
	
		DSP(int Shift, int maxMultWidth, int maxMultHeight);
	
		/** The destructor */
		virtual ~DSP() {}

		/** Returns the with of the multiplier that this DSP block is using 
		 * @return the with of the multiplier that this DSP block is using */
		int getMultiplierWidth();
	
		/** Assigns the designated width to the multiplier of the DSP block
		 * @param w width assigned to the multiplier of the DSP block.
		 */
		void setMultiplierWidth(int w);
		
		/** Returns the height of the multiplier that this DSP block is using 
		 * @return the height of the multiplier that this DSP block is using */
		int getMultiplierHeight();
	
		/** Assigns the designated height to the multiplier of the DSP block
		 * @param w height assigned to the multiplier of the DSP block.
		 */
		void setMultiplierHeight(int w);
	
		/** Returns the amount by which an input can be shifted inside the DSP block 
		 * @return the amount by which an input can be shifted inside the DSP block */
		int getShiftAmount();
	
		/** Returns the maximum with of the multiplier that this DSP block is using 
		 * @return the maximum with of the multiplier that this DSP block is using */
		int getMaxMultiplierWidth();
	
		
		/** Returns the maximum height of the multiplier that this DSP block is using 
		 * @return the maximum height of the multiplier that this DSP block is using */
		int getMaxMultiplierHeight();
	
		/** Returns the number of adders this DSP block is using
		 * @return the number of adders this DSP block is using */
		int getNumberOfAdders();
	
		/** Assign the number of adders used by this DSP block in order to know the number of addition operands.
		 * @param o	number of addition operands
		 */
		void setNumberOfAdders(int n);
	
		/** Returns TRUE if the DSP block is configured to perform multiply-accumulate
		 * @return TRUE if the DSP block is configured to perform multiply-accumulate */
		bool isMultiplyAccumulate();
	
		/** Sets a flag that will configure the DSP block to multiply-accumulate when TRUE.
		 * @param m value assigned to multiply-accumulate flag
		 */
		void setMultiplyAccumulate(bool m);
	
		/** Procedure that returns by the value of its two parameters the coordinates
		 * the DSP within a tiling.
		 * @param xT the horizontal coordinate of the top-right corner
		 * @param yT the vertical coordinate of the top-right corner
		 * @param xB the horizontal coordinate of the bottom-left corner
		 * @param yB the vertical coordinate of the bottom-left corner
		 */
		void getCoordinates(int &xT, int &yT, int &xB, int &yB);
	
		/** Procedure that return by the value of its two parameters the coordinates
		 * of the top-right corner of the DSP within a tiling.
		 * @param x the horizontal coordinate of the top-right corner
		 * @param y the vertical coordinate of the top-right corner
		 */
		void getTopRightCorner(int &x, int &y);
	
		/** Assigns the x and y coordinates of the top-right corner of the DSP within the tiling.
		 * @param x the horizontal coordinate of the top-right corner
		 * @param y the vertical coordinate of the top-right corner
		 */
		void setTopRightCorner(int x, int y);
	
		/** Procedure that return by the value of its two parameters the coordinates
		 * of the bottom-left corner of the DSP within a tiling.
		 * @param x the horizontal coordinate of the bottom-left corner
		 * @param y the vertical coordinate of the bottom-left corner
		 */
		void getBottomLeftCorner(int &x, int &y);
	
		/** Assigns the x and y coordinates of the bottom-left corner of the DSP within the tiling.
		 * @param x the horizontal coordinate of the bottom-left corner
		 * @param y the vertical coordinate of the bottom-left corner
		 */
		void setBottomLeftCorner(int x, int y);
	
		/** Returns a reference to the DSP object whose output is the shifted input
		 * for this DSP block. This method will be mainly used for Virtex architecture. 
		 * @return a reference to the DSP object whose output is the shifted input for this DSP block*/
		DSP* getShiftIn();
	
		/** Assign the DSP object whose output is the shifted input for this DSP block.
		 *  This method will be mainly used for Virtex architecture.
		 * @param d the DSP object whose output is the shifted input for this DSP block.
		 */
		void setShiftIn(DSP* d);
	
		/** Returns a reference to the DSP object whose shifted input is the output
		 * of this DSP block. This method will be mainly used for Virtex architecture 
		 * @return a reference to the DSP object whose shifted input is the output of this DSP block*/
		DSP* getShiftOut();
	
		/** Assign the DSP object whose shifted input is the output of this DSP block.
		 *  This method will be mainly used for Virtex architecture.
		 * @param d the DSP object whose shifted input is the output of this DSP block.
		 */
		void setShiftOut(DSP* d);
	
		/** Returns references to all DSP objects that must be added with this DSP.
		 * @return  references to all DSP objects that must be added with this DSP */
		DSP** getAdditionOperands();
	
		/** Assigns an array containing the operands of the addition that this DSP is part of.
		 * @param o the array containing the operands of the addition that this DSP is part of.
		 */
		void setAdditionOperands(DSP** o);
	
		/** Swaps the width and height of the DSP block. This is used in case of asymetrical DSP
		 * blocks when we want to do a vertical/horizontal replacement of the block and we need
		 * to rotate the block by 90 degrees.
		 */
		void rotate();

		/** Assigns the shift amount that is used within this DSP block.
		 * @param s shift amount used within this DSP block.
		 */
		void setShiftAmount(int s);

		
		void allocatePositions(int dimension);
		void push(int X, int Y);
		int pop();
		void setPosition(int p);
		
		void tilingResetPosition();
		
		void resetPosition();
		int getAvailablePositions();
		int getCurrentPosition();
		
		int getNrOfPrimitiveDSPs();
		void setNrOfPrimitiveDSPs(int nr);

		void resetRotation();

		/** function that returns true if the multiplier is to be used 
		 *  with inputs X and Y swapped
		 *  @return true if DSP inputs are swapped
		*/
		void setRotated(bool rotateValue);

		/** function that returns true if the asymetrical multiplier is rotated 
		  * @return true if DSP inputs are swapped
		*/
		bool isRotated();		

		/** one DSP has multiple possible configurations (positions where it can be placed in a tiling)
		 *  this function assigns to the current DSP position the p-th configuration;
		 * @param[in] p       the configuration index
		 */ 
		void setConfiguration(int p);

		int *Xpositions;
		int *Ypositions;
	
	protected:
		int    nrOfPrimitiveDSPs;
		int    multiplierWidth_;	/**< The recommended width of the multiplier used by this DSP */
		int    maxMultiplierWidth_;	/**< The recommended width of the multiplier used by this DSP */
		int    multiplierHeight_;	/**< The recommended width of the multiplier used by this DSP */
		int    maxMultiplierHeight_;	/**< The recommended width of the multiplier used by this DSP */

		int    fixedShift_;         /**< The shift capacity of one input of the DSP block */
		int    nrAdders_;          	/**< The number of adders used by this DSP reflects the number of multipliers used and the number of outputs */
		bool   multAccumulate_;     /**< If true the DSP block will be configured as Multiply-Accumulate */
	
		// attributes used for tiling algorithm
		int 	positionX_[2];		/**< The X coordinates for the 2 points that mark the position of the DSP block inside the tiling */
		int		positionY_[2];		/**< The Y coordinates for the 2 points that mark the position of the DSP block inside the tiling */
		DSP*   	shiftIn_;        	/**< The DSP block from which this block obtains a shifted operand (Virtex4) */
		DSP*	shiftOut_;			/**< The DSP block to which this block provides a shifted operand (Virtex4) */
		DSP**	addOperands_;		/**< The DSP blocks whose multiplication results may be added to this one depending on the number of adders used (StratixII)*/

	private:
	
		bool rotated;               /**< sets the multiplier configuration from X x Y to Y x X (used with asymmetrical multipliers)*/
	

	
	
	
		int posPop;
		int posPush;
		int max_pos;
		int availablepos;
	};

}

#endif
