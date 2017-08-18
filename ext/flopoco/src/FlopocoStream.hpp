#ifndef FlopocoStream_HPP
#define FlopocoStream_HPP
#include <vector>
#include <sstream>
#include <utility>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include <cstdlib>

//#include "VHDLLexer.hpp"
#include "LexerContext.hpp"

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

namespace flopoco{


	/** 
	 * The FlopocoStream class.
	 * Segments of code having the same pipeline informations are scanned 
	 * on-the-fly using flex++ to find the VHDL signal IDs. The found IDs
	 * are augmented with pipeline depth information __IDName__PipelineDepth__
	 * for example __X__2__.
	 */
	class FlopocoStream{
	
		/* Methods for overloading the output operator available on streams */
		/*friend FlopocoStream& operator <= (FlopocoStream& output, string s) {

			output.vhdlCodeBuffer << " <= " << s;
			return output;
		}*/


		template <class paramType> friend FlopocoStream& operator <<(FlopocoStream& output, paramType c) {
			output.vhdlCodeBuffer << c;
			return output;
		}
		

		
		friend FlopocoStream & operator<<(FlopocoStream& output, FlopocoStream fs) {
			output.vhdlCodeBuffer << fs.str();
			return output; 
		}
		
		friend FlopocoStream& operator<<( FlopocoStream& output, UNUSED(ostream& (*f)(ostream& fs)) ){
			output.vhdlCodeBuffer << std::endl;
			return output;
		}
		
		public:
			/**
			 * FlopocoStream constructor. 
			 * Initializes the two streams: vhdlCode and vhdlCodeBuffer
			 */
			FlopocoStream();

			/**
			 * FlopocoStream destructor
			 */
			~FlopocoStream();

			/**
			 * method that does the similar thing as str() does on ostringstream objects.
			 * Processing is done using a buffer (vhdlCodeBuffer). 
			 * The output code is = previously transformed code (vhdlCode) + 
			 *  newly transformed code ( transform(vhdlCodeBuffer) );
			 * The transformation on vhdlCodeBuffer is done when the buffer is 
			 * flushed 
			 * @return the augmented string encapsulated by FlopocoStream  
			 */
			string str(){
				flush(currentCycle_);
				return vhdlCode.str();
			}

			/** 
			 * Resets both the buffer and the code stream. 
			 * @return returns empty string for compatibility issues.
			 */ 
			string str(string UNUSED(s) ){
				vhdlCode.str("");
				vhdlCodeBuffer.str("");
				return "";
			}
			
			/**
			 * the information from the buffer is flushed when cycle information
			 * changes. In order to annotate signal IDs with with cycle information
			 * the cycle information needs to be passed to the flush method 
			 * @param[in] currentCycle the current pipeline level for the vhdl code
			 *            from the buffer
			 */
			void flush(int currentCycle){
				if (! disabledParsing ){
					ostringstream bufferCode;
					if ( vhdlCodeBuffer.str() != string("") ){
						/* do processing if buffer is not empty */
					
						/* scan buffer sequence and annotate ids */
						bufferCode << annotateIDs( currentCycle );
					
						/* the newly processed code is appended to the existing one */
						vhdlCode << bufferCode.str();
					
					}
				}else{
					vhdlCode << vhdlCodeBuffer.str();				
				}
				/* reset buffer */
				vhdlCodeBuffer.str(""); 
			}
			
			/** 
			 * Function used to flush the buffer (annotate the last IDs from the
			 * buffer) when constructor has finished writing into the vhdl stream
			 */ 
			void flush(){
				flush ( currentCycle_ );
			}
			
			/**
			 * Function that annotates the signal IDs from the buffer with 
			 * __IDName__CycleInfo__
			 * @param[in] currentCycle Cycle Information
			 * @return the string containing the annotated information
			 */
			string annotateIDs( int currentCycle ){
//				vhdlCode << "-- CurrentCycle is = " << currentCycle << endl;
				ostringstream vhdlO;
				istringstream in( vhdlCodeBuffer.str() );
				/* instantiate the flex++ object  for lexing the buffer info */
				LexerContext* lexer = new LexerContext(&in, &vhdlO);
				/* This variable is visible from within the flex++ scanner class */
				lexer->yyTheCycle = currentCycle;
				/* call the FlexLexer ++ on the buffer. The lexing output is
				 in the variable vhdlO. Additionally, a temporary table contating
				 the tuples <name, cycle> is created */
				lexer->lex();
				/* the temporary table is used to update the member of the FlopocoStream
				 class useTable */

				updateUseMap(lexer);
				/* the annotated string is returned */
				return vhdlO.str();
			}
			
			/**
			 * The extern useTable rewritten by flex for each buffer is used 
			 * to update a useTable contained locally as a member variable of 
			 * the FlopocoStream class
			 * @param[in] tmpUseTable a vector of pairs which will be copied 
			 *            into the member variable useTable 
			 */
			void updateUseTable(vector<pair<string,int> > tmpUseTable){
				vector<pair<string, int> >::iterator iter;
				for (iter = tmpUseTable.begin(); iter!=tmpUseTable.end();++iter){
					pair < string, int> tmp;
					tmp.first  =  (*iter).first;
					tmp.second = (*iter).second;
					useTable.push_back(tmp);
				}			
			}

			/**
			 * A wrapper for updateUseTable
			 * The external table is erased of information
			 */			
			void updateUseMap(LexerContext* lexer){
				updateUseTable(lexer->theUseTable);
				lexer->theUseTable.erase(lexer->theUseTable.begin(), lexer->theUseTable.end());
			}
			
			void setCycle(int cycle){
				currentCycle_=cycle;
			}

			/**
			 * member function used to set the code resulted after a second parsing
			 * was perfromed
			 * @param[in] code the 2nd parse level code 
			 */
			void setSecondLevelCode(string code){
				vhdlCode.str("");
				vhdlCode << code;
			}
			
			/**
			 * Returns the useTable
			 */  
			vector<pair<string, int> > getUseTable(){
				return useTable;
			}


			void disableParsing(bool s){
				disabledParsing = s;
			}
			
			bool isParsing(){
				return !disabledParsing;
			}
			
			bool isEmpty(){
				return ((vhdlCode.str()).length() == 0 && (vhdlCodeBuffer.str()).length() == 0 );
			}


			ostringstream vhdlCode;              /**< the vhdl code afte */
			ostringstream vhdlCodeBuffer;        /**< the vhdl code buffer */
			
			int currentCycle_;                   /**< the current cycle is used in the picewise code scanning */
	
			vector<pair<string, int> > useTable; /**< table contating <id, cycle> info */

		protected:
		
			bool disabledParsing;
	};
}
#endif
