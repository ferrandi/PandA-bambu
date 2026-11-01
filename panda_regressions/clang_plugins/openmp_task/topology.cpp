/* SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 * Copyright (C) 2026 Politecnico di Milano
 */
#include "TopoGraphGenerator.hpp"
#include "openmp_task_names.hpp"

int main(int argc, char**)
{
   using namespace Bambu::OpenMP::Task;
   if(!openmp_task::IsGraphEntry("task_graph") || openmp_task::IsGraphEntry("build_task_graph") ||
      !openmp_task::IsTaskEntry("_omp_task_entry_3_transformed_2") ||
      openmp_task::IsTaskEntry("my_omp_task_entry_helper") ||
      openmp_task::IsTaskEntry("_omp_task_entry_helper_transformed") ||
      !openmp_task::IsFrontendTaskEntry(".omp_task_entry..3") ||
      openmp_task::IsFrontendTaskEntry("user.omp_task_entry..3") ||
      !openmp_task::IsOutlinedFunction("_Z2fnPi.omp_outlined") ||
      openmp_task::IsOutlinedFunction("user.omp_outlined_helper"))
   {
      return 1;
   }
   AdjMap edges;
   edges["0"].insert("1");
   if(argc > 1)
   {
      edges["1"].insert("0");
   }
   TopoGraphGenerator graph;
   graph.setGraphParams(edges, Nodes{"0", "1"}, {}, {}, {});
   graph.buildTopo();
   graph.buildTopo();
   return 0;
}
