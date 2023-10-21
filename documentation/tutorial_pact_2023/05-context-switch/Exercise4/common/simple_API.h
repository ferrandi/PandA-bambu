#ifndef __GRAPH_H__
#define __GRAPH_H__

#include <stddef.h>

#ifndef N_THREADS
#define N_THREADS 8
#endif


typedef unsigned int NodeId;
typedef unsigned int EdgeId;
typedef unsigned int PropertyId;


typedef struct Edge {
  NodeId node;
  PropertyId property;
} Edge;

#ifndef MAX_VERTEX_NUMBER
#define MAX_VERTEX_NUMBER 1309073
#endif

#ifndef MAX_EDGE_NUMBER
#define MAX_EDGE_NUMBER 5309056
#endif

typedef struct Graph {
  size_t numVertices;
//  PropertyId * VertexPropertyVector;
  EdgeId outEdgesIDs[MAX_VERTEX_NUMBER];
  EdgeId inEdgesIDs[MAX_VERTEX_NUMBER];

  Edge inEdges[MAX_EDGE_NUMBER];
  Edge outEdges[MAX_EDGE_NUMBER];
} Graph;


//===----------------------------------------------------------------------===//
//                          Backward Star Interface
//===----------------------------------------------------------------------===//

//size_t getInDegree(Graph * graph, NodeId node);
static inline size_t getInDegree(Graph * graph, NodeId node) {
  return graph->inEdgesIDs[node+1] - graph->inEdgesIDs[node];
}

//Edge * getInEdges(Graph * graph, NodeId node);
static inline Edge * getInEdges(Graph * graph, NodeId node) {
  EdgeId idx = graph->inEdgesIDs[node];
  return &graph->inEdges[idx];
}

//===----------------------------------------------------------------------===//
//                           Forward Star Interface
//===----------------------------------------------------------------------===//

//size_t getOutDegree(Graph * graph, NodeId node);
static inline size_t getOutDegree(Graph * graph, NodeId node) {
  return graph->outEdgesIDs[node+1] - graph->outEdgesIDs[node];
}

//Edge * getOutEdges(Graph * graph, NodeId node);
static inline Edge * getOutEdges(Graph * graph, NodeId node) {
  EdgeId idx = graph->outEdgesIDs[node];
  return &graph->outEdges[idx];
}

extern void
loadGraph(char * InVertexFileName, char * OutVertexFileName, char * InEdgeFileName, char * OutEdgeFileName);

extern Graph TheGraph;

extern unsigned numAnswers;

extern unsigned counter[N_THREADS];

#endif /* __GRAPH_H__ */
