#ifndef DEBUG_PRINT_HPP
#define DEBUG_PRINT_HPP

#ifdef PRINT_DBG_MSG
#define PRINT_DBG(stuff) llvm::errs() << stuff
#else
#define PRINT_DBG(stuff)
#endif

#endif // DEBUG_PRINT_HPP