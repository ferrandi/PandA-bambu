#define GRAPHSAGE_VERTEX_COUNT 6
#define GRAPHSAGE_EDGE_COUNT 12
#define GRAPHSAGE_FEATURE_DIMENSION 3
#define GRAPHSAGE_WORKER_COUNT 2

extern "C" void __attribute__((noinline))
graphsage_mean(const int row_offsets[GRAPHSAGE_VERTEX_COUNT + 1], const int neighbors[GRAPHSAGE_EDGE_COUNT],
               const int features[GRAPHSAGE_VERTEX_COUNT * GRAPHSAGE_FEATURE_DIMENSION],
               int output[GRAPHSAGE_VERTEX_COUNT * GRAPHSAGE_FEATURE_DIMENSION])
{
#pragma omp parallel num_threads(GRAPHSAGE_WORKER_COUNT)
   {
#pragma omp for
      for(int vertex = 0; vertex < GRAPHSAGE_VERTEX_COUNT; ++vertex)
      {
         const int begin = row_offsets[vertex];
         const int end = row_offsets[vertex + 1];
         const int degree = end - begin;
         for(int feature = 0; feature < GRAPHSAGE_FEATURE_DIMENSION; ++feature)
         {
            int sum = 0;
            for(int edge = begin; edge < end; ++edge)
               sum += features[neighbors[edge] * GRAPHSAGE_FEATURE_DIMENSION + feature];
            output[vertex * GRAPHSAGE_FEATURE_DIMENSION + feature] = degree == 0 ? 0 : sum / degree;
         }
      }
   }
}
