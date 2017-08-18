#mem
#set_directive_resource -core RAM_1P_BRAM "gemm" m1
#set_directive_resource -core RAM_1P_BRAM "gemm" m2
#set_directive_resource -core RAM_1P_BRAM "gemm" prod

#partitioning
#set_directive_array_partition -factor 64 -type cyclic gemm m1
#set_directive_array_partition -factor 64 -type cyclic gemm m2
#set_directive_array_partition -factor 64 -type cyclic gemm prod

#unrolling
#set_directive_unroll -factor 8 gemm/inner
#set_directive_unroll -factor 8 gemm/middle
#set_directive_unroll -factor 8 gemm/outter

#pipeline
set_directive_pipeline gemm/inner
set_directive_pipeline gemm/middle
set_directive_pipeline gemm/outter

#resources
set_directive_resource -core Mul "gemm" mult
