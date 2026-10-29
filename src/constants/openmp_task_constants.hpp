/* SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 * Copyright (C) 2026 Politecnico di Milano
 */
#ifndef OPENMP_TASK_CONSTANTS_HPP
#define OPENMP_TASK_CONSTANTS_HPP

#include "Parameter.hpp"

namespace openmp_task
{
   inline bool IsEnabled(const Parameter& parameters)
   {
      if(!parameters.isOption(OPT_openmp) || !parameters.getOption<bool>(OPT_openmp) ||
         !parameters.IsParameter("enable-task"))
      {
         return false;
      }
      const auto value = parameters.GetParameter<std::string>("enable-task");
      return value == "1" || value == "true";
   }

   inline bool IsGraphEntry(const std::string& name)
   {
      return name == "task_graph";
   }

   constexpr const char* GraphFilename = "task_graph.xml";
} // namespace openmp_task

#endif
