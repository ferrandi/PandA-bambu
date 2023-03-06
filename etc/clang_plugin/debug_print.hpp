#ifndef DEBUG_PRINT_HPP
#define DEBUG_PRINT_HPP

#ifndef NDEBUG
#define PRINT_DBG(stuff) llvm::errs() << stuff
#define PRINT_DBG_VAR(stuff, var)  \
   PRINT_DBG(stuff);               \
   var->print(llvm::errs(), true); \
   PRINT_DBG("\n")
#else
#define PRINT_DBG(stuff)
#define PRINT_DBG_VAR(stuff, var)
#endif

#endif // DEBUG_PRINT_HPP