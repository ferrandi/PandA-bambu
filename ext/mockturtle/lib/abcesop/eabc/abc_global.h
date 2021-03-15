/**CFile****************************************************************

  FileName    [abc_global.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Global declarations.]

  Synopsis    [Global declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Jan 30, 2009.]

  Revision    [$Id: abc_global.h,v 1.00 2009/01/30 00:00:00 alanmi Exp $]

***********************************************************************/

#pragma once

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#ifndef __MINGW32__
//#define inline __inline // compatible with MS VS 6.0
#pragma warning(disable : 4152) // warning C4152: nonstandard extension, function/data pointer conversion in expression
#pragma warning(disable : 4200) // warning C4200: nonstandard extension used : zero-sized array in struct/union
#pragma warning(disable : 4244) // warning C4244: '+=' : conversion from 'int ' to 'unsigned short ', possible loss of data
#pragma warning(disable : 4514) // warning C4514: 'Vec_StrPop' : unreferenced inline function has been removed
#pragma warning(disable : 4710) // warning C4710: function 'Vec_PtrGrow' not inlined
//#pragma warning( disable : 4273 )
#endif
#endif

#ifdef WIN32
  #ifdef WIN32_NO_DLL
    #define ABC_DLLEXPORT
    #define ABC_DLLIMPORT
  #else
    #define ABC_DLLEXPORT __declspec(dllexport)
    #define ABC_DLLIMPORT __declspec(dllimport)
  #endif
#else  /* defined(WIN32) */
#define ABC_DLLIMPORT
#endif /* defined(WIN32) */

#ifndef ABC_DLL
#define ABC_DLL ABC_DLLIMPORT
#endif

#if !defined(___unused)
#if defined(__GNUC__)
#define ___unused __attribute__ ((__unused__))
#else
#define ___unused
#endif
#endif

#include <time.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

namespace abc::exorcism {

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

/**
 * Signed integral type that can contain a pointer.
 * This is a signed integral type that is the same size as a pointer.
 * NOTE: This type may be different sizes on different platforms.
 */
#if       defined(__ccdoc__)
typedef platform_dependent_type ABC_PTRINT_T;
#elif     defined(ABC_USE_STDINT_H)
typedef intptr_t ABC_PTRINT_T;
#elif     defined(LIN64)
typedef long ABC_PTRINT_T;
#elif     defined(NT64)
typedef long long ABC_PTRINT_T;
#elif     defined(NT) || defined(LIN) || defined(WIN32)
typedef int ABC_PTRINT_T;
#else
   #error unknown platform
#endif /* defined(PLATFORM) */

/**
 * 64-bit signed integral type.
 */
#if       defined(__ccdoc__)
typedef platform_dependent_type ABC_INT64_T;
#elif     defined(ABC_USE_STDINT_H)
typedef int64_t ABC_INT64_T;
#elif     defined(LIN64)
typedef long ABC_INT64_T;
#elif     defined(NT64) || defined(LIN)
typedef long long ABC_INT64_T;
#elif     defined(WIN32) || defined(NT)
typedef signed __int64 ABC_INT64_T;
#else
   #error unknown platform
#endif /* defined(PLATFORM) */


////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#define ABC_SWAP(Type, a, b)  { Type t = a; a = b; b = t; }

#define ABC_ALLOC(type, num)     ((type *) malloc(sizeof(type) * (size_t)(num)))
#define ABC_CALLOC(type, num)    ((type *) calloc((size_t)(num), sizeof(type)))
#define ABC_FALLOC(type, num)    ((type *) memset(malloc(sizeof(type) * (size_t)(num)), 0xff, sizeof(type) * (size_t)(num)))
#define ABC_FREE(obj)            ((obj) ? (free((char *) (obj)), (obj) = 0) : 0)
#define ABC_REALLOC(type, obj, num) \
        ((obj) ? ((type *) realloc((char *)(obj), sizeof(type) * (size_t)(num))) : \
         ((type *) malloc(sizeof(type) * (size_t)(num))))

static inline int      Abc_MaxInt( int a, int b )             { return a > b ?  a : b; }
static inline int      Abc_Base2Log( unsigned n )             { int r; if ( n < 2 ) return (int)n; for ( r = 0, n--; n; n >>= 1, r++ ) {}; return r; }
static inline char *   Abc_UtilStrsav( char * s )             { return s ? strcpy(ABC_ALLOC(char, strlen(s)+1), s) : NULL;  }
static inline int      Abc_Lit2Var( int Lit )                 { assert(Lit >= 0); return Lit >> 1;                          }
static inline int      Abc_LitIsCompl( int Lit )              { assert(Lit >= 0); return Lit & 1;                           }

// time counting
typedef ABC_INT64_T abctime;
static inline abctime Abc_Clock()
{
#if (defined(LIN) || defined(LIN64)) && !(__APPLE__ & __MACH__) && !defined(__MINGW32__)
    struct timespec ts;
    if ( clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) < 0 ) 
        return (abctime)-1;
    abctime res = ((abctime) ts.tv_sec) * CLOCKS_PER_SEC;
    res += (((abctime) ts.tv_nsec) * CLOCKS_PER_SEC) / 1000000000;
    return res;
#else
    return (abctime) clock();
#endif
}

}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
