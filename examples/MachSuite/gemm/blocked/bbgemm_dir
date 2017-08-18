#mem
#set_directive_resource -core RAM_1P_BRAM "bbgemm" m1
#set_directive_resource -core RAM_1P_BRAM "bbgemm" m2
#set_directive_resource -core RAM_1P_BRAM "bbgemm" prod

#partitioning
#set_directive_array_partition -factor 64 -type cyclic bbgemm m1
#set_directive_array_partition -factor 64 -type cyclic bbgemm m2
#set_directive_array_partition -factor 64 -type cyclic bbgemm prod

#unrolling
#set_directive_unroll -factor 8 bbgemm/loopjj
#set_directive_unroll -factor 8 bbgemm/loopkk
#set_directive_unroll -factor 8 bbgemm/loopi
#set_directive_unroll -factor 8 bbgemm/loopk
#set_directive_unroll -factor 8 bbgemm/loopj

#pipeline
#set_directive_pipeline bbgemm/loopjj
#set_directive_pipeline bbgemm/loopkk
#set_directive_pipeline bbgemm/loopi
#set_directive_pipeline bbgemm/loopk
#set_directive_pipeline bbgemm/loopj

#resources
set_directive_resource -core Mul "bbgemm" mul
