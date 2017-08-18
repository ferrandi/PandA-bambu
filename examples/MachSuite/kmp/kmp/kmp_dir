#select functional units you want

#select memory resources
set_directive_resource -core RAM_1P_BRAM "CPF" pattern
set_directive_resource -core RAM_1P_BRAM "CPF" kmpNext
set_directive_resource -core RAM_1P_BRAM "kmp" pattern
set_directive_resource -core RAM_1P_BRAM "kmp" input

#loop pipelining factors
#set_directive_pipeline kmp/init
#set_directive_pipeline kmp/k1
#set_directive_pipeline kmp/k2
#set_directive_pipeline CPF/c1
#set_directive_pipeline CPF/c2

#loop unrolling
#set_directive_unroll -factor 2  kmp/init
#set_directive_unroll -factor 2  kmp/k1
#set_directive_unroll -factor 2  kmp/k2
#set_directive_unroll -factor 2  CDF/c1
#set_directive_unroll -factor 2  CDF/c2

#Array partitioning
set_directive_array_partition -factor 2 -type cyclic kmp pattern
#set_directive_array_partition -factor 2 -type cyclic kmp input 
#set_directive_array_partition -factor 2 -type complete CPF pattern
#set_directive_array_partition -factor 2 -type complete CPF input
