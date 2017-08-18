#select memory resources
set_directive_resource -core RAM_1P_BRAM "ss_sort" a
set_directive_resource -core RAM_1P_BRAM "ss_sort" b
set_directive_resource -core RAM_1P_BRAM "ss_sort" bucket
set_directive_resource -core RAM_1P_BRAM "ss_sort" sum

set_directive_resource -core RAM_1P_BRAM "update" a
set_directive_resource -core RAM_1P_BRAM "update" b
set_directive_resource -core RAM_1P_BRAM "update" bucket

set_directive_resource -core RAM_1P_BRAM "hist" a
set_directive_resource -core RAM_1P_BRAM "hist" bucket

set_directive_resource -core RAM_1P_BRAM "init" bucket

set_directive_resource -core RAM_1P_BRAM "last_step_scan" bucket
set_directive_resource -core RAM_1P_BRAM "last_step_scan" sum

set_directive_resource -core RAM_1P_BRAM "sum_scan" bucket
set_directive_resource -core RAM_1P_BRAM "sum_scan" sum

set_directive_resource -core RAM_1P_BRAM "local_scan" bucket

#loop pipelining factors
#set_directive_pipeline local_scan/local_1
#set_directive_pipeline local_scan/local_2

#set_directive_pipeline sum_scan/sum_1

#set_directive_pipeline last_step_scan/last_1
#set_directive_pipeline last_step_scan/last_2

#set_directive_pipeline init/init_1

#set_directive_pipeline hist/hist_1
#set_directive_pipeline hist/hist_2

#set_directive_pipeline update/update_1
#set_directive_pipeline update/update_2

#set_directive_pipeline ss_sort/sort_1

#loop unrolling
#set_directive_unroll -factor 2 local_scan/local_1
#set_directive_unroll -factor 2 local_scan/local_2

#set_directive_unroll -factor 2 sum_scan/sum_1

#set_directive_unroll -factor 2 last_step_scan/last_1
#set_directive_unroll -factor 2 last_step_scan/last_2

#set_directive_unroll -factor 2 init/init_1

#set_directive_unroll -factor 2 hist/hist_1
#set_directive_unroll -factor 2 hist/hist_2

#set_directive_unroll -factor 2 update/update_1
#set_directive_unroll -factor 2 update/update_2

#set_directive_unroll -factor 2 ss_sort/sort_1

#Array partitioning
#set_directive_array_partition -factor 2 -type cyclic local_scan bucket

#set_directive_array_partition -factor 2 -type cyclic sum_scan sum
#set_directive_array_partition -factor 2 -type cyclic sum_scan bucket

#set_directive_array_partition -factor 2 -type cyclic last_step_scan bucket
#set_directive_array_partition -factor 2 -type cyclic last_step_scan sum

#set_directive_array_partition -factor 2 -type cyclic init bucket

#set_directive_array_partition -factor 2 -type cyclic hist bucket
#set_directive_array_partition -factor 2 -type cyclic hist a

#set_directive_array_partition -factor 2 -type cyclic update b
#set_directive_array_partition -factor 2 -type cyclic update a
#set_directive_array_partition -factor 2 -type cyclic update bucket

#set_directive_array_partition -factor 2 -type cyclic ss_sort a
#set_directive_array_partition -factor 2 -type cyclic ss_sort b
#set_directive_array_partition -factor 2 -type cyclic ss_sort bucket
#set_directive_array_partition -factor 2 -type cyclic ss_sort sum
