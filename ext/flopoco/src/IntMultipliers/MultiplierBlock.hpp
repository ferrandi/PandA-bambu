#ifndef MultiplierBlock_HPP
#define MultiplierBlock_HPP

#include <stdio.h>
#include <string>
#include <stdlib.h>

using namespace std;


namespace flopoco {

	class MultiplierBlock 
	{
	public: 
		
		/**
		 * The default constructor
		 */
		MultiplierBlock(int wX, int wY, int topX, int topY, string input1, string input2, int weightShift = 0, int cycle = -1);
	
		
		/**
		 * Set the value of the output signal name
		 * @param name= the new value
		 */
		void setSignalName(string name);

		/**
		 * Set the value of the output signal length
		 * @param length= the new length
		 */
		void setSignalLength(int length);

		/**
		*returns the weight of the multiplierBlock (top right corner)		
		**/
		int getWeight()
		{
			return weight;
		}


		/**
		*returns the output signal name
		**/
		string getSigName();

		/**
		*returns the output signal length
		**/
		int getSigLength();

		/**
		*returns the width of the DSP
		**/
		int getwX();

		/**
		*returns the height of the DSP
		**/
		int getwY();

		/**
		*returns the top right corner X coordinate
		**/
		int gettopX();

		/**
		*returns the top right corner Y coordinate
		**/
		int gettopY();

		/**
		*returns the bottom left corner X coordinate
		**/
		int getbotX();
		
		/**
		*returns the bottom left corner Y coordinate
		**/
		int getbotY();

		int getCycle();


		/**
		*returns the name of the first signal which enters in DSP. 
		*for example if the DSP is doing a multiplication like Z <=X * Y
		*then X is the first input signal and Y is the second input signal
		**/
		string getInputName1()
		{return inputName1;}

		/**
		*returns the name of the second  signal which enters in DSP. 
		*for example if the DSP is doing a multiplication like Z <=X * Y
		*then X is the first input signal and Y is the second input signal
		**/
		string getInputName2()
		{return inputName2;}
		
		/**
		*verifies if two block can be chained in the same supertile or not
		**/
		bool canBeChained(MultiplierBlock* next, bool isXilinx);

		/**
		 * checks if two blocks are adjacent
		**/
		bool neighbors(MultiplierBlock* next); 

		/**
		*returns the next block which is chained with the current block in the supertile
		**/
		MultiplierBlock* getNext();

		/**
		*returns the previous block which is chained with the current block in the supertile
		**/
		MultiplierBlock* getPrevious();

		/**
		 *sets the next block which is chained with the current block in the supertile
		 **/
		void setNext(MultiplierBlock* b);

		/**
		*sets the previous block which is chained with the current block in the supertile
		**/
		void setPrevious(MultiplierBlock* b);
		
			

		bool operator <= (MultiplierBlock* b);



		
	private:
			
		int wX; 							/**< x size */
		int wY; 							/**< y size */
		int topX; 							/**< x position in a larger multiplier (top right corner) */
		int topY; 							/**< y position (top right corner */
		int cycle;							/**< cycle */
		MultiplierBlock* previous;
		MultiplierBlock* next;
		string signalName;					/**<the name of the output signal*/
		int signalLength;					/**<the length of the output signal*/
		int weight;
		int weightShift;					/**<wFull-wOut-g*/
		string inputName1, inputName2;		/**<names of the inputs*/
		string srcFileName;
	};

}
#endif
