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
 *              Copyright (C) 2022-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
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
#define STR_cost_latency_table_default                                                                               \
   "store_node|32=10,load_node|32=20,nop_node|32=10,mul_node|1=0.53300000000000003,mul_node|8=2.2559999999999998,"   \
   "mul_node|16=3.778,mul_node|32=5.4339999999999993,mul_node|64=8.2169999999999987,add_node|1=0."                   \
   "53300000000000003,add_node|8=1.002,add_node|16=1.109,add_node|32=1.321,add_node|64=1.7450000000000001,idiv_"     \
   "node|1=0.53300000000000003,idiv_node|8=8.2609999999999992,idiv_node|16=23.864000000000001,idiv_"                 \
   "node|32=55.195,idiv_node|64=153.52799999999999,irem_node|1=0.53300000000000003,irem_node|8=9."                   \
   "0609999999999999,irem_node|16=21.321000000000002,irem_node|32=54.350999999999999,irem_node|64=154."              \
   "405,shl_node|1=0.53300000000000003,shl_node|8=1.0349999999999999,shl_node|16=1.194,shl_node|32=1.621,"           \
   "shl_node|64=1.8580000000000001,shr_node|1=0.53300000000000003,shr_node|8=1.018,shr_node|16=1."                   \
   "3089999999999999,shr_node|32=1.702,shr_node|64=1.9019999999999999,and_node|1=0.53300000000000003,and_"           \
   "node|8=0.496,and_node|16=0.504,and_node|32=0.56899999999999995,and_node|64=0.58299999999999996,"                 \
   "or_node|1=0.53300000000000003,or_node|8=0.496,or_node|16=0.504,or_node|32=0."                                    \
   "56899999999999995,or_node|64=0.58299999999999996,xor_node|1=0.45000000000000001,xor_node|8=0."                   \
   "52800000000000002,xor_node|16=0.59499999999999997,xor_node|32=0.56000000000000005,xor_node|64=0."                \
   "65700000000000003,select_node|1=0.58499999999999996,select_node|8=1.0820000000000001,select_node|16=1."          \
   "1970000000000001,"                                                                                               \
   "select_node|32=1.536,select_node|64=1.7130000000000001,Fmul_node|32=8.5489999999999995,Fmul_node|64=20,Faddsub_" \
   "node|32=20,Faddsub_node|64=30,Ffdiv_node|32=60,Ffdiv_node|64=110"

#endif
