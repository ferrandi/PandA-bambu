This is a minimal port of ac\_types (https://github.com/hlslibs/ac_types) to bambu/PANDA.
With respect the original library these are the changes mainly done:
  - Compiler support: This version of the library support both Clang and GCC.
     Clang/llvm is able to lower ac_types to builtin data types when
     optimizations are enable.
     GCC is working well but will not lower the ac_types to builtin when they
     are used as top level function parameter data types.
     To improve the efficiency of hardware generated, some optimizations have
     been done:
     - bit_adjust function is called on any input and output operand of basic
       operators. This helps the (bit) value range analysis to infer the
       minimum number of bits for operands and results.
     - the class iv use specialized classes to store the array of integers. In
       particular, for ac_types requiring less than 32bits a single integer
       object is used. This makes the SROA compiler step more effective.
       Similar optimizations have been done even for larger data types.
     - Inlining is forced almost everywhere. This implies many simplifications
       and an effective hardware generation process.
     - constexpr and some features from c++14 have been used to propagate
       constants at compile time.
   - Library extension: the library has been extended to make it more
     compatible with the ap_types library. In particular, it has been added:
     - the range operator over a slice of bits
     - a constructor from a constant string able to convert such string
       in a double.
     - conversion operators from string to ac_int
     - two headers files ap_int.h and ap_fixed.h for ap_types library 
       compatibility.

