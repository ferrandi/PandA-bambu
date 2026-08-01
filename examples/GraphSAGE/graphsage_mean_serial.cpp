#include "graphsage_constants.hpp"

extern "C" void __attribute__((noinline))
graphsage_mean_serial(const int row_offsets[GRAPHSAGE_VERTEX_COUNT + 1],
                      const int neighbors[GRAPHSAGE_EDGE_COUNT], const int features[GRAPHSAGE_OUTPUT_COUNT],
                      int output[GRAPHSAGE_OUTPUT_COUNT])
{
   for(int vertex = 0; vertex < GRAPHSAGE_VERTEX_COUNT; ++vertex)
   {
      const int begin = row_offsets[vertex], end = row_offsets[vertex + 1], degree = end - begin;
      for(int feature = 0; feature < GRAPHSAGE_FEATURE_DIMENSION; ++feature)
      {
         int sum = 0;
         for(int edge = begin; edge < end; ++edge)
            sum += features[neighbors[edge] * GRAPHSAGE_FEATURE_DIMENSION + feature];
         output[vertex * GRAPHSAGE_FEATURE_DIMENSION + feature] = degree == 0 ? 0 : sum / degree;
      }
   }
}
