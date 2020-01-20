/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (C) 2004-2020 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/**
 * @file exceptions.hpp
 * @brief exceptions managed by PandA
 *
 * This structure is used to manage the exception raised by the Panda toolset.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

/// Autoheader include
#include "config_HAVE_ASSERTS.hpp"
#include "config_HAVE_PRINT_STACK.hpp"

/// STD include
#include <iostream>
#include <string>
#if HAVE_PRINT_STACK
#include <cxxabi.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#endif

extern int exit_code;

extern bool error_on_warning;

/**
 * Template function used to throw an exception
 * @param t is the expression under check
 * @param  expression is the string representing the source of the object t.
 * @param  pp is the name of the calling function
 * @param  file is the file where throw_error is called
 * @param  line is the line where throw_error is called
 */
template <class T>
inline T& throw_error(const T& t, const std::string& expression,
                      const char*
#ifndef NDEBUG
                          pp
#endif
                      ,
                      const char*
#ifndef NDEBUG
                          file
#endif
                      ,
                      int
#ifndef NDEBUG
                          line
#endif
                      ,
                      int code = EXIT_FAILURE)
{
   exit_code = code;
#ifdef NDEBUG
   throw std::string(std::string("error -> ") + expression + std::string("\n"));
#else
   throw std::string(std::string("error -> ") + expression + std::string("\n\t") + std::string(pp) + std::string("\n\t") + std::string(file) + std::string(":") + std::to_string(line));
#endif
   static T t_local = t;
   return t_local;
}

/**
 * Prints a warning, along with information about the source code
 */
inline void throw_warning(const std::string& expression,
                          const char*
#ifndef NDEBUG
                              pp
#endif
                          ,
                          const char*
#ifndef NDEBUG
                              file
#endif
                          ,
                          int
#ifndef NDEBUG
                              line
#endif
)
{
#ifdef NDEBUG
   throw std::string(std::string("warning -> ") + expression + std::string("\n"));
#else
   throw std::string(std::string("warning -> ") + expression + std::string("\n\t") + std::string(pp) + std::string("\n\t") + std::string(file) + std::string(":") + std::to_string(line));
#endif
}

/**
 * Template function used to throw an exception
 * @param t is the expression under check
 * @param  expression is the string representing the source of the object t.
 * @param  pp is the name of the calling function
 * @param  file is the file where throw_error is called
 * @param  line is the line where throw_error is called
 */
template <class T>
inline T& throw_error(const T& t, const char* expression, const char* pp, const char* file, int line, int code = EXIT_FAILURE)
{
   return throw_error(t, std::string(expression), pp, file, line, code);
}
#if HAVE_PRINT_STACK
#define PRINT_STACK                                                                                                                                          \
   do                                                                                                                                                        \
   {                                                                                                                                                         \
      const size_t print_stack_max_depth = 100;                                                                                                              \
      size_t print_stack_stack_depth;                                                                                                                        \
      void* print_stack_stack_addrs[print_stack_max_depth];                                                                                                  \
      char** print_stack_stack_strings;                                                                                                                      \
                                                                                                                                                             \
      char buf[256];                                                                                                                                         \
      ssize_t readlink_res = readlink("/proc/self/exe", buf, sizeof(buf));                                                                                   \
      if(readlink_res == -1)                                                                                                                                 \
         printf("Failed command: %s\n", "readlink");                                                                                                         \
      buf[readlink_res] = '\0';                                                                                                                              \
      std::string program(buf);                                                                                                                              \
      print_stack_stack_depth = static_cast<size_t>(backtrace(print_stack_stack_addrs, print_stack_max_depth));                                              \
      print_stack_stack_strings = backtrace_symbols(print_stack_stack_addrs, static_cast<int>(print_stack_stack_depth));                                     \
                                                                                                                                                             \
      printf("Call stack\n");                                                                                                                                \
                                                                                                                                                             \
      for(size_t print_stack_i = 0; print_stack_i < print_stack_stack_depth; print_stack_i++)                                                                \
      {                                                                                                                                                      \
         size_t print_stack_sz = 2000; /* just a guess, template names will go much wider*/                                                                  \
         char* print_stack_function = static_cast<char*>(malloc(print_stack_sz));                                                                            \
         char* print_stack_address = static_cast<char*>(malloc(print_stack_sz));                                                                             \
         char *print_stack_begin = nullptr, *print_stack_end = nullptr;                                                                                      \
         size_t print_stack_first = 0, print_stack_last = 0;                                                                                                 \
         size_t print_stack_counter;                                                                                                                         \
         print_stack_counter = 0;                                                                                                                            \
         /* find the parentheses and address offset surrounding the mangled name */                                                                          \
         for(char* print_stack_j = print_stack_stack_strings[print_stack_i]; *print_stack_j; ++print_stack_j, print_stack_counter++)                         \
         {                                                                                                                                                   \
            if(*print_stack_j == '(')                                                                                                                        \
            {                                                                                                                                                \
               print_stack_begin = print_stack_j;                                                                                                            \
            }                                                                                                                                                \
            else if(*print_stack_j == '+')                                                                                                                   \
            {                                                                                                                                                \
               print_stack_end = print_stack_j;                                                                                                              \
            }                                                                                                                                                \
            else if(*print_stack_j == '[')                                                                                                                   \
            {                                                                                                                                                \
               print_stack_first = print_stack_counter;                                                                                                      \
            }                                                                                                                                                \
            else if(*print_stack_j == ']')                                                                                                                   \
            {                                                                                                                                                \
               print_stack_last = print_stack_counter;                                                                                                       \
            }                                                                                                                                                \
         }                                                                                                                                                   \
         if(print_stack_begin && print_stack_end)                                                                                                            \
         {                                                                                                                                                   \
            *print_stack_begin++ = 0;                                                                                                                        \
            *print_stack_end = 0;                                                                                                                            \
            /* found our mangled name, now in [print_stack_begin, print_stack_end)*/                                                                         \
                                                                                                                                                             \
            int print_stack_status;                                                                                                                          \
            char* print_stack_ret = abi::__cxa_demangle(print_stack_begin, print_stack_function, &print_stack_sz, &print_stack_status);                      \
            if(print_stack_ret)                                                                                                                              \
            {                                                                                                                                                \
               /*return value may be a realloc() of the input*/                                                                                              \
               print_stack_function = print_stack_ret;                                                                                                       \
            }                                                                                                                                                \
            else                                                                                                                                             \
            {                                                                                                                                                \
               /* demangling failed, just pretend it's a C function with no args */                                                                          \
               strncpy(print_stack_function, print_stack_begin, print_stack_sz);                                                                             \
               strncat(print_stack_function, "()", print_stack_sz);                                                                                          \
               print_stack_function[print_stack_sz - 1] = 0;                                                                                                 \
            }                                                                                                                                                \
            printf("%s:%s\n", print_stack_stack_strings[print_stack_i], print_stack_function);                                                               \
            if(print_stack_first && print_stack_last)                                                                                                        \
            {                                                                                                                                                \
               std::string print_stack_command;                                                                                                              \
               print_stack_command += "addr2line";                                                                                                           \
               strncpy(print_stack_address, &((print_stack_stack_strings[print_stack_i])[print_stack_first + 1]), print_stack_last - print_stack_first - 1); \
               print_stack_address[print_stack_last - print_stack_first] = '\0';                                                                             \
               print_stack_command += " -C " + std::string(print_stack_address) + " -e " + program;                                                          \
               int print_stack_res = std::system(print_stack_command.c_str());                                                                               \
               if(is_failure(print_stack_res))                                                                                                               \
                  printf("Failed command: %s\n", print_stack_command.c_str());                                                                               \
            }                                                                                                                                                \
         }                                                                                                                                                   \
         else                                                                                                                                                \
         {                                                                                                                                                   \
            /* didn't find the mangled name, just print the whole line*/                                                                                     \
            printf("%s\n", print_stack_stack_strings[print_stack_i]);                                                                                        \
            if(print_stack_first && print_stack_last)                                                                                                        \
            {                                                                                                                                                \
               std::string print_stack_command;                                                                                                              \
               print_stack_command += "addr2line";                                                                                                           \
               strncpy(print_stack_address, &((print_stack_stack_strings[print_stack_i])[print_stack_first + 1]), print_stack_last - print_stack_first - 1); \
               print_stack_address[print_stack_last - print_stack_first] = '\0';                                                                             \
               print_stack_command += " -C " + std::string(print_stack_address) + " -e " + program;                                                          \
               int print_stack_res = std::system(print_stack_command.c_str());                                                                               \
               if(is_failure(print_stack_res))                                                                                                               \
                  printf("Failed command: %s\n", print_stack_command.c_str());                                                                               \
            }                                                                                                                                                \
         }                                                                                                                                                   \
         printf("\n");                                                                                                                                       \
         free(print_stack_function);                                                                                                                         \
         free(print_stack_address);                                                                                                                          \
      }                                                                                                                                                      \
      free(print_stack_stack_strings); /* malloc()ed by backtrace_symbols*/                                                                                  \
   } while(0)
#else
#define PRINT_STACK
#endif

/// helper function used to throw an error in a standard way
#define THROW_ERROR(str_expr) throw_error(0, (str_expr), __PRETTY_FUNCTION__, __FILE__, __LINE__)

/// helper function used to throw an error with a code error
#define THROW_ERROR_CODE(code, str_expr)                                         \
   do                                                                            \
   {                                                                             \
      PRINT_STACK;                                                               \
      throw_error(0, (str_expr), __PRETTY_FUNCTION__, __FILE__, __LINE__, code); \
   } while(0)

/// helper function used to check an assert and if needed to throw an error in a standard way
#if HAVE_ASSERTS
#define THROW_ASSERT(cond, str_expr)                              \
   do                                                             \
   {                                                              \
      if(cond)                                                    \
      {                                                           \
         ;                                                        \
      }                                                           \
      else                                                        \
      {                                                           \
         PRINT_STACK;                                             \
         THROW_ERROR(std::string(str_expr) + " (" + #cond + ")"); \
      }                                                           \
   } while(0)
#else
#define THROW_ASSERT(cond, str_expr)
#endif
/// helper function used to specify that some points should never be reached
#define THROW_UNREACHABLE(str_expr)                                                              \
   do                                                                                            \
   {                                                                                             \
      THROW_ERROR(std::string("This point should never be reached - " + std::string(str_expr))); \
   } while(0)

/// helper function used to throw a warning in a standard way: though it uses PRINT_DBG_MEX,
/// the debug level used is such that the message is always printed
#define THROW_WARNING(str_expr) ((error_on_warning) ? ((void)(THROW_ERROR(str_expr))) : ((void)(std::cerr << std::string("Warning: ") + str_expr << std::endl)))

/// helper function to mark points not yet implemented
#define NOT_YET_IMPLEMENTED() ((error_on_warning) ? ((void)(THROW_ERROR(std::string("Not yet implemented")))) : (throw_warning((std::string("Not yet implemented")), __PRETTY_FUNCTION__, __FILE__, __LINE__)))

/// helper function used to check an assert and if needed to throw an error in a standard way
#if HAVE_ASSERTS
#define THROW_WARNING_ASSERT(cond, str_expr) ((cond) ? (void)0 : (THROW_WARNING(std::string(str_expr) + " (" + #cond + ")")))
#else
#define THROW_WARNING_ASSERT(cond, str_expr) (void)0
#endif

/**
 * Error code returned by THROW_ERROR_CODE
 */
enum throw_error_code
{
   NODE_NOT_YET_SUPPORTED_EC = 2, /**< Node not yet supported*/
   IRREDUCIBLE_LOOPS_EC,          /**< irreducible loops are not currently supported */
   NESTED_FUNCTIONS_EC,           /**< nested functions are not currently supported */
   PRAGMA_FAILURE_EC,             /**< pragma pattern not yet supported*/
   TASK_GRAPH_TRANSFORMATION_EC,  /**< task graph structure not yet supported */
   BOH_EC,                        /**< boh not yet supported */
   FORK_STRUCT,                   /**< malformed struct */
   TASK_CREATION,                 /**< error with task creation */
   PROFILING_EC,                  /**< error during profiling */
   COMPILING_EC,                  /**< error during compilation */
   VLA_EC,                        /**< Not supported variable length array */
   POINTER_PLUS_EC,               /**< pointer_plus_expr not removed */
   VARARGS_EC,                    /**< varargs with cross-compiler */
   BITFIELD_EC,                   /**< bitfield not supported */
   C_EC,                          /**< C pattern not supported */

   /// Not trapped by panda torture script
   GRMON_EC, /**< GRMON serious error (e.g.: locked usb device)*/
   BOARD_EC  /**< Corrupted bitstream */
};

/**
 * Return true if the return_value corresponds to an error
 * @param return_value is a return value of a system
 * @return true if return value corresponds to an error
 */
bool IsError(const int error_value);

/**
 * Return true if the return value corresponds to a failure (not to an error)
 * @param return_value is a return value of a system
 * @return true if return value corresponds to a failure
 */
bool is_failure(const int error_value);

#endif
