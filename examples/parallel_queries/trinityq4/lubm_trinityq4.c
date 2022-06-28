#include "simple_API.h"
#include <stdio.h>

// var_2 = "<http://www.Department0.University0.edu>"
// p_var_3 = "ub:worksFor"
// p_var_4 = "ub:FullProfessor"
// p_var_5 = "a"
// p_var_7 = "ub:name"
// p_var_9 = "ub:emailAddress"
// p_var_11 = "ub:telephone"

__attribute__((noinline)) void kernel(size_t i_var_3, Graph* graph, NodeId var_2, PropertyId p_var_3,
                                      PropertyId p_var_4, PropertyId p_var_5, PropertyId p_var_7, PropertyId p_var_9,
                                      PropertyId p_var_11, size_t in_degree_var_2, Edge* var_2_1_inEdges)
{
   unsigned localCounter = 0;
   PropertyId var_3; // corresponding to element having label "ub:worksFor"
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
         PropertyId var_5; // corresponding to element having label "a"
         var_5 = var_1_3_outEdges[i_var_5].property;
         NodeId var_4; // corresponding to element having label "ub:FullProfessor"
         var_4 = var_1_3_outEdges[i_var_5].node;
         int cond_level_4 = ((var_5 == p_var_5) & (var_4 == p_var_4));
         if(cond_level_4)
         {
            Edge* var_1_5_outEdges = getOutEdges(graph, var_1);
            size_t i_var_7;
            for(i_var_7 = 0; i_var_7 < out_degree_var_1; i_var_7++)
            {
               PropertyId var_7; // corresponding to element having label "ub:name"
               var_7 = var_1_5_outEdges[i_var_7].property;
               NodeId var_6; // corresponding to element having label "?Y1"
               var_6 = var_1_5_outEdges[i_var_7].node;
               int cond_level_6 = (var_7 == p_var_7);
               if(cond_level_6)
               {
                  Edge* var_1_7_outEdges = getOutEdges(graph, var_1);
                  size_t i_var_9;
                  for(i_var_9 = 0; i_var_9 < out_degree_var_1; i_var_9++)
                  {
                     PropertyId var_9; // corresponding to element having label "ub:emailAddress"
                     var_9 = var_1_7_outEdges[i_var_9].property;
                     NodeId var_8; // corresponding to element having label "?Y2"
                     var_8 = var_1_7_outEdges[i_var_9].node;
                     int cond_level_8 = (var_9 == p_var_9);
                     if(cond_level_8)
                     {
                        Edge* var_1_9_outEdges = getOutEdges(graph, var_1);
                        size_t i_var_11;
                        for(i_var_11 = 0; i_var_11 < out_degree_var_1; i_var_11++)
                        {
                           PropertyId var_11; // corresponding to element having label "ub:telephone"
                           var_11 = var_1_9_outEdges[i_var_11].property;
                           NodeId var_10; // corresponding to element having label "?Y3"
                           var_10 = var_1_9_outEdges[i_var_11].node;
                           int cond_level_10 = (var_11 == p_var_11);
                           if(cond_level_10)
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
      atomicIncrement(&(counter[i_var_3 % N_THREADS]), localCounter);
   }
}

__attribute__((noinline)) void parallel(Graph* graph, NodeId var_2, PropertyId p_var_3, PropertyId p_var_4,
                                        PropertyId p_var_5, PropertyId p_var_7, PropertyId p_var_9, PropertyId p_var_11,
                                        size_t in_degree_var_2, Edge* var_2_1_inEdges)
{
   size_t i_var_3;
#pragma omp parallel for
   for(i_var_3 = 0; i_var_3 < in_degree_var_2; i_var_3++)
   {
      kernel(i_var_3, graph, var_2, p_var_3, p_var_4, p_var_5, p_var_7, p_var_9, p_var_11, in_degree_var_2,
             var_2_1_inEdges);
   }
}

__attribute__((noinline)) int search(Graph* graph, NodeId var_2, PropertyId p_var_3, PropertyId p_var_4,
                                     PropertyId p_var_5, PropertyId p_var_7, PropertyId p_var_9, PropertyId p_var_11)
{
   size_t in_degree_var_2 = getInDegree(graph, var_2);
   Edge* var_2_1_inEdges = getInEdges(graph, var_2);
#ifndef NDEBUG
   printf("In degree %d\n", in_degree_var_2);
#endif
   parallel(graph, var_2, p_var_3, p_var_4, p_var_5, p_var_7, p_var_9, p_var_11, in_degree_var_2, var_2_1_inEdges);
   for(int i = 0; i < N_THREADS; ++i)
      numAnswers += counter[i];
   return numAnswers;
}

int test(NodeId var_2, PropertyId p_var_3, PropertyId p_var_4, PropertyId p_var_5, PropertyId p_var_7,
         PropertyId p_var_9, PropertyId p_var_11)
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

   // var_2 = "<http://www.Department0.University0.edu>" 4804
   // p_var_3 = "ub:worksFor" 16
   // p_var_4 = "ub:FullProfessor" 21219
   // p_var_5 = "a" 14
   // p_var_7 = "ub:name" 17
   // p_var_9 = "ub:emailAddress" 10
   // p_var_11 = "ub:telephone" 11
   int ret_value = search(&TheGraph, var_2, p_var_3, p_var_4, p_var_5, p_var_7, p_var_9, p_var_11);
#ifndef NDEBUG
   printf("%d\n", ret_value);
#endif
   return ret_value;
}

#ifndef NDEBUG
int main()
{
   return test(8204, 2, 6764, 10, 8, 17, 16) != 10;
   // return test(4804, 16, 21219, 14, 17, 10, 11);
}
#endif
