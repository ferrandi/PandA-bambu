// Copyright (C) 2020-2026 Politecnico di Milano
//
// Part of the PandA/Bambu OpenMP Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//

#ifndef KMP_BAMBU_NAMES_H
#define KMP_BAMBU_NAMES_H

#define KMP_GET_TID_FROM_GTID __kmp_bambu_tid_from_gtid
#define KMP_CS_GET_GTID __kmp_bambu_cs_get_gtid
#define KMP_CS_GET_TID __kmp_bambu_cs_get_tid
#define KMP_SET_REDUCE_DATA __kmp_bambu_set_th_local_reduce_data
#define KMP_GET_REDUCE_DATA __kmp_bambu_get_th_local_reduce_data
#define KMP_BARRIER_REACHED __kmp_bambu_barrier_reached
#define KMP_WAIT_ALL_THREADS __kmp_bambu_wait_all_threads
#define KMP_CRITICAL __kmp_bambu_critical
#define KMP_END_CRITICAL __kmp_bambu_end_critical
#define KMP_T_NPROC __kmp_bambu_t_nproc
#define KMP_TH_SET_NPROC __kmp_bambu_th_set_nproc
#define KMP_FORK_CALL __kmp_bambu_fork_call

#endif