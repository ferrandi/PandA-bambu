#set_directive_allocation -limit 8 -type operation fft1D_512 Mul
#set_directive_allocation -limit 64 -type operation fft1D_512 Add

set_directive_resource -core RAM_1P_BRAM "fft1D_512" work_x
set_directive_resource -core RAM_1P_BRAM "fft1D_512" work_y
set_directive_resource -core RAM_1P_BRAM "fft1D_512" DATA_x
set_directive_resource -core RAM_1P_BRAM "fft1D_512" DATA_y
set_directive_resource -core RAM_1P_BRAM "fft1D_512" smem

#set_directive_resource -core RAM_1P_BRAM "twiddles8" cos_512
#set_directive_resource -core RAM_1P_BRAM "twiddles8" sin_512

#set_directive_array_partition fft1D_512 data_x
#set_directive_array_partition fft1D_512 data_y
#set_directive_array_partition storex8 reversed
#set_directive_array_partition storey8 reversed
#set_directive_array_partition fft1D_512 reversed

#set_directive_array_partition "twiddles8" cos_555
#set_directive_array_partition "twiddles8" sin_555

#set_directive_pipeline fft1D_512/loop1
#set_directive_pipeline fft1D_512/loop2
#set_directive_pipeline fft1D_512/loop3
#set_directive_pipeline fft1D_512/loop4
#set_directive_pipeline fft1D_512/loop5
#set_directive_pipeline fft1D_512/loop6
#set_directive_pipeline fft1D_512/loop7
#set_directive_pipeline fft1D_512/loop8
#set_directive_pipeline fft1D_512/loop9
#set_directive_pipeline fft1D_512/loop10
#set_directive_pipeline fft1D_512/loop11

#set_directive_unroll -factor 2 fft1D_512/loop1
#set_directive_unroll -factor 2 fft1D_512/loop2
#set_directive_unroll -factor 2 fft1D_512/loop3
#set_directive_unroll -factor 2 fft1D_512/loop4
#set_directive_unroll -factor 2 fft1D_512/loop5
#set_directive_unroll -factor 2 fft1D_512/loop6
#set_directive_unroll -factor 2 fft1D_512/loop7
#set_directive_unroll -factor 2 fft1D_512/loop8
#set_directive_unroll -factor 2 fft1D_512/loop9
#set_directive_unroll -factor 2 fft1D_512/loop10
#set_directive_unroll -factor 2 fft1D_512/loop11
