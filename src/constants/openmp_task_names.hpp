/* SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 * Copyright (C) 2026 Politecnico di Milano
 */
#ifndef OPENMP_TASK_NAMES_HPP
#define OPENMP_TASK_NAMES_HPP
#include <string>
namespace openmp_task
{
   constexpr const char* GraphEntryName = "task_graph";
   constexpr const char* GraphFilename = "task_graph.xml";
   constexpr const char* DetectionFilename = "openmp-task-detected.txt";
   inline bool IsGraphEntry(const std::string& name)
   {
      return name == GraphEntryName;
   }

   // Sanitized by the LLVM frontend (.omp_task_entry. -> omp_task_entry).
   // Only the reserved prefix followed by generated numeric/suffix tokens is valid.
   inline bool IsTaskEntry(const std::string& name)
   {
      const std::string prefix = "omp_task_entry_";
      const std::string internalPrefix = "_" + prefix;
      const auto start = name.compare(0, prefix.size(), prefix) == 0                 ? prefix.size() :
                         name.compare(0, internalPrefix.size(), internalPrefix) == 0 ? internalPrefix.size() :
                                                                                       0U;
      if(!start)
      {
         return false;
      }
      auto position = name.find("transformed", start);
      if(position == std::string::npos)
      {
         return false;
      }
      for(auto i = start; i < position; ++i)
      {
         if(name[i] != '_' && (name[i] < '0' || name[i] > '9'))
         {
            return false;
         }
      }
      position += std::string("transformed").size();
      for(; position < name.size(); ++position)
      {
         if(name[position] != '_' && (name[position] < '0' || name[position] > '9'))
         {
            return false;
         }
      }
      return true;
   }

   inline bool IsRuntimeFunction(const std::string& name)
   {
      return name.compare(0, 7, "__kmpc_") == 0 || name.compare(0, 6, "__kmp_") == 0;
   }

   inline bool IsFrontendTaskEntry(const std::string& name)
   {
      return name.compare(0, std::string(".omp_task_entry.").size(), ".omp_task_entry.") == 0;
   }

   inline bool IsOutlinedFunction(const std::string& name)
   {
      const std::string marker = ".omp_outlined";
      const auto position = name.find(marker);
      if(position == std::string::npos)
      {
         return false;
      }
      const auto suffix = name.substr(position + marker.size());
      return suffix.find_first_not_of(".0123456789") == std::string::npos;
   }
} // namespace openmp_task
#endif
