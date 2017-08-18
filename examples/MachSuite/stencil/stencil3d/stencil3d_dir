#select functional units you want
set_directive_resource -core Mul "stencil3d" mul1
set_directive_resource -core Mul "stencil3d" mul2

#select memory resources
set_directive_resource -core RAM_1P_BRAM "stencil3d" orig
set_directive_resource -core RAM_1P_BRAM "stencil3d" sol

#loop pipelining factors
set_directive_pipeline stencil3d/loop_height
set_directive_pipeline stencil3d/loop_col
set_directive_pipeline stencil3d/loop_row

#loop unrolling
#set_directive_unroll -factor 2  stencil3d/loop_height 
#set_directive_unroll -factor 2  stencil3d/loop_col
#set_directive_unroll -factor 2  stencil3d/loop_row

#Array partitioning
#set_directive_array_partition -factor 2 -type cyclic stencil3d sol 
#set_directive_array_partition -factor 2 -type cyclic stencil3d orig 
