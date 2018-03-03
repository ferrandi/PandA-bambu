#include <stdlib.h>
#define MAX_VERTEX_NUMBER 2
#define MAX_EDGE_NUMBER 2

typedef unsigned int NodeId;
typedef unsigned int EdgeId;
typedef unsigned int PropertyId;

typedef struct Edge {
  NodeId node;
  PropertyId property;
} Edge;

typedef struct Graph {
  size_t numVertices;
//  PropertyId * VertexPropertyVector;
  EdgeId outEdgesIDs[MAX_VERTEX_NUMBER];
  EdgeId inEdgesIDs[MAX_VERTEX_NUMBER];

  Edge inEdges[MAX_EDGE_NUMBER];
  Edge outEdges[MAX_EDGE_NUMBER];
} Graph;

int sum(int a, Graph * graph, int c)
{
   return a + graph->numVertices + graph->outEdgesIDs[0] + graph->outEdgesIDs[1] + graph->inEdgesIDs[0] + graph->inEdgesIDs[1] + graph->inEdges[0].node + graph->inEdges[0].property + graph->inEdges[1].node + graph->inEdges[1].property + graph->outEdges[0].node + graph->outEdges[0].property + graph->outEdges[1].node + graph->outEdges[1].property + c;
}
