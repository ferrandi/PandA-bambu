// general c++ library for manipulating streams
#include <iostream>
#include <sstream>
#include <math.h>	// for NaN
#include <fstream>

/* header of libraries to manipulate multiprecision numbers
  There will be used in the emulate function to manipulate arbitraly large
  entries */
#include "gmp.h"
//#include "mpfr.h"
//#include "FPNumber.hpp"

// include the header of the Operator
#include "LutSrRng.hpp"

using namespace std;

namespace flopoco
{
namespace random
{
	

// personalized parameter
string LutSrRng::operatorInfo = "LutSrRng info r t k";



struct rng_para
{
	int n,r,t,k;
	uint32_t s;
};

rng_para table[]={
	{1024 , 32 , 3 , 32 ,  0x1a5eb},
	{1024 , 32 , 4 , 32 ,  0x1562cd6},
	{1024 , 32 , 5 , 32 ,  0x1c48},
	{1024 , 32 , 6 , 32 ,  0x2999b26},
	{1280 , 40 , 3 , 32 ,  0xc51b5},
	{1280 , 40 , 4 , 32 ,  0x4ffa6a},
	{1280 , 40 , 5 , 32 ,  0x3453f},
	{1280 , 40 , 6 , 32 ,  0x171013},
	{1536 , 48 , 3 , 32 ,  0x76010},
	{1536 , 48 , 4 , 32 ,  0xc2dc4a},
	{1536 , 48 , 5 , 32 ,  0x4b2be0},
	{1536 , 48 , 6 , 32 ,  0x811a15},
	{1788 , 56 , 3 , 32 ,  0xa2aae},
	{1788 , 56 , 4 , 32 ,  0x23f5fd},
	{1788 , 56 , 5 , 32 ,  0x1dde4b},
	{1788 , 56 , 6 , 32 ,  0x129b8},
	{2048 , 64 , 3 , 32 ,  0x5f81cb},
	{2048 , 64 , 4 , 32 ,  0x456881},
	{2048 , 64 , 5 , 32 ,  0xbfbaac},
	{2048 , 64 , 6 , 32 ,  0x21955e},
	{2556 , 80 , 3 , 32 ,  0x276868},
	{2556 , 80 , 4 , 32 ,  0x2695b0},
	{2556 , 80 , 5 , 32 ,  0x2d51a0},
	{2556 , 80 , 6 , 32 ,  0x4450c5},
	{3060 , 96 , 3 , 32 ,  0x79e56},
	{3060 , 96 , 4 , 32 ,  0x9a7cd},
	{3060 , 96 , 5 , 32 ,  0x41a62},
	{3060 , 96 , 6 , 32 ,  0x1603e},
	{3540 , 112 , 3 , 32 ,  0x29108e},
	{3540 , 112 , 4 , 32 ,  0x27ec7c},
	{3540 , 112 , 5 , 32 ,  0x2e1e55},
	{3540 , 112 , 6 , 32 ,  0x3dac0a},
	{3900 , 128 , 3 , 32 ,  0x10023},
	{3900 , 128 , 4 , 32 ,  0x197bf8},
	{3900 , 128 , 5 , 32 ,  0xcc71},
	{3900 , 128 , 6 , 32 ,  0x14959e},
	{5064 , 160 , 3 , 32 ,  0x1aedee},
	{5064 , 160 , 4 , 32 ,  0x1a23b0},
	{5064 , 160 , 5 , 32 ,  0x1aaf88},
	{5064 , 160 , 6 , 32 ,  0x1f6302},
	{5064 , 192 , 3 , 32 ,  0x48a92},
	{5064 , 192 , 4 , 32 ,  0x439d3},
	{5064 , 192 , 5 , 32 ,  0x4637},
	{5064 , 192 , 6 , 32 ,  0x577ce},
	{6120 , 224 , 3 , 32 ,  0x23585f},		//Q1 not match the table in paper
	{6120 , 224 , 4 , 32 ,  0x25e3a1},
	{6120 , 224 , 5 , 32 ,  0x270f3f},
	{6120 , 224 , 6 , 32 ,  0x259047},
	{8033 , 256 , 3 , 32 ,  0x437c26},
	{8033 , 256 , 4 , 32 ,  0x439995},
	{8033 , 256 , 5 , 32 ,  0x43664f},
	{8033 , 256 , 6 , 32 ,  0x427ba2},
	{11213 , 384 , 3 , 32 ,  0x11d4d},		// not match the table in paper
	{11213 , 384 , 4 , 32 ,  0x23dd1},
	{11213 , 384 , 5 , 32 ,  0x257a8},
	{11213 , 384 , 6 , 32 ,  0x17bd8},
	{19937 , 624 , 3 , 32 ,  0xda8},
	{19937 , 624 , 4 , 32 ,  0xb433},
	{19937 , 624 , 5 , 32 ,  0x2fffb},
	{19937 , 624 , 6 , 32 ,  0x25c7d},
	{19937 , 750 , 3 , 32 ,  0xb433},
	{19937 , 750 , 4 , 32 ,  0xb433},
	{19937 , 750 , 5 , 32 ,  0xb433},
	{19937 , 750 , 6 , 32 ,  0xb433}
};

//no of elements in table
unsigned  int no_tuple=sizeof(table)/sizeof(table[0]);


// Simple LCG RNG
	static int LCG(uint32_t &s) 
	{ return (s=1664525UL*s+1013904223UL)>>16; }		

//permute from last element of vector
	static void Permute(uint32_t &s, std::vector<int> &p)		
	{ for(int j=p.size();j>1;j--) swap(p[j-1],p[LCG(s)%j]); }


	LutSrRng::LutSrRng(Target* target, int tr, int t, int k)
	: Operator(target), tr(tr), t(t), k(k), seedtap(0)
	{
		setCopyrightString("David Thomas, Junfei Yan (2012-)");
		srcFileName="LutSrRng";

  		// definition of the name of the operator
  		ostringstream name;
  		name << "lut_sr_rng_" << tr << "_" << t << "_" << k;
  		setName(name.str());
  		// Copyright 
  		setCopyrightString("Junfei Yan and David Thomas 2011");
		
		// We have recurrences of one cycle, anything more is an error
		setHasDelay1Feedbacks();

//============================================================================================================================
// 0: select suitable parameters n, s according to r and t; k=32
		int m=0;
		unsigned int q=0;

		do {	
			if ((table[q].t==t) && (table[q].r >= tr)) 
				{	m=1;
					n=table[q].n;
					s=table[q].s;
					r=table[q].r;
				}
			else q++;

		} while(m!=1);//&& q<no_tuple


		REPORT(INFO, "n=" << n << "  r=" << r << "  t=" << t << "  k=" << k << "  s=" << s)
			//	printf("\n n->%u   r->%u   t->%u   k->%u   s->%x\n", n, r, t, k, s);
//define the size of taps and cycle

		taps.resize(n);
		cycle.resize(n);
		perm.resize(r); 

 	        cs.assign(n,0);

		std::vector<int> outputs(r), len(r,0);  //zero array
		int bit;

		//printf("m=%d	n= %d	r=%d	k=%d\n", m, n, r, k);

//============================================================================================================================
//Matrix expansion

// 1: Create cycle through bits for seed loading
		for(i=0; i<r; i++)
		{	cycle[i]=perm[i]=(i+1)%r;	}

		outputs=perm;			//set current output of each fifo

// 2: Extend bit-wide FIFOs
		for( i=r;i<n;i++)
		{ 
			do{ bit=LCG(s)%r; 

			//printf("bit=%d	len[%d]= %d\n", bit, bit, len[bit]);

			  }		//randomly selects one bit has a SR length smaller than k
			while(len[bit]>=k);

		//std::cerr<<">jump out of loop\n";		
		//printf("i=%d	n= %d	r=%d	k=%d\n", i, n, r, k);
			cycle[i]=i;       
			swap(cycle[i], cycle[bit]);
			outputs[bit]=i;    len[bit]++;
		}
		
// 3: Loading connections
		for(i=0;i<n;i++) 
		{	taps[i].insert(cycle[i]);}


// 4: XOR connections		
		for(int j=1;j<t;j++)
		{ 
			Permute(s, outputs);
			for(i=0;i<r;i++)
			{
				taps[i].insert(outputs[i]);		
				if(taps[i].size()<taps[seedtap].size())		
					seedtap=i;
			}	
		}

// 5: Output permutation
		Permute(s, perm); 


//============================================================================================================================
//output taps

	/*
	ofstream fout;
	fout.open("taps.txt");
	for(int i=0; i<n; i++)
	{
		fout << i << "	->	";
		set<int>::iterator it=taps[i].begin();
		while(it!=taps[i].end())
		{
			fout << *it++ << "	";
		}
			fout << "\n";
	}

	fout<<flush;
	fout.close();
	*/
//============================================================================================================================
		addInput("m");			//mode-> m=1 load; m=0 RNG
		addInput("Sin");		//serial load input in load mode
		addOutput("RNG", tr, 1, true);
		addOutput("Sout");

	for (int i=0; i<n; i++)
	{
		declare(join("SR_",i),1,false, Signal::registeredWithZeroInitialiser);
	}

// 6: output FIFO connections in VHDL
	for (int i=0; i<n; i++)
	{
		//register SR
		vhdl << tab << use(join("SR_",i)) << "<=" << join("state_",i) << ";" << endl;
		nextCycle();
		set<int>:: iterator it=taps[i].begin();	//set it points to the first element of tap[i]
	// seedtap case
		if (i==seedtap)			
 		{	//vhdl << tab << declare(join("state_",i))<< " <= ";
			vhdl << tab << declare(join("state_",i))<< " <=  Sin WHEN m='1' ELSE ";
			while (it!= taps[i].end()) 
			{ vhdl << use(join("SR_",*it++)) <<" XOR ";}

			vhdl << "'0';\n" << endl;
		}

	//non-seedtap cases
		else 
		{	
			if(taps[i].size()==1){
				vhdl << tab << declare(join("state_",i)) << " <= " << use(join("SR_",cycle[i])) << ";\n";
			}else{
				vhdl << tab << declare(join("state_",i)) << " <= " << use(join("SR_",cycle[i])) << " WHEN m='1' ELSE ";

				set<int>:: iterator it=taps[i].begin();
				while (it!= taps[i].end()) 
				{ vhdl <<use(join("SR_",*it++)) <<" XOR ";}

				vhdl << "'0';\n" << endl;
			}

		}

		setCycleFromSignal("SR_0");

	}
		nextCycle();

// 7: r XOR connections for outputs

	for (int i=0; i<tr; i++)
		{vhdl << tab << "RNG" << of(i) << " <= " << use(join("state_", perm[i])) << ";" << endl;}

	vhdl << tab << "Sout <= " << use(join("SR_",cycle[seedtap])) << ";" << endl;


}

//============================================================================================================================

LutSrRng::~LutSrRng(){	
}

//============================================================================================================================
void LutSrRng::emulate(TestCase * tc) {
  mpz_class smode = tc->getInputValue("m");
  mpz_class ssin= tc->getInputValue("Sin");
  std::vector<int> rout(tr);	
  
  mpz_class sr=0;
  mpz_class temp=0;

  /* then manipulate our bit vectors in order to get the correct output*/

	 // Advance state cs[0:n-1] using inputs (m,s_in)

	ns.assign(n,0);

		//printf("seedtap1->%d\n", seedtap);	
		for( int i=0;i<n;i++)
		{ // Do XOR tree and FIFOs
			if(smode==0)
				{ // RNG mode
					std::set<int>::iterator it=taps[i].begin();
					while(it!=taps[i].end()) 
					{	
						ns[i] ^= cs[*it++];	

					}
				}
				else
				{ // load mode 
					ns[i]= (i==seedtap) ? mpz_get_ui(ssin.get_mpz_t()) : cs[cycle[i]];	

				}  

		}

		//std::cerr<<">all ns\n\n\n";
		
		// capture permuted output signals
		mpz_class s_out=cs[cycle[seedtap]]; // output of load chain
		
		cs = ns;	// simulate "clock-edge", so FFs toggle

		for( int i=0;i<tr;i++) 
		{		//std::cerr<<">in loop 3\n";
		//temp=0;

		rout[i]=cs[perm[i]];	

		mpz_set_ui(temp.get_mpz_t(),rout[i]);

		mpz_mul_2exp(temp.get_mpz_t(), temp.get_mpz_t(), i);			//MSB rout[0]; LSB rout[r-1]

		sr+=temp;			//find value for r outputs 

		//printf("rout[%d]->%d ", i,rout[i]);
		//gmp_printf("temp[%d]->%Zd	sr->%Zd\n", i, temp.get_mpz_t(),sr.get_mpz_t());

		}


  /* at the end, we indicate to the TestCase object what is the expected
    output corresponding to the inputs */
  tc->addExpectedOutput("RNG",sr);
  tc->addExpectedOutput("Sout",s_out);

	//std::cerr<<">over	RNG\n";

}


//============================================================================================================================
void LutSrRng::buildStandardTestCases(TestCaseList* tcl)
{
	//	std::cerr<<">standard testcases\n";

	TestCase *tc;
	mpz_class x;
	mpz_class mode;


//Test.1:	TEST_INIT_STATE
	
	for( int i=0;i<n;i++)
	{
		
	tc = new TestCase(this);
	mpz_class x = getLargeRandom(1);

	//gmp_printf("x= %Zd\n", x.get_mpz_t());

		mpz_class mode = 1;

		tc->addInput("Sin", x);
		tc->addInput("m", mode);

	emulate(tc);
	tcl->add(tc);

	//printf("i= %d	n=%d\n", i,n);

	}


//Test.2:	TEST_OUT_DATA
	
	for(int i=0;i<n*2;i++)			// Q7 2*n?
	{
	tc = new TestCase(this);
		mpz_class x = 0;
		mpz_class mode = 0;

		tc->addInput("Sin", x);
		tc->addInput("m", mode);

	emulate(tc);
	tcl->add(tc);
	}
	

//Test.3:	TEST_REF_READBACK

	for( int i=0;i<n;i++)
	{

	tc = new TestCase(this); 
		mpz_class x = 1;
		mpz_class mode = 1;

		tc->addInput("Sin", x);
		tc->addInput("m", mode);
	emulate(tc);
	tcl->add(tc);
	}

}


//============================================================================================================================


}; // random
}; // flopoco
