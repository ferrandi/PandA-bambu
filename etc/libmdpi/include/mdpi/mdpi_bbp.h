// Copyright (C) 2025-2026 Politecnico di Milano
//
// Part of the PandA/Bambu MDPI Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//

#ifndef __MDPI_BBP_H
#define __MDPI_BBP_H

struct __bambu_csim_bb_stats
{
   unsigned long long exec;
   const unsigned csteps;
};

struct __bambu_csim_function_stats
{
   unsigned id;
   struct __bambu_csim_bb_stats* bb_exec;
   unsigned bb_exec_size;
   unsigned long long last_cycle;
};

struct __bambu_csim_design_stats
{
   unsigned run_id;
   struct __bambu_csim_function_stats* functions;
   unsigned functions_size;
};

void __bambu_csim_design_stats_trace(struct __bambu_csim_function_stats* _s, unsigned bbi, int csteps);
void __bambu_csim_design_stats_rotate(struct __bambu_csim_design_stats* _s, const char* _prof_filename);

struct __bambu_csim_function_stats* __bambu_csim_get_stats(struct __bambu_csim_design_stats* _s, unsigned fid);
void __bambu_csim_module_push(struct __bambu_csim_function_stats* called, int cstep_fix);
void __bambu_csim_module_pop(unsigned long count);

#endif // __MDPI_BBP_H
