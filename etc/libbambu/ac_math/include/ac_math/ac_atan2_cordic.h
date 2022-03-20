/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Math Library                                       *
 *                                                                        *
 *  Software Version: 2.0                                                 *
 *                                                                        *
 *  Release Date    : Thu Aug  2 11:19:34 PDT 2018                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 2.0.10                                              *
 *                                                                        *
 *  Copyright , Mentor Graphics Corporation,                     *
 *                                                                        *
 *  All Rights Reserved.                                                  *
 *
 **************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
 *                                                                        *
 *      http://www.apache.org/licenses/LICENSE-2.0                        *
 *                                                                        *
 *  Unless required by applicable law or agreed to in writing, software   *
 *  distributed under the License is distributed on an "AS IS" BASIS,     *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or       *
 *  implied.                                                              *
 *  See the License for the specific language governing permissions and   *
 *  limitations under the License.                                        *
 **************************************************************************
 *                                                                        *
 *  The most recent version of this package is available at github.       *
 *                                                                        *
 *************************************************************************/
//*****************************************************************************************
// File: ac_atan2_cordic.h
//
// Description:
//    Provides cordic based implementation of the inverse tan function for the
//    AC Datatype ac_fixed.
//
// Usage:
//    A sample testbench and its implementation look like
//    this:
//
//    #include <ac_fixed.h>
//    #include <ac_math/ac_atan2_cordic.h>
//    using namespace ac_math;
//
//    typedef ac_fixed< 7,  3, true, AC_RND, AC_SAT>  input_type;
//    typedef ac_fixed<12,  5, true, AC_RND, AC_SAT> output_type;
//
//    #pragma hls_design top
//    void project(
//      const input_type       &y,
//      const input_type       &x,
//      output_type &atan2_output,
//    )
//    {
//      ac_atan2_cordic(y, x, atan2_output);
//    }
//
//    #ifndef __BAMBU__
//    #include <mc_scverify.h>
//
//    CCS_MAIN(int arg, char **argc)
//    {
//      input_type y =  0.5;
//      input_type x = -0.5;
//      output_type atan2_output;
//      CCS_DESIGN(project)(input, atan2_output);
//      CCS_RETURN (0);
//    }
//    #endif
//
//*****************************************************************************************

#ifndef _INCLUDED_AC_ATAN2_CORDIC_H_
#define _INCLUDED_AC_ATAN2_CORDIC_H_

#ifndef __cplusplus
#error C++ is required to include this header file
#endif

#include <ac_fixed.h>

// The computation of the K table using double arithmetic
//  limits what practical TE could be chosen.

#define TE 70

#define M_PI 3.14159265358979323846

namespace ac_math
{
   typedef ac_fixed<TE + 1, 0, false, AC_RND, AC_WRAP> table_tATAN2;

   static table_tATAN2 atan_pow2_table[] = {.78539816339744827899949086713604629039764404296875,
                                            .463647609000806093515478778499527834355831146240234375,
                                            .2449786631268641434733268624768243171274662017822265625,
                                            .1243549945467614381566789916178095154464244842529296875,
                                            .06241880999595735002305474381500971503555774688720703125,
                                            .0312398334302682774421544564802388777025043964385986328125,
                                            .01562372862047683129416153491320073953829705715179443359375,
                                            .007812341060101111143987306917324531241320073604583740234375,
                                            .0039062301319669717573901390750279460917226970195770263671875,
                                            .001953122516478818758434155000713872141204774379730224609375,
                                            .0009765621895593194594364927496599193545989692211151123046875,
                                            .00048828121119489828992621394121442790492437779903411865234375,
                                            .000244140620149361771244744812037197334575466811656951904296875,
                                            .00012207031189367020785306594543584424172877334058284759521484375,
                                            .0000610351561742087725935014541622791739428066648542881011962890625,
                                            .00003051757811552609572715473451598455767452833242714405059814453125,
                                            .000015258789061315761542377868187347900175154791213572025299072265625,
                                            .0000076293945311019699810389967098434027548137237317860126495361328125,
                                            .00000381469726560649614175075618194288296081140288151800632476806640625,
                                            .00000190734863281018717653752213292417394541189423762261867523193359375,
                                            .00000095367431640596073824851265643420816786601790226995944976806640625,
                                            .00000047683715820308900109974169223558959629372111521661281585693359375,
                                            .00000023841857910155784131879010134813512422624626196920871734619140625,
                                            .00000011920928955078082648352637284983046583874966017901897430419921875,
                                            .000000059604644775390625,
                                            .0000000298023223876953125,
                                            .00000001490116119384765625,
                                            .000000007450580596923828125,
                                            .0000000037252902984619140625,
                                            .00000000186264514923095703125,
                                            .000000000931322574615478515625,
                                            .0000000004656612873077392578125,
                                            .00000000023283064365386962890625,
                                            .000000000116415321826934814453125,
                                            .0000000000582076609134674072265625,
                                            .00000000002910383045673370361328125,
                                            .000000000014551915228366851806640625,
                                            .0000000000072759576141834259033203125,
                                            .00000000000363797880709171295166015625,
                                            .000000000001818989403545856475830078125,
                                            .0000000000009094947017729282379150390625,
                                            .00000000000045474735088646411895751953125,
                                            .000000000000227373675443232059478759765625,
                                            .0000000000001136868377216160297393798828125,
                                            .00000000000005684341886080801486968994140625,
                                            .000000000000028421709430404007434844970703125,
                                            .0000000000000142108547152020037174224853515625,
                                            .00000000000000710542735760100185871124267578125,
                                            .000000000000003552713678800500929355621337890625,
                                            .0000000000000017763568394002504646778106689453125,
                                            .00000000000000088817841970012523233890533447265625,
                                            .000000000000000444089209850062616169452667236328125,
                                            .0000000000000002220446049250313080847263336181640625,
                                            .00000000000000011102230246251565404236316680908203125,
                                            .000000000000000055511151231257827021181583404541015625,
                                            .0000000000000000277555756156289135105907917022705078125,
                                            .00000000000000001387778780781445675529539585113525390625,
                                            .000000000000000006938893903907228377647697925567626953125,
                                            .0000000000000000034694469519536141888238489627838134765625,
                                            .00000000000000000173472347597680709441192448139190673828125,
                                            .000000000000000000867361737988403547205962240695953369140625,
                                            .0000000000000000004336808689942017736029811203479766845703125,
                                            .00000000000000000021684043449710088680149056017398834228515625,
                                            .000000000000000000108420217248550443400745280086994171142578125,
                                            .0000000000000000000542101086242752217003726400434970855712890625,
                                            .00000000000000000002710505431213761085018632002174854278564453125,
                                            .000000000000000000013552527156068805425093160010874271392822265625,
                                            .0000000000000000000067762635780344027125465800054371356964111328125,
                                            .00000000000000000000338813178901720135627329000271856784820556640625,
                                            .000000000000000000001694065894508600678136645001359283924102783203125};

   static table_tATAN2 atan_2mi(int i)
   {
      if(i >= TE)
      {
         return 0;
      }
      return atan_pow2_table[i];
   }

   //============================================================================
   // Function:  theta = atan2(t)
   // Inputs:
   //   - argument t = y/x range of t = [R]
   // Outputs:
   //   - atan2, inverse tan angle in radians
   //-------------------------------------------------------------------------

   template <int YW, int YI, ac_q_mode YQ, ac_o_mode YO, int XW, int XI, ac_q_mode XQ, ac_o_mode XO, int OW, int OI,
             ac_q_mode OQ, ac_o_mode OO>
   void ac_atan2_cordic(ac_fixed<YW, YI, true, YQ, YO> y, ac_fixed<XW, XI, true, XQ, XO> x,
                        ac_fixed<OW, OI, true, OQ, OO>& atan)
   {
      // Number of iterations depends on output precision OW
      const int N_I = OW - OI + 3;

      // assume maximal N_I of 127
      const int LOG_N_I = (N_I < 16) ? 4 : (N_I < 32) ? 5 : (N_I < 64) ? 6 : 7;
      // Precision for internal computation: n + log(n)
      const int ICW = (N_I + LOG_N_I);

      typedef ac_fixed<ICW + 4, 3, true> fx_a;
      fx_a acc_a = 0;
      bool x_neg = x < 0;

      const int XYI = AC_MAX(YI, XI) + 2;
      const int XYW = ICW + 4 + XYI;
      typedef ac_fixed<XYW, XYI, true> fx_xy;
      typedef ac_fixed<XYW - 1, XYI - 1, false> fx_xy_1;
      typedef ac_fixed<XYW - 2, XYI - 2, false> fx_xy_2;

      fx_xy_1 x1 = x_neg ? (fx_xy_1)-x : (fx_xy_1)x;
      fx_xy y1 = y;

      for(int i = 0; i < N_I; i++)
      {
         ac_fixed<ICW, 0, false> d_a = atan_2mi(i);
         fx_xy_2 x_2mi = x1 >> i; // x1 * pow(2, -i)
         fx_xy y_2mi = y1 >> i;   // y1 * pow(2, -i)
         if(y1 < 0)
         {
            x1 -= y_2mi;
            y1 += x_2mi;
            acc_a -= d_a;
         }
         else
         {
            x1 += y_2mi;
            y1 -= x_2mi;
            acc_a += d_a;
         }
      }
      if(!y)
      {
         acc_a = x_neg ? fx_a(M_PI) : fx_a(0);
      }
      else if(!x)
      {
         acc_a = y < 0 ? fx_a(-M_PI / 2) : fx_a(M_PI / 2);
      }
      else if(x_neg)
      {
         acc_a = (y < 0 ? fx_a(-M_PI) : fx_a(M_PI)) - acc_a;
      }
      atan = acc_a;
   }

} // namespace ac_math

#endif
