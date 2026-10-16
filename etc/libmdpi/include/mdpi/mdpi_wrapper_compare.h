// Copyright (C) 2023-2026 Politecnico di Milano
//
// Part of the PandA/Bambu MDPI Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//

#ifndef __MDPI_WRAPPER_COMPARE_H
#define __MDPI_WRAPPER_COMPARE_H

#include "mdpi_debug.h"
#include "mdpi_user.h"

#include <stdlib.h>
#include <string.h>

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

#define _ms_argcmp(suffix, idx, cmp)                                                                                   \
   do                                                                                                                  \
   {                                                                                                                   \
      const size_t elm_count = P##idx##_size_##suffix / sizeof(m_getvalt(P##idx));                                     \
      for(i = 0; i < elm_count; ++i)                                                                                   \
      {                                                                                                                \
         if(m_cmp##cmp((m_getptrt(P##idx))P##idx##_##suffix + i, (m_getptrt(P##idx))m_getptr(P##idx) + i))             \
         {                                                                                                             \
            error("Memory parameter %u (%zu/%zu) mismatch with respect to " #suffix " reference.\n", idx, i + 1,       \
                  elm_count);                                                                                          \
            ++mismatch_count;                                                                                          \
         }                                                                                                             \
      }                                                                                                                \
      const size_t tail_offset = elm_count * sizeof(m_getvalt(P##idx));                                                \
      const size_t tail_size = P##idx##_size_##suffix - tail_offset;                                                   \
      if(tail_size &&                                                                                                  \
         memcmp((const char*)P##idx##_##suffix + tail_offset, (const char*)m_getptr(P##idx) + tail_offset, tail_size)) \
      {                                                                                                                \
         error("Memory parameter %u trailing bytes mismatch with respect to " #suffix " reference.\n", idx);           \
         ++mismatch_count;                                                                                             \
      }                                                                                                                \
      _m_argdump(suffix, P##idx##_##suffix, idx);                                                                      \
      free(P##idx##_##suffix);                                                                                         \
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

#endif // __MDPI_WRAPPER_COMPARE_H
