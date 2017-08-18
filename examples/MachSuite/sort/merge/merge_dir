#select memory resources
set_directive_resource -core RAM_1P_BRAM "ms_mergesort" a
set_directive_resource -core RAM_1P_BRAM "merge" a

#loop pipelining factors
#set_directive_pipeline ms_mergesort/mergesort_label1
set_directive_pipeline ms_mergesort/mergesort_label2

set_directive_pipeline merge/merge_label1
set_directive_pipeline merge/merge_label2
set_directive_pipeline merge/merge_label3

#loop unrolling
#set_directive_unroll -factor 2  ms_mergesort/mergesort_label1
#set_directive_unroll -factor 2  ms_mergesort/mergesort_label2

#set_directive_unroll -factor 2  merge/merge_label1
#set_directive_unroll -factor 2  merge/merge_label2
#set_directive_unroll -factor 2  merge/merge_label3

#Array partitioning
#set_directive_array_partition -factor 2 -type cyclic ms_mergesort a
#set_directive_array_partition -factor 2 -type cyclic merge a
