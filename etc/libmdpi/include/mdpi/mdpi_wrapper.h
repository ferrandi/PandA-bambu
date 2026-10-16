// Copyright (C) 2023-2026 Politecnico di Milano
//
// Part of the PandA/Bambu MDPI Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//

#ifndef __MDPI_WRAPPER_H
#define __MDPI_WRAPPER_H

#define __BAMBU_IPC_ENTITY MDPI_ENTITY_DRIVER
#include "mdpi_debug.h"
#include "mdpi_driver.h"
#include "mdpi_user.h"

#include "mdpi_wrapper_compare.h"

#define m_map_default(ptr) NULL
#define m_interface_default(idx, ptr, bitsize, align) \
   __m_interface_port(ptr, bitsize, align);           \
   _m_pp_setargptr(idx, ptr);                         \
   _m_setargptr(idx, ptr)

#define m_interface_banked(idx, ptr, bitsize, align) \
   __m_interface_banked(idx, ptr, bitsize, align);   \
   _m_pp_setargptr(idx, ptr);                        \
   _m_setargptr(idx, ptr)

#define __m_interface_banked(idx, ptr, bitsize, align)                                                         \
   ptr_t __bankptrval_##idx = (ptr_t)ptr;                                                                      \
   get_bank_map_internal_addr((char*)ptr, &__bankptrval_##idx);                                                \
   info("Pointer parameter " BPTR_FORMAT " mapped at " PTR_FORMAT "\n", bptr_to_int(ptr), __bankptrval_##idx); \
   __m_interface_port(reinterpret_cast<bptr_t>(&__bankptrval_##idx), bitsize)

#define m_map_ptr(ptr) (void*)ptr
#define m_interface_ptr(idx, ptr, bitsize, align)                               \
   bptr_t __ptrval_##idx = (bptr_t)ptr;                                         \
   __m_interface_ptr(&__ptrval_##idx, sizeof(bptr_t) * 8, __m_param_size(idx)); \
   _m_pp_setargptr(idx, ptr);                                                   \
   _m_setargptr(idx, ptr)

#define m_map_array(...) m_map_default(__VA_ARGS__)
#define m_interface_array(idx, ptr, bitsize, align)                       \
   __m_interface_array(ptr, bitsize, align, __m_param_size(idx) / align); \
   _m_pp_setargptr(idx, ptr);                                             \
   _m_setargptr(idx, ptr)

#define m_map_array_csroa(...) m_map_default(__VA_ARGS__)
#define m_interface_array_csroa(idx, ptr, bitsize, align, part_desc, dim_sizes, dim_count)                       \
   __m_interface_array_csroa(ptr, bitsize, align, __m_param_size(idx) / align, part_desc, dim_sizes, dim_count); \
   _m_pp_setargptr(idx, ptr);                                                                                    \
   _m_setargptr(idx, ptr)

#define m_map_fifo(...) m_map_default(__VA_ARGS__)
#define m_interface_fifo(idx, ptr, bitsize, align)                       \
   __m_interface_fifo(ptr, bitsize, align, __m_param_size(idx) / align); \
   _m_pp_setargptr(idx, ptr);                                            \
   _m_setargptr(idx, ptr)

#define m_map_channel(ptr) NULL
#define m_interface_channel(idx, ptr, bitsize, align, depth) \
   __m_interface_channel(*m_getptr(P##idx), depth);          \
   _m_setargchannel(idx)

#define m_map_none(...) m_map_default(__VA_ARGS__)
#define m_interface_none(...) m_interface_default(__VA_ARGS__)

#define m_map_valid(...) m_map_default(__VA_ARGS__)
#define m_interface_valid(...) m_interface_default(__VA_ARGS__)

#define m_map_ovalid(...) m_map_default(__VA_ARGS__)
#define m_interface_ovalid(...) m_interface_default(__VA_ARGS__)

#define m_map_acknowledge(...) m_map_default(__VA_ARGS__)
#define m_interface_acknowledge(...) m_interface_default(__VA_ARGS__)

#define m_map_handshake(...) m_map_default(__VA_ARGS__)
#define m_interface_handshake(...) m_interface_default(__VA_ARGS__)

#define m_map_axis(...) m_map_fifo(__VA_ARGS__)
#define m_interface_axis(...) m_interface_fifo(__VA_ARGS__)

#define m_map_m_axi(...) m_map_ptr(__VA_ARGS__)
#define m_interface_m_axi(...) m_interface_ptr(__VA_ARGS__)

#define m_interface_bank_offset(ptr) __m_interface_port(ptr, sizeof(*ptr) * 8)

#define m_argcmp(idx, cmp)       \
   _m_argdump(sim, P##idx, idx); \
   _m_pp_argcmp(idx, cmp);       \
   _m_argcmp(idx, cmp)

#define m_channelcmp(idx, cmp) _m_channelcmp(idx, cmp)

#define m_retvalcmp(cmp) _m_pp_retvalcmp(cmp) _m_retvalcmp(cmp)

#endif // __MDPI_WRAPPER_H
