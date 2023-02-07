/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (C) 2022-2023 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/**
 * @file cost_latency_table.hpp
 * @brief default table used by THR LLVM optimization step.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef CONST_LATENCY_TABLE_HPP
#define CONST_LATENCY_TABLE_HPP

/// default string for latencies
#define STR_cost_latency_table_default                                                                                 \
   "store_expr|32=10,load_expr|32=20,nop_expr|32=10,mult_expr|1=0.53300000000000003,mult_expr|8=2.2559999999999998,"   \
   "mult_expr|16=3.778,mult_expr|32=5.4339999999999993,mult_expr|64=8.2169999999999987,plus_expr|1=0."                 \
   "53300000000000003,plus_expr|8=1.002,plus_expr|16=1.109,plus_expr|32=1.321,plus_expr|64=1.7450000000000001,trunc_"  \
   "div_expr|1=0.53300000000000003,trunc_div_expr|8=8.2609999999999992,trunc_div_expr|16=23.864000000000001,trunc_"    \
   "div_expr|32=55.195,trunc_div_expr|64=153.52799999999999,trunc_mod_expr|1=0.53300000000000003,trunc_mod_expr|8=9."  \
   "0609999999999999,trunc_mod_expr|16=21.321000000000002,trunc_mod_expr|32=54.350999999999999,trunc_mod_expr|64=154." \
   "405,lshift_expr|1=0.53300000000000003,lshift_expr|8=1.0349999999999999,lshift_expr|16=1.194,lshift_expr|32=1.621," \
   "lshift_expr|64=1.8580000000000001,rshift_expr|1=0.53300000000000003,rshift_expr|8=1.018,rshift_expr|16=1."         \
   "3089999999999999,rshift_expr|32=1.702,rshift_expr|64=1.9019999999999999,bit_and_expr|1=0.53300000000000003,bit_"   \
   "and_expr|8=0.496,bit_and_expr|16=0.504,bit_and_expr|32=0.56899999999999995,bit_and_expr|64=0.58299999999999996,"   \
   "bit_ior_expr|1=0.53300000000000003,bit_ior_expr|8=0.496,bit_ior_expr|16=0.504,bit_ior_expr|32=0."                  \
   "56899999999999995,bit_ior_expr|64=0.58299999999999996,bit_xor_expr|1=0.45000000000000001,bit_xor_expr|8=0."        \
   "52800000000000002,bit_xor_expr|16=0.59499999999999997,bit_xor_expr|32=0.56000000000000005,bit_xor_expr|64=0."      \
   "65700000000000003,cond_expr|1=0.58499999999999996,cond_expr|8=1.0820000000000001,cond_expr|16=1.1970000000000001," \
   "cond_expr|32=1.536,cond_expr|64=1.7130000000000001,Fmult_expr|32=8.5489999999999995,Fmult_expr|64=20,Fplus_expr|"  \
   "32=20,Fplus_expr|64=30,Frdiv_expr|32=60,Frdiv_expr|64=110"

#endif
