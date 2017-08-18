
/*----------------------------------------------------------------------------
| One of the macros `BIGENDIAN' or `LITTLEENDIAN' must be defined.
*----------------------------------------------------------------------------*/
#define LITTLEENDIAN

/*----------------------------------------------------------------------------
| The macro `BITS64' can be defined to indicate that 64-bit integer types are
| supported by the compiler.
*----------------------------------------------------------------------------*/
#define BITS64

/*----------------------------------------------------------------------------
| Each of the following `typedef's defines the most convenient type that holds
| integers of at least as many bits as specified.  For example, `__uint8' should
| be the most convenient type that can hold unsigned integers of as many as
| 8 bits.  The `flagTT' type must be able to hold either a 0 or 1.  For most
| implementations of C, `flagTT', `__uint8', and `__int8' should all be `typedef'ed
| to the same as `int'.
*----------------------------------------------------------------------------*/
typedef _Bool __flag;
typedef unsigned char __uint8;
typedef signed char __int8;
typedef unsigned short int __uint16;
typedef short int __int16;
typedef unsigned int __uint32;
typedef signed int __int32;
#ifdef BITS64
typedef unsigned long long int __uint64;
typedef signed long long int __int64;
#endif

/*----------------------------------------------------------------------------
| Each of the following `typedef's defines a type that holds integers
| of _exactly_ the number of bits specified.  For instance, for most
| implementation of C, `__bits16' and `__sbits16' should be `typedef'ed to
| `unsigned short int' and `signed short int' (or `short int'), respectively.
*----------------------------------------------------------------------------*/
typedef unsigned char __bits8;
typedef signed char __sbits8;
typedef unsigned short int __bits16;
typedef signed short int __sbits16;
typedef unsigned int __bits32;
typedef signed int __sbits32;
#ifdef BITS64
typedef unsigned long long int __bits64;
typedef signed long long int __sbits64;
#endif

#ifdef BITS64
/*----------------------------------------------------------------------------
| The `LIT64' macro takes as its argument a textual integer literal and
| if necessary ``marks'' the literal as having a 64-bit integer type.
| For example, the GNU C Compiler (`gcc') requires that 64-bit literals be
| appended with the letters `LL' standing for `long long', which is `gcc's
| name for the 64-bit integer type.  Some compilers may allow `LIT64' to be
| defined as the identity macro:  `#define LIT64( a ) a'.
*----------------------------------------------------------------------------*/
#define LIT64( a ) a##LL
#endif

/*----------------------------------------------------------------------------
| The macro `INLINE' can be used before functions that should be inlined.  If
| a compiler does not support explicit inlining, this macro should be defined
| to be `static'.
*----------------------------------------------------------------------------*/
#define INLINE extern inline

