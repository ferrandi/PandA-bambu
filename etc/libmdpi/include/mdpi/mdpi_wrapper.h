//    Copyright (C) 2023-2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//    This file is part of the PandA/Bambu MDPI Library.
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
#ifndef __MDPI_WRAPPER_H
#define __MDPI_WRAPPER_H

#define __BAMBU_IPC_ENTITY MDPI_ENTITY_DRIVER
#include "mdpi_debug.h"
#include "mdpi_driver.h"
#include "mdpi_user.h"

#define typeof __typeof__
#ifdef __cplusplus
template <typename T>
struct __m_type
{
   typedef T type;
};
template <typename T>
struct __m_type<T*>
{
   typedef typename __m_type<T>::type type;
};
template <>
struct __m_type<void*>
{
   typedef typename __m_type<unsigned char>::type type;
};
template <>
struct __m_type<const void*>
{
   typedef typename __m_type<const unsigned char>::type type;
};
#define m_getptrt(val) __m_type<typeof(val)>::type*
#define m_getvalt(val) __m_type<typeof(val)>::type
template <typename T>
T* m_getptr(T& obj)
{
   return &obj;
}
template <typename T>
T* m_getptr(T* obj)
{
   return obj;
}
#define __m_float_distance(a, b) m_float_distance(a, b)
#else
#define m_getptrt(val) typeof(val)
#define m_getvalt(val) typeof(*val)
#define m_getptr(ptr) (ptr)
#define __m_float_distance(a, b) \
   ((typeof(a) (*)(typeof(a), typeof(a)))((sizeof(a) == sizeof(float)) ? m_float_distancef : m_float_distance))(a, b)
#define __m_floats_distance(a, b) \
   ((typeof(a) (*)(typeof(a), typeof(a)))((sizeof(a) == sizeof(float)) ? m_floats_distancef : m_floats_distance))(a, b)
#endif

#define m_cmpval(ptra, ptrb) *(ptra) != *(ptrb)
#define m_cmpmem(ptra, ptrb) memcmp(ptra, ptrb, sizeof(m_getvalt(ptrb)))
#define m_cmpflt(ptra, ptrb) __m_float_distance(*(ptra), *(ptrb)) > max_ulp
#define m_cmpflts(ptra, ptrb) __m_floats_distance(*(ptra), *(ptrb)) > max_ulp

#define _ms_setargptr(suffix, idx, ptr)                       \
   const size_t P##idx##_size_##suffix = __m_param_size(idx); \
   void* P##idx##_##suffix = malloc(P##idx##_size_##suffix);  \
   memcpy(P##idx##_##suffix, ptr, P##idx##_size_##suffix)

#define _ms_argdump(suffix, param, idx)                                                                          \
   do                                                                                                            \
   {                                                                                                             \
      char filename[32];                                                                                         \
      sprintf(filename, "P" #idx "." #suffix ".%zu.dat", __m_call_count);                                        \
      FILE* out = fopen(filename, "wb");                                                                         \
      if(out != NULL)                                                                                            \
      {                                                                                                          \
         fwrite(m_getptr(param), 1, __m_param_size(idx), out);                                                   \
         fclose(out);                                                                                            \
         debug("Parameter " #idx " " #suffix " output dump for execution %zu stored in '%s'.\n", __m_call_count, \
               filename);                                                                                        \
      }                                                                                                          \
      else                                                                                                       \
      {                                                                                                          \
         error("Unable to open parameter dump file '%s'.\n", filename);                                          \
      }                                                                                                          \
   } while(0)

#ifdef BAMBU_SIM_DUMP_OUTPUT
static size_t __m_call_count = 0;
#define m_call_next(...) ++__m_call_count
#define _m_argdump(suffix, param, idx) _ms_argdump(suffix, param, idx)
#else
#define m_call_next(...)
#define _m_argdump(...)
#endif

#define _ms_argcmp(suffix, idx, cmp)                                                                             \
   do                                                                                                            \
   {                                                                                                             \
      const size_t elm_count = P##idx##_size_##suffix / sizeof(m_getvalt(P##idx));                               \
      for(i = 0; i < elm_count; ++i)                                                                             \
      {                                                                                                          \
         if(m_cmp##cmp((m_getptrt(P##idx))P##idx##_##suffix + i, (m_getptrt(P##idx))m_getptr(P##idx) + i))       \
         {                                                                                                       \
            error("Memory parameter %u (%zu/%zu) mismatch with respect to " #suffix " reference.\n", idx, i + 1, \
                  elm_count);                                                                                    \
            ++mismatch_count;                                                                                    \
         }                                                                                                       \
      }                                                                                                          \
      _m_argdump(suffix, P##idx##_##suffix, idx);                                                                \
      free(P##idx##_##suffix);                                                                                   \
   } while(0)

#define _ms_setargchannel(suffix, idx) m_getvalt(P##idx) P##idx##_##suffix = *m_getptr(P##idx)

#define _ms_channelcmp(suffix, idx, cmp)                                                                          \
   if(m_getptr(P##idx)->size() != m_getptr(P##idx##_##suffix)->size())                                            \
   {                                                                                                              \
      error("Channel parameter %u size mismatch with respect to " #suffix " reference: %zu != %zu.\n", idx,       \
            m_getptr(P##idx)->size(), m_getptr(P##idx##_##suffix)->size());                                       \
      ++mismatch_count;                                                                                           \
   }                                                                                                              \
   else                                                                                                           \
   {                                                                                                              \
      for(i = 0; i < m_getptr(P##idx)->size(); ++i)                                                               \
      {                                                                                                           \
         if(m_cmp##cmp(&m_getptr(P##idx)->operator[](i), &m_getptr(P##idx##_##suffix)->operator[](i)))            \
         {                                                                                                        \
            error("Channel parameter %u (%zu/%zu) mismatch with respect to " #suffix " reference.\n", idx, i + 1, \
                  m_getptr(P##idx)->size());                                                                      \
            ++mismatch_count;                                                                                     \
         }                                                                                                        \
      }                                                                                                           \
   }

#define _ms_retvalcmp(suffix, cmp)                                             \
   if(m_cmp##cmp(&retval, &retval_##suffix))                                   \
   {                                                                           \
      error("Return value mismatch with respect to " #suffix " reference.\n"); \
      ++mismatch_count;                                                        \
   }

#ifdef BAMBU_SKIP_VERIFICATION
#define _m_setargptr(...)
#define _m_argcmp(...)
#define _m_setargchannel(...)
#define _m_channelcmp(...)
#define _m_retvalcmp(...)
#else
#define _m_setargptr(idx, ptr) _ms_setargptr(gold, idx, ptr)
#define _m_argcmp(idx, cmp) _ms_argcmp(gold, idx, cmp)
#define _m_setargchannel(idx) _ms_setargchannel(gold, idx)
#define _m_channelcmp(idx, cmp) _ms_channelcmp(gold, idx, cmp)
#define _m_retvalcmp(cmp) _ms_retvalcmp(gold, cmp)
#endif

#if defined(PP_VERIFICATION) && !defined(BAMBU_CSIM)
#define _m_pp_setargptr(idx, ptr) _ms_setargptr(pp, idx, ptr)
#define _m_pp_argcmp(idx, cmp) _ms_argcmp(pp, idx, cmp)
#define _m_pp_retvalcmp(cmp) _ms_retvalcmp(pp, cmp)
#else
#define _m_pp_setargptr(...)
#define _m_pp_argcmp(...)
#define _m_pp_retvalcmp(...)
#endif

#define m_map_default(ptr) NULL
#define m_interface_default(idx, ptr, bitsize, align) \
   __m_interface_port(ptr, bitsize, align);             \
   _m_pp_setargptr(idx, ptr);                           \
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
#define m_interface_channel(idx, ptr, bitsize, align)             \
   __m_interface_channel(*m_getptr(P##idx), __m_param_size(idx)); \
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
