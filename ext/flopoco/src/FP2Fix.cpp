/*
*/

#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <string.h>

#include <gmp.h>

#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"

#include "FP2Fix.hpp"

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <list>
#include <map>
#include <vector>
#include <math.h>
#include <locale>

#include <stdio.h>
#include <mpfr.h>

#include "Shifters.hpp"

using namespace std;

namespace flopoco{

#define DEBUGVHDL 0


   FP2Fix::FP2Fix(Target* target, int _LSBO, int _MSBO, int _Signed,int _wEI, int _wFI, bool _trunc_p) :
         Operator(target), wEI(_wEI), wFI(_wFI), Signed(_Signed), LSBO(_LSBO), MSBO(_MSBO), trunc_p(_trunc_p) {

      int MSB=MSBO;
      int LSB=LSBO;
      Shifter*FXP_shifter;
      IntAdder* Exponent_difference;

      ostringstream name;

      if ((MSB < LSB)){
         cerr << " FP2Fix: Input constraint LSB <= MSB not met."<<endl;
         exit (EXIT_FAILURE);
      }
      if(LSB<0 && -LSB > wFI){
         cerr << " FP2Fix: Input constraint -LSB <= wFI not met."<<endl;
         exit (EXIT_FAILURE);
      }

      int wFO = MSB - LSB + 1;
      mpz_class maxExpWE = mpz_class(1)<<(wEI-1);
      mpz_class minExpWE = 1 - maxExpWE;

      int eMax = static_cast<int>(maxExpWE.get_si()) - 1;
      int wFO0;
      if(eMax+1 < MSB + 1)
         wFO0 = eMax + 1 - LSB;
      else
         wFO0 = MSB + 1 - LSB;

      if (( maxExpWE < MSB ) || ( minExpWE > LSB)){
         cerr << " The exponent is too small for full coverage. Try increasing the exponent !"<<endl;
         exit (EXIT_FAILURE);
      }
      int absMSB = MSB>=0?MSB:-MSB;
      int absLSB = LSB>=0?LSB:-LSB;
      name<<"FP2Fix_" << wEI << "_" << wFI << (LSB<0?"M":"") << "_" << absLSB << "_" << (MSB<0?"M":"") << absMSB <<"_"<< (Signed==1?"S":"US") << "_" << (trunc_p==1?"T":"NT");
      setName(name.str());

      setCopyrightString("Fabrizio Ferrandi (2012)");

      /* Set up the IO signals */

      addFPInput ("I", wEI,wFI);
      addOutput ("O", MSB-LSB+1);

      /*	VHDL code description	*/
      vhdl << tab << declare("eA0",wEI) << " <= I" << range(wEI+wFI-1,wFI) << ";"<<endl;
      vhdl << tab << declare("fA0",wFI+1) << " <= \"1\" & I" << range(wFI-1, 0)<<";"<<endl;
      mpz_class bias;
      bias = eMax - 1;
      vhdl << tab << declare("bias",wEI) << " <= not conv_std_logic_vector(" << bias << ", "<< wEI<<");"<<endl;

      Exponent_difference = new IntAdder(target, wEI);
      Exponent_difference->changeName(getName()+"Exponent_difference");
      oplist.push_back(Exponent_difference);
      inPortMap  (Exponent_difference, "X", "bias");
      inPortMap  (Exponent_difference, "Y", "eA0");
      inPortMapCst(Exponent_difference, "Cin", "'1'");
      outPortMap (Exponent_difference, "R","eA1");
      vhdl << instance(Exponent_difference, "Exponent_difference");
      
      setCycleFromSignal("eA1");
      setCriticalPath(Exponent_difference->getOutputDelay("R"));

      
      manageCriticalPath(target->localWireDelay() + target->lutDelay());
      int wShiftIn = intlog2(wFO0+2);
      if(wShiftIn < wEI)
        vhdl << tab << declare("shiftedby", wShiftIn) <<  " <= eA1" << range(wShiftIn-1, 0)                 << " when eA1" << of(wEI-1) << " = '0' else " << rangeAssign(wShiftIn-1,0,"'0'") << ";"<<endl;
      else
        vhdl << tab << declare("shiftedby", wShiftIn) <<  " <= " << rangeAssign(wShiftIn-wEI,0,"'0'") << " & eA1 when eA1" << of(wEI-1) << " = '0' else " << rangeAssign(wShiftIn-1,0,"'0'") << ";"<<endl;

      //FXP shifter mappings
      FXP_shifter = new Shifter(target, wFI+1, wFO0+2, Shifter::Left);
      oplist.push_back(FXP_shifter);

      inPortMap (FXP_shifter, "X", "fA0");
      inPortMap (FXP_shifter, "S", "shiftedby");
      outPortMap (FXP_shifter, "R", "fA1");
      vhdl << instance(FXP_shifter, "FXP_shifter");

      syncCycleFromSignal("fA1");
      setCriticalPath(FXP_shifter->getOutputDelay("R"));

      if(trunc_p)
      {
         if(Signed == 0)
         {
            vhdl << tab << declare("fA4",wFO) <<  "<= fA1" << range(wFO0+wFI+LSB, wFI+1+LSB)<< ";"<<endl;
         }
         else
         {
            vhdl << tab << declare("fA2",wFO) <<  "<= fA1" << range(wFO0+wFI+LSB, wFI+1+LSB)<< ";"<<endl;
            manageCriticalPath(target->localWireDelay() + target->adderDelay(wFO));
            vhdl << tab << declare("fA4",wFO) <<  "<= fA2 when I" << of(wEI+wFI) <<" = '0' else -signed(fA2);" <<endl;
         }
      }
      else
      {
         vhdl << tab << declare("fA2a",wFO+1) <<  "<= '0' & fA1" << range(wFO0+wFI+LSB, wFI+1+LSB)<< ";"<<endl;
         IntAdder* MantSum;
         if(Signed == 0)
         {
            manageCriticalPath(target->localWireDelay() + target->lutDelay());
            vhdl << tab << declare("notallzero") << " <= '0' when fA1" << range(wFI+LSB-1, 0) << " = " << rangeAssign(wFI+LSB-1, 0,"'0'") << " else '1';"<<endl;
            vhdl << tab << declare("round") << " <= fA1" << of(wFI+LSB) << " and notallzero ;"<<endl;
         }
         else
         {
            manageCriticalPath(target->localWireDelay() + target->lutDelay());
            vhdl << tab << declare("notallzero") << " <= '0' when fA1" << range(wFI+LSB-1, 0) << " = " << rangeAssign(wFI+LSB-1, 0,"'0'") << " else '1';"<<endl;
            vhdl << tab << declare("round") << " <= (fA1" << of(wFI+LSB) << " and I" << of(wEI+wFI) << ") or (fA1" << of(wFI+LSB) << " and notallzero and not I" << of(wEI+wFI) << ");"<<endl;
         }   
         vhdl << tab << declare("fA2b",wFO+1) <<  "<= '0' & " << rangeAssign(wFO-1,1,"'0'") << " & round;"<<endl;
         MantSum = new IntAdder(target, wFO+1);
         MantSum->changeName(getName()+"MantSum");
         oplist.push_back(MantSum);
         inPortMap  (MantSum, "X", "fA2a");
         inPortMap  (MantSum, "Y", "fA2b");
         inPortMapCst(MantSum, "Cin", "'0'");
         outPortMap (MantSum, "R","fA3");
         vhdl << instance(MantSum, "MantSum");
         setCycleFromSignal("fA3");
         setCriticalPath(MantSum->getOutputDelay("R"));
         if(Signed == 0)
         {
            vhdl << tab << declare("fA4",wFO) <<  "<= fA3" << range(wFO-1, 0)<< ";"<<endl;
         }
         else
         {
            manageCriticalPath(target->localWireDelay() + target->adderDelay(wFO+1));
            vhdl << tab << declare("fA3b",wFO+1) <<  "<= -signed(fA3);" <<endl;
            manageCriticalPath(target->localWireDelay() + target->lutDelay());
            vhdl << tab << declare("fA4",wFO) <<  "<= fA3" << range(wFO-1, 0) << " when I" << of(wEI+wFI) <<" = '0' else fA3b" << range(wFO-1, 0) << ";" <<endl;
         }
      }
      if (eMax > MSB)
      {
         vhdl << tab << declare("overFl0") << "<= '1' when I" << range(wEI+wFI-1,wFI) << " > conv_std_logic_vector("<< eMax+MSB << "," << wEI << ") else I" << of(wEI+wFI+2)<<";"<<endl;
      }
      else
      {
         vhdl << tab << declare("overFl0") << "<= I" << of(wEI+wFI+2)<<";"<<endl;
      }
      
      if(trunc_p)
      {
         if(Signed == 0)
            vhdl << tab << declare("overFl1") << " <= fA1" << of(wFO0+wFI+1+LSB) << ";"<<endl;
         else
         {
            manageCriticalPath(target->localWireDelay() + target->lutDelay());
            vhdl << tab << declare("notZeroTest") << " <= '1' when fA4 /= conv_std_logic_vector(0," << wFO <<")"<< " else '0';"<<endl;
            vhdl << tab << declare("overFl1") << " <= (fA4" << of(wFO-1) << " xor I" << of(wEI+wFI) << ") and notZeroTest;"<<endl;
        }
      }
      else
      {
         vhdl << tab << declare("overFl1") << " <= fA3" << of(wFO) << ";"<<endl;
      }

      manageCriticalPath(target->localWireDelay() + target->lutDelay());
      vhdl << tab << declare("eTest") << " <= (overFl0 or overFl1);" << endl;

      manageCriticalPath(target->localWireDelay() + target->lutDelay());
      vhdl << tab << "O <= fA4 when eTest = '0' else" << endl;
      vhdl << tab << tab << "I" << of(wEI+wFI) << " & (" << wFO-2 << " downto 0 => not I" << of(wEI+wFI) << ");"<<endl;
   }


   FP2Fix::~FP2Fix() {
   }


   void FP2Fix::emulate(TestCase * tc)
   {
      /* Get I/O values */
      mpz_class svI = tc->getInputValue("I");
      FPNumber  fpi(wEI, wFI, svI);
      mpfr_t i;
      mpfr_init2(i, 1+wFI);
      fpi.getMPFR(i);
      //std::cerr << "FP " << printMPFR(i, 100) << std::endl;
      mpz_class svO;
      
      mpfr_t cst, tmp2;
      mpfr_init2(cst, 10000); //init to infinite prec
      mpfr_init2(tmp2, 10000); //init to infinite prec
      mpfr_set_ui(cst, 2 , GMP_RNDN);
      mpfr_set_si(tmp2, -LSBO , GMP_RNDN);
      mpfr_pow(cst, cst, tmp2, GMP_RNDN);
      mpfr_mul(i, i, cst, GMP_RNDN);

      if(trunc_p)
         mpfr_get_z(svO.get_mpz_t(), i, GMP_RNDZ);
      else
         mpfr_get_z(svO.get_mpz_t(), i, GMP_RNDN);
      
      if (Signed != 0)
      {
         mpz_class tmpCMP = (mpz_class(1)  << (MSBO-LSBO))-1;
         if (svO > tmpCMP){ //negative number 
            mpz_class tmpSUB = (mpz_class(1) << (MSBO-LSBO+1));
            svO = svO - tmpSUB;
	 }
      }
      //std::cerr << "FIX " << svO << std::endl;
      tc->addExpectedOutput("O", svO);
      // clean-up
      mpfr_clears(i,cst, tmp2, NULL);
  }
  
  TestCase* FP2Fix::buildRandomTestCase(int i)
  {
     TestCase *tc;
     mpz_class a;
     tc = new TestCase(this); 
     mpz_class e = (getLargeRandom(wEI+wFI) % (MSBO+(Signed?0:1))); // Should be between 0 and MSBO+1/0
     mpz_class normalExn = mpz_class(1)<<(wEI+wFI+1);
     mpz_class bias = ((1<<(wEI-1))-1);
     mpz_class sign = Signed ? getLargeRandom(1) : 0;
     e = bias + e;
     a = getLargeRandom(wFI) + (e << wFI) + (sign << wFI+wEI) + normalExn;
     tc->addInput("I", a);
     /* Get correct outputs */
     emulate(tc);
     return tc;		
   }
   

   void FP2Fix::buildStandardTestCases(TestCaseList* tcl){
	   // please fill me with regression tests or corner case tests!

   }

}
