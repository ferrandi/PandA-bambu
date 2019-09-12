#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include "simple_API.h"

void
loadGraph(char * InVertexFileName, char * OutVertexFileName, char * InEdgeFileName, char * OutEdgeFileName)
{
  int IVF, OVF, IEF, OEF;

  IVF = open(InVertexFileName, O_RDONLY);
  assert(IVF != -1);
  OVF = open(OutVertexFileName, O_RDONLY);
  assert(OVF != -1);
  IEF = open(InEdgeFileName, O_RDONLY);
  assert(IEF != -1);
  OEF = open(OutEdgeFileName, O_RDONLY);
  assert(OEF != -1);

  unsigned int vertexNumber, secondvertexNumber;
  unsigned int inEdgeNumber;
  unsigned int outEdgeNumber;
  unsigned bytes_read;
  bytes_read = read(IVF, &vertexNumber, sizeof(unsigned int));
  assert(bytes_read == sizeof(unsigned int));
  bytes_read = read(OVF, &secondvertexNumber, sizeof(unsigned int));
  assert(secondvertexNumber == vertexNumber);
  assert(bytes_read == sizeof(unsigned int));

  bytes_read = read(IEF, &inEdgeNumber, sizeof(unsigned int));
  assert(bytes_read == sizeof(unsigned int));
  bytes_read = read(OEF, &outEdgeNumber, sizeof(unsigned int));
  assert(bytes_read == sizeof(unsigned int));

  TheGraph.numVertices = vertexNumber - 1;
  printf("VertexNumber : %lu\n", TheGraph.numVertices);
  printf("InEdgeNumber : %d\n", inEdgeNumber);
  printf("outEdgeNumber : %d\n", outEdgeNumber);

  bytes_read = read(IVF, TheGraph.inEdgesIDs,  sizeof(TheGraph.inEdgesIDs[0])  * (TheGraph.numVertices + 1));
  assert(bytes_read == sizeof(TheGraph.inEdgesIDs[0])  * (TheGraph.numVertices + 1));
  bytes_read = read(OVF, TheGraph.outEdgesIDs, sizeof(TheGraph.outEdgesIDs[0]) * (TheGraph.numVertices + 1));
  assert(bytes_read == sizeof(TheGraph.outEdgesIDs[0])  * (TheGraph.numVertices + 1));

  bytes_read = read(IEF, TheGraph.inEdges, sizeof(Edge) * inEdgeNumber);
  assert(bytes_read == sizeof(Edge) * inEdgeNumber);
  bytes_read = read(OEF, TheGraph.outEdges, sizeof(Edge) * outEdgeNumber);
  assert(bytes_read == sizeof(Edge) * outEdgeNumber);

  printf("Graph Loading Completed!\n");

  close(IVF);
  close(OVF);
  close(IEF);
  close(OEF);
}

