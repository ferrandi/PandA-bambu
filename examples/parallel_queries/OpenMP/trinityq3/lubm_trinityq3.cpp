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
extern "C" __attribute__((noinline)) int search(Graph* graph, NodeId var_2, PropertyId p_var_3, PropertyId p_var_5,
                                                PropertyId p_var_6, PropertyId p_var_8, PropertyId p_var_9,
                                                PropertyId p_var_10, PropertyId p_var_11, PropertyId p_var_12)
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
            size_t i_var_10;
            for(i_var_10 = 0; i_var_10 < out_degree_var_1; i_var_10++)
            {
               PropertyId var_10; // corresponding to element having label "ub:memberOf"
               var_10 = var_1_3_outEdges[i_var_10].property;
               NodeId var_7; // corresponding to element having label "?Z"
               var_7 = var_1_3_outEdges[i_var_10].node;
               int cond_level_4 = (var_10 == p_var_10);
               if(cond_level_4)
               {
                  size_t out_degree_var_7 = getOutDegree(graph, var_7);
                  Edge* var_7_5_outEdges = getOutEdges(graph, var_7);
                  size_t i_var_9;
                  for(i_var_9 = 0; i_var_9 < out_degree_var_7; i_var_9++)
                  {
                     PropertyId var_9; // corresponding to element having label "a"
                     var_9 = var_7_5_outEdges[i_var_9].property;
                     NodeId var_8; // corresponding to element having label "ub:Department"
                     var_8 = var_7_5_outEdges[i_var_9].node;
                     int cond_level_6 = ((var_9 == p_var_9) & (var_8 == p_var_8));
                     if(cond_level_6)
                     {
                        Edge* var_1_7_outEdges = getOutEdges(graph, var_1);
                        size_t i_var_12;
                        for(i_var_12 = 0; i_var_12 < out_degree_var_1; i_var_12++)
                        {
                           PropertyId var_12; // corresponding to element having label "ub:undergraduateDegreeFrom"
                           var_12 = var_1_7_outEdges[i_var_12].property;
                           NodeId var_4; // corresponding to element having label "?Y"
                           var_4 = var_1_7_outEdges[i_var_12].node;
                           int cond_level_8 = (var_12 == p_var_12);
                           if(cond_level_8)
                           {
                              size_t in_degree_var_4 = getInDegree(graph, var_4);
                              Edge* var_4_9_inEdges = getInEdges(graph, var_4);
                              size_t i_var_11;
                              for(i_var_11 = 0; i_var_11 < in_degree_var_4; i_var_11++)
                              {
                                 PropertyId var_11; // corresponding to element having label "ub:subOrganizationOf"
                                 var_11 = var_4_9_inEdges[i_var_11].property;
                                 NodeId var_7_10; // corresponding to element having label "?Z"
                                 var_7_10 = var_4_9_inEdges[i_var_11].node;
                                 int cond_level_10 = ((var_11 == p_var_11) & (var_7_10 == var_7));
                                 if(cond_level_10)
                                 {
                                    size_t out_degree_var_4 = getOutDegree(graph, var_4);
                                    Edge* var_4_11_outEdges = getOutEdges(graph, var_4);
                                    size_t i_var_6;
                                    for(i_var_6 = 0; i_var_6 < out_degree_var_4; i_var_6++)
                                    {
                                       PropertyId var_6; // corresponding to element having label "a"
                                       var_6 = var_4_11_outEdges[i_var_6].property;
                                       NodeId var_5; // corresponding to element having label "ub:University"
                                       var_5 = var_4_11_outEdges[i_var_6].node;
                                       int cond_level_12 = ((var_6 == p_var_6) & (var_5 == p_var_5));
                                       if(cond_level_12)
                                       {
                                          // here the "required" results are written (if any)
                                          localCounter++;
                                       }
                                    }
                                 }
                              }
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }
   return localCounter;
}

int test(NodeId var_2, PropertyId p_var_3, PropertyId p_var_5, PropertyId p_var_6, PropertyId p_var_8,
         PropertyId p_var_9, PropertyId p_var_10, PropertyId p_var_11, PropertyId p_var_12)
{
   // var_2 = "ub:Undergraduate-Student" 14398
   // p_var_3 = "a" 14
   // p_var_5 = "ub:University" 11347
   // p_var_6 = "a" 14
   // p_var_8 = "ub:Department" 11412
   // p_var_9 = "a" 14
   // p_var_10 = "ub:memberOf" 4
   // p_var_11 = "ub:subOrganizationOf" 5
   // p_var_12 = "ub:undergraduateDegreeFrom" 6 0
#ifdef __BAMBU_SIM__
   m_param_alloc(0, sizeof(TheGraph));
#endif
   return search(&TheGraph, var_2, p_var_3, p_var_5, p_var_6, p_var_8, p_var_9, p_var_10, p_var_11, p_var_12);
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
   return test(25273, 10, 1685, 10, 1828, 10, 4, 14, 3) != 0;
}
