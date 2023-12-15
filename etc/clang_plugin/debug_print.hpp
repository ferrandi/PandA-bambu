#ifndef DEBUG_PRINT_HPP
#define DEBUG_PRINT_HPP

#undef LLVM_DEBUG

#ifndef NDEBUG
#define LLVM_DEBUG(X) \
   do                 \
   {                  \
      X;              \
   } while(false)
#else
#define LLVM_DEBUG(X)
#endif

#endif // DEBUG_PRINT_HPP