/* 
 * File:   black_scholes.h
 * Author: Yolanda
 *
 * Created on May 15, 2013, 2:41 PM
 */


#ifndef BLACK_SCHOLES_H
#define	BLACK_SCHOLES_H

# include "fixedptc.h"

fixedpt asset_path_fixed_simplified ( fixedpt s0, fixedpt mu, fixedpt sigma, fixedpt t1, int n, int *seed);
void get_two_normal_fixed(int *seed, fixedpt *n1, fixedpt *n2);
fixedpt get_uniform_fixed ( int *seed );



#endif	/* BLACK_SCHOLES_H */

