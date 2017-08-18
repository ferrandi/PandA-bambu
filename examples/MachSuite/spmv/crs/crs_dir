set_directive_resource -core RAM_1P_BRAM "spmv" val
set_directive_resource -core RAM_1P_BRAM "spmv" cols
set_directive_resource -core RAM_1P_BRAM "spmv" rowDelimiters
set_directive_resource -core RAM_1P_BRAM "spmv" vec
set_directive_resource -core RAM_1P_BRAM "spmv" out

#set_directive_array_partition -factor 64 -type cyclic spmv val
#set_directive_array_partition -factor 64 -type cyclic spmv cols
#set_directive_array_partition -factor 64 -type cyclic spmv rowDelimiters
#set_directive_array_partition -factor 64 -type cyclic spmv out

#set_directive_unroll -factor 8 spmv/spmv_1
#set_directive_unroll -factor 8 spmv/spmv_2

#set_directive_pipeline spmv/spmv_1
set_directive_pipeline spmv/spmv_2

set_directive_resource -core Mul "spmv" Si
