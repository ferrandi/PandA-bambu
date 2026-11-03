/* SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 * Copyright (C) 2026 Politecnico di Milano
 */
#ifndef OPENMP_TASK_CONSTANTS_HPP
#define OPENMP_TASK_CONSTANTS_HPP

#include "Parameter.hpp"
#include "openmp_task_names.hpp"

namespace openmp_task
{
   inline bool IsEnabled(const Parameter& parameters)
   {
      return parameters.HasOpenMPTaskGraph();
   }

} // namespace openmp_task

#endif
