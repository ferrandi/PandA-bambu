set_directive_resource -core RAM_1P_BRAM "ellpack" nzval
set_directive_resource -core RAM_1P_BRAM "ellpack" cols
set_directive_resource -core RAM_1P_BRAM "ellpack" vec
set_directive_resource -core RAM_1P_BRAM "ellpack" out

#set_directive_array_partition -factor 64 -type cyclic ellpack nzval
#set_directive_array_partition -factor 64 -type cyclic ellpack cols
#set_directive_array_partition -factor 64 -type cyclic ellpack vec
#set_directive_array_partition -factor 64 -type cyclic ellpack out

#set_directive_unroll -factor 8 ellpack/ellpack_1
#set_directive_unroll -factor 8 ellpack/ellpack_2

#set_directive_pipeline ellpack/ellpack_1
set_directive_pipeline ellpack/ellpack_2

set_directive_resource -core Mul "ellpack" Si
