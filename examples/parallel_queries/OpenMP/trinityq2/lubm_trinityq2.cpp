#include "simple_API.hpp"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

Graph TheGraph;

void loadGraph(char* InVertexFileName, char* OutVertexFileName, char* InEdgeFileName, char* OutEdgeFileName)
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
   printf("VertexNumber : %d\n", TheGraph.numVertices);
   printf("InEdgeNumber : %d\n", inEdgeNumber);
   printf("outEdgeNumber : %d\n", outEdgeNumber);

   bytes_read = read(IVF, TheGraph.inEdgesIDs, sizeof(TheGraph.inEdgesIDs[0]) * (TheGraph.numVertices + 1));
   assert(bytes_read == sizeof(TheGraph.inEdgesIDs[0]) * (TheGraph.numVertices + 1));
   bytes_read = read(OVF, TheGraph.outEdgesIDs, sizeof(TheGraph.outEdgesIDs[0]) * (TheGraph.numVertices + 1));
   assert(bytes_read == sizeof(TheGraph.outEdgesIDs[0]) * (TheGraph.numVertices + 1));

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

#pragma HLS bus bank_number = 16 chunk_size = 4
#pragma HLS interface port = graph mode = bus
#if CACHE == 1
#pragma HLS cache bundle = bus line_count = 512 line_size = 8 bus_size = 64 ways = 1 num_write_outstanding = \
    2 rep_policy = lru write_policy = wt
#endif
extern "C" __attribute__((noinline)) int search(Graph* graph, NodeId var_2, PropertyId p_var_3, PropertyId p_var_5)
{
   size_t in_degree_var_2 = getInDegree(graph, var_2);
   Edge* var_2_1_inEdges = getInEdges(graph, var_2);
   unsigned localCounter = 0;
#pragma omp parallel num_threads(N_THREADS)
   {
#pragma omp for schedule(static, 1) reduction(+ : localCounter)
      for(size_t i_var_3 = 0; i_var_3 < in_degree_var_2; i_var_3++)
      {
         PropertyId var_3; // corresponding to element having label "a"
         var_3 = var_2_1_inEdges[i_var_3].property;
         NodeId var_1; // corresponding to element having label "?X"
         var_1 = var_2_1_inEdges[i_var_3].node;
         int cond_level_2 = (var_3 == p_var_3);
         if(cond_level_2)
         {
            size_t out_degree_var_1 = getOutDegree(graph, var_1);
            Edge* var_1_3_outEdges = getOutEdges(graph, var_1);
            size_t i_var_5;
            for(i_var_5 = 0; i_var_5 < out_degree_var_1; i_var_5++)
            {
               PropertyId var_5; // corresponding to element having label "ub:name"
               var_5 = var_1_3_outEdges[i_var_5].property;
               NodeId var_4; // corresponding to element having label "?Y"
               var_4 = var_1_3_outEdges[i_var_5].node;
               int cond_level_4 = (var_5 == p_var_5);
               if(cond_level_4)
               {
                  // here the "required" results are written (if any)
                  localCounter++;
               }
            }
         }
      }
   }
   return localCounter;
}

int test(NodeId var_2, PropertyId p_var_3, PropertyId p_var_5)
{
   // var_2 = "ub:Course" 16329
   // p_var_3 = "a" 14
   // p_var_5 = "ub:name" 17
#ifdef __BAMBU_SIM__
   m_param_alloc(0, sizeof(TheGraph));
#endif
   return search(&TheGraph, var_2, p_var_3, p_var_5);
}

int main(int argc, char* argv[])
{
   char* DATASETInVertexFile;
   char* DATASETOutVertexFile;
   char* DATASETInEdgeFile;
   char* DATASETOutEdgeFile;
   if(argc == 5)
   {
      DATASETInVertexFile = argv[1];
      DATASETOutVertexFile = argv[2];
      DATASETInEdgeFile = argv[3];
      DATASETOutEdgeFile = argv[4];
   }
   else
   {
      printf("Missing graph paths\n");
      abort();
   }
   loadGraph(DATASETInVertexFile, DATASETOutVertexFile, DATASETInEdgeFile, DATASETOutEdgeFile);
   return test(2473, 10, 8) != 828;
}
