#ifndef PolynomialEvaluator_HPP
#define PolynomialEvaluator_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <gmpxx.h>
#include "../utils.hpp"

#include "../IntMultiplier.hpp"
#include "../IntAdder.hpp"
#include "../Operator.hpp"


namespace flopoco{

#define MINF -16384

	
	/**
	 * The FixedPointCoefficinet class provides the objects for representing 
	 * fixed-point coefficinets. It is used by the polynomial evaluator
	 */
	class FixedPointCoefficient
	{
	public:
		
		FixedPointCoefficient(FixedPointCoefficient *x){
			size_= x->getSize();
			weight_ = x->getWeight();
		}

		/**
		 * Constructor. Initializes the attributes.
		 */
		FixedPointCoefficient(unsigned size, int weight){
			size_   = size;
			weight_  = weight;
		}

		FixedPointCoefficient(unsigned size, int weight, mpfr_t valueMpfr){
			size_   = size;
			weight_  = weight;
			valueMpfr_=(mpfr_t*)malloc(sizeof(mpfr_t));
			mpfr_init2(*valueMpfr_, mpfr_get_prec(valueMpfr));
			mpfr_set(*valueMpfr_, valueMpfr, GMP_RNDN);
		}
			
		/** Destructor */
		~FixedPointCoefficient();

		/** 
		 * Fetch the weight of the MSB
		 * @return weight of the MSB
		 */
		int getWeight(){
			return weight_;
		} 

		/** 
		 * Set the weight of the MSB
		 */
		void setWeight(int w){
			weight_=w;
		} 


		/** 
		 * Fetch the size (width) of the coefficient
		 * @return coefficient width
		 */
		unsigned getSize(){
			return size_;
		}

		void setSize(unsigned s){
			size_ = s;
		}

			
		mpfr_t* getValueMpfr(){
			return valueMpfr_;
		}


	protected:
		int size_;        /**< The width (in bits) of the coefficient. In theory this equals to the
		                     minimum number of bits required to represent abs(value) */
		int weight_;      /**< The shift value. Coefficient is represented as value * 2 (shift) */
			
		mpfr_t* valueMpfr_;
	};

	/**
	 * The YVar class
	 */
	class YVar: public FixedPointCoefficient
	{
	public:
		/** constructor */	
		YVar( unsigned size, int weight, const int sign = 0 ):
			FixedPointCoefficient(size, weight), sign_(sign){
		}

		/** Destructor */
		~YVar(){};

		/**
		 * Fetch the variable size (if known). 0 = unknown
		 * @return sign
		 */ 
		int getSign(){
			return sign_;
		}
			
	protected:
		int sign_;        /**< The variable sign in known */         
	};
	
	/** class LevelSignal. Models The level signals */
	class LevelSignal{
	public: 
		LevelSignal(string name, unsigned size, int shift):
			name_(name), size_(size), shift_(shift){
		}
			
		LevelSignal(LevelSignal* l){
			name_  = l->getName();
			size_  = l->getSize();
			shift_ = l->getWeight();
		}
			
		/* Destructor */
		~LevelSignal(){};

		string getName(){
			return name_;
		}
			
		unsigned getSize(){
			return size_;
		}
			
		int getWeight(){
			return shift_;
		}
			
	protected:
		string   name_; /**TODO Comment */
		unsigned size_;
		int      shift_;
	}; 
	
	/** horner variables, sigma and pi */
	class Sigma{
	public: 
		Sigma(int size1, int weight1, int size2, int weight2){
			if (size2 == 0) {
				size = size1; 
				weight = weight1;
				guardBits = 0;
				//					cout <<  "Created Sigma size=" << size << " weight="<<weight << " guardBits=" << guardBits << endl;
			}else{
				/* not sigma 0 */
				int msb = max ( weight1, weight2);
				int lsb = min ( int (weight1 + (weight1>=0?1:0) - size1), int(weight2 + (weight2>=0?1:0) - size2) );
					
				size = msb - lsb + 1 + 1; /* the second 1 is to overflow bit */
				weight = msb + 1;
					
				guardBits = abs (weight1 - weight2);
				//					cout <<  "Created Sigma size=" << size << " weight="<<weight << " guardBits=" << guardBits << endl;
			}
		}
			
		/* Destructor */
		~Sigma(){};

		unsigned getSize(){
			return size;
		}
			
		int getWeight(){
			return weight;
		}
			
		int getGuardBits(){
			return guardBits;
		}
			
	protected:
		unsigned size;
		int      weight;
		int      guardBits;
	}; 

	/** horner variables, sigma and pi */
	class Pi{
	public: 
		Pi(unsigned size1, int weight1, unsigned size2, int weight2):
			size(size1+size2), weight(weight1+weight2){
			//				cout << "Created Pi size=" << size << " weight="<<weight << endl;
		}
			
		/* Destructor */
		~Pi(){};

		unsigned getSize(){
			return size;
		}
			
		int getWeight(){
			return weight;
		}
			
	protected:
		unsigned size;
		int      weight;
	}; 

	
	
	

	/** 
	 * The PolynomialEvaluator class. Constructs the oprator for evaluating
	 * a polynomial shiftth fix-point coefficients.
	 */
	class PolynomialEvaluator : public Operator
	{
	public:

		/**
		 * The polynomial evaluator class. FIXME the parameters are subhect to change
		 * TODO document them
		 */
		PolynomialEvaluator(Target* target, vector<FixedPointCoefficient*> coef, YVar* y, int targetPrec, mpfr_t* approxError, map<string, double> inputDelays = emptyDelayMap);

		/** Destructor */
		~PolynomialEvaluator();
			
			
		/* ------------------------------ EXTRA -----------------------------*/
		/* ------------------------------------------------------------------*/
			
		/** Update coefficinents. 
		 * The input coefficients have the format: 
		 *           size   = #of bits at the right of the .
		 *           weight = the weight of the MSB
		 * for example 0.000CCCCCC would be size=9 and weight=-3
		 * and will get converted to        size=9-3=6 (#of C) weight = -3  
		 * @param coef A coefficinets returned according to TableGenerator  
		 */
		void updateCoefficients(vector<FixedPointCoefficient*> coef);

		/** Sets the polynomial degree 
		 * @param d the polynomial degree
		 */
		void setPolynomialDegree(int d){ if (d < 0) throw("Negative polynomial degree"); degree_ = d;}

		/** Gets the polynomial degree 
		 * @return the polynomial degree
		 */
		int getPolynomialDegree(){ return degree_; }
			
		/** print the polynomial 
		 * @param[in] coef  the coefficinets
		 * @param[in] y     the variable
		 * @param[in] level the initial recursion level 0
		 * @return          the polynomial string
		 */
		string printPolynomial( vector<FixedPointCoefficient*> coef, YVar* y, int level);
			
			
			
			
		/* ----------------- ERROR RELATED ----------------------------------*/
		/* ------------------------------------------------------------------*/
			
		/** sets the approximation error budget for the polynomial evaluator.
		 * @param p the error budget as an mpfr pointer
		 */			
		void setApproximationError( mpfr_t *p);
			
		/** sets the mpfr associated to the maximal value of y. This value
		 * is required in the error analysis step
		 * @param[in] y the variable containing info about the size and weight
		 */
		void setMaxYValue(YVar* y);

		/** perform errorVector allocations */
		void allocateErrorVectors();
			
		/** hide messy vector initializations */
		void allocateAndInitializeExplorationVectors();
			
		/** The function that does the error estimation starting from a 
		 * set of guard bits for the coefficients and a set of truncations
		 * for the Y
		 * @param[in] yGuard the vector containing the truncation error for Y
		 * @param[in] aGuard the vector with the guard bits for the coeffs.
		 * @return the maximum approximation error for this set of params
		 */
		mpfr_t* errorEstimator(vector<int> &yGuard, vector<int> &aGuard);

			
			
		void hornerGuardBitProfiler();
		
		/* ---------------- EXPLORATION RELATED -----------------------------*/
		/* ------------------------------------------------------------------*/
			
		/** hide messy vector exploration vectors initializations */
		void initializeExplorationVectors();
			
			
		/** fills-up a map of objective states for the truncations */
		void determineObjectiveStatesForTruncationsOnY();
			
		/** updates the MaxBoundY vector with the maximum number of states
		 * for each Y */
		void setNumberOfPossibleValuesForEachY();
			
		/** TODO PROPER WAY; it only accounts (for now) on the DPS block
		 *  size. We can use as many guard bits in order not to use extra
		 *  multipliers 
		 */
		void resetCoefficientGuardBits();
			
		void reinitCoefficientGuardBits();
			
		void reinitCoefficientGuardBitsLastIteration();
			
		/** Get range values for multiplications. We pefrom one error 
		 * analysis run where we dont't truncate or add any guard bits
		 */
		void coldStart(){
			mpfr_t* tmp; 
			tmp = errorEstimator(yGuard_, aGuard_);
			mpfr_clear(*tmp);
			free(tmp);
		}
			
		/** We allow possible truncation of y. Horner evaluation therefore 
		    allows for d differnt truncations on y (1 to d). For each y 
		    there are at most 3 possible truncation values:
		    -no truncation
		    -truncate so to fit x dimmension of the embedded multiplier
		    -truncate so to fit y dimmension of the embedded multiplier
		    * @param[in] i      What y are we talking about. Takes values in 1..d
		    * @param[in] level  which one of the 3 values are we talking about 0..2
		    * @return the corresponding value to the selected level 
		    */ 
		int getPossibleYValue(int i, int state);
		/** advances to the next step in the design space exploration on the
		 * y dimmension.
		 * @return true if there is a next state, false if a solution has been
		 *found.
		 */
		bool nextStateY();

		/** advances to the next step in the design space exploration on the
		 * coefficient guard bit direction.
		 * @return true if there is a next state, false if a solution has been
		 *found.
		 */
		bool nextStateA();
			
		unsigned getOutputSize(){
			return wR;
		}
			
		int getOutputWeight(){
			return weightR;
		}

			
		vector<FixedPointCoefficient*> getCoeffParamVector(){
			return coef_;
		}
			
		int getRWidth(){
			return wR;
		}
			
		int getRWeight(){
			return weightR;
		}

	protected:
		
		mpfr_t* approximationError; /* the error performed in the polynomial finding step */
		mpfr_t targetError;         /* the target error for this component. In most cases
		                               this should be equal to 1/2 ulp as the other 1/2 ulp
		                               is lost in the final rounding */

		unsigned wR;
		int weightR;
			
		vector<FixedPointCoefficient*> coef_;
		YVar* y_;
		int targetPrec_;
		unsigned degree_;
		mpfr_t maxABSy;
			
			
			
		vector<int> yGuard_; // negative values => truncation
		vector<int> yState_; 


		vector<int> aGuard_; // positive values refer to "real" guard bits

		vector<int> maxBoundY;
		vector<int> maxBoundA;
			
			
		int currentPrec;
		bool sol;
			
		float thresholdForDSP; 
			
		vector<signed> sigmakPSize,  pikPSize, pikPTSize;
		vector<signed> sigmakPWeight, pikPWeight, pikPTWeight;
		    
	private:
		multimap<int, int> objectiveStatesY;
	
		/* for efficiency we move these variables here */
		




	};
}

#endif
