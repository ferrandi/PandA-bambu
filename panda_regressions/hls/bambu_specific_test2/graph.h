#ifndef __GRAPH_H
#define __GRAPH_H

#define MAX_VERTEX_NUMBER 2
#define MAX_EDGE_NUMBER 2

typedef unsigned int NodeId;
typedef unsigned int EdgeId;
typedef unsigned int PropertyId;

typedef struct Edge
{
   NodeId node;
   PropertyId property;
} Edge;

typedef struct Graph
{
   size_t numVertices;
   //  PropertyId * VertexPropertyVector;
   EdgeId outEdgesIDs[MAX_VERTEX_NUMBER];
   EdgeId inEdgesIDs[MAX_VERTEX_NUMBER];

   Edge inEdges[MAX_EDGE_NUMBER];
   Edge outEdges[MAX_EDGE_NUMBER];
} Graph;

#endif