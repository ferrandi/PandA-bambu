#include <stdlib.h>
#include "graph.h"

int sum(int a, Graph * graph, int c)
{
   return a + graph->numVertices + graph->outEdgesIDs[0] + graph->outEdgesIDs[1] + graph->inEdgesIDs[0] + graph->inEdgesIDs[1] + graph->inEdges[0].node + graph->inEdges[0].property + graph->inEdges[1].node + graph->inEdges[1].property + graph->outEdges[0].node + graph->outEdges[0].property + graph->outEdges[1].node + graph->outEdges[1].property + c;
}
