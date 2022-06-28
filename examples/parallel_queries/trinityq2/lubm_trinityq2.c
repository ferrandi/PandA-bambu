#include "simple_API.h"
#include <stdio.h>

__attribute__((noinline)) void kernel(size_t i_var_3, Graph* graph, NodeId var_2, PropertyId p_var_3,
                                      PropertyId p_var_5, Edge* var_2_1_inEdges)
{
   unsigned localCounter = 0;
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
      atomicIncrement(&(counter[i_var_3 % N_THREADS]), localCounter);
   }
}

__attribute__((noinline)) void parallel(Graph* graph, NodeId var_2, PropertyId p_var_3, PropertyId p_var_5,
                                        Edge* var_2_1_inEdges, size_t in_degree_var_2)
{
   size_t i_var_3;
#pragma omp parallel for
   for(i_var_3 = 0; i_var_3 < in_degree_var_2; i_var_3++)
   {
      kernel(i_var_3, graph, var_2, p_var_3, p_var_5, var_2_1_inEdges);
   }
}

__attribute__((noinline)) int search(Graph* graph, NodeId var_2, PropertyId p_var_3, PropertyId p_var_5)
{
   size_t in_degree_var_2 = getInDegree(graph, var_2);
#ifndef NDEBUG
   printf("In degree %d\n", in_degree_var_2);
#endif
   Edge* var_2_1_inEdges = getInEdges(graph, var_2);
   parallel(graph, var_2, p_var_3, p_var_5, var_2_1_inEdges, in_degree_var_2);

   for(int i = 0; i < N_THREADS; ++i)
      numAnswers += counter[i];
   return numAnswers;
}

int test(NodeId var_2, PropertyId p_var_3, PropertyId p_var_5)
{
#if defined(DATASETInVertexFile) && defined(DATASETOutVertexFile) && defined(DATASETInEdgeFile) && \
    defined(DATASETOutEdgeFile)
   loadGraph(DATASETInVertexFile, DATASETOutVertexFile, DATASETInEdgeFile, DATASETOutEdgeFile);
#else
   // loadGraph("dataset/40-InVertexFile.bin", "dataset/40-OutVertexFile.bin", "dataset/40-InEdgeFile.bin",
   // "dataset/40-OutEdgeFile.bin");
   loadGraph("dataset/1-InVertexFile.bin", "dataset/1-OutVertexFile.bin", "dataset/1-InEdgeFile.bin",
             "dataset/1-OutEdgeFile.bin");
#endif

   // var_2 = "ub:Course" 16329
   // p_var_3 = "a" 14
   // p_var_5 = "ub:name" 17
   int ret_value = search(&TheGraph, var_2, p_var_3, p_var_5);
#ifndef NDEBUG
   printf("%d\n", ret_value);
#endif
   return ret_value;
}

#ifndef NDEBUG
int main()
{
   return test(2473, 10, 8) != 828;
}
#endif
