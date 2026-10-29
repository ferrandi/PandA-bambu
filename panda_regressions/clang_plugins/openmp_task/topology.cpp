/* SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 * Copyright (C) 2026 Politecnico di Milano
 */
#include "TopoGraphGenerator.hpp"

int main(int argc, char**)
{
   using namespace Bambu::OpenMP::Task;
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
