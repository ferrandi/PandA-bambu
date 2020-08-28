/*===============================================================*/
/*                                                               */
/*                         digitrec_sw.h                         */
/*                                                               */
/*              Software version for digit recognition           */
/*                                                               */
/*===============================================================*/

#ifndef __DIGITREC_SW_H__
#define __DIGITREC_SW_H__

#include "typedefs.h"

void DigitRec_sw(const DigitType global_training_set[NUM_TRAINING * DIGIT_WIDTH], 
                 const DigitType global_test_set[NUM_TEST * DIGIT_WIDTH], 
                 LabelType global_results[NUM_TEST]);

#endif
