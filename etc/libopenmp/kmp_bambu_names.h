//    Copyright (C) 2020-2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//    This file is part of the PandA/Bambu OpenMP Library.
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//
// Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
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