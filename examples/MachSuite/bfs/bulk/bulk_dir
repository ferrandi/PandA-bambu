#select functional units you want
#none..

#select memory resources
set_directive_resource -core RAM_1P_BRAM "bfs" nodes
set_directive_resource -core RAM_1P_BRAM "bfs" edges
set_directive_resource -core RAM_1P_BRAM "bfs" level
set_directive_resource -core RAM_1P_BRAM "bfs" level_counts

#loop pipelining factors
#set_directive_pipeline bfs/init_horizions
#set_directive_pipeline bfs/init_levels
#set_directive_pipeline bfs/loop_horizons
#set_directive_pipeline bfs/loop_nodes
set_directive_pipeline bfs/loop_neighbors

#set_directive_unroll -factor 2 bfs/init_horizions
#set_directive_unroll -factor 2 bfs/init_levels
#set_directive_unroll -factor 2 bfs/loop_horizons 
#set_directive_unroll -factor 2 bfs/loop_nodes
#set_directive_unroll -factor 2 bfs/loop_neighbors

#Array partitioning
#set_directive_array_partition -factor 2 -type cyclic "bfs" nodes 
#set_directive_array_partition -factor 2 -type cyclic "bfs" edges
#set_directive_array_partition -factor 2 -type cyclic "bfs" levels
#set_directive_array_partition -factor 2 -type cyclic "bfs" level_counts
