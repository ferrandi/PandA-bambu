#mem
set_directive_resource -core RAM_1P_BRAM "fft" x
set_directive_resource -core RAM_1P_BRAM "fft" y

#partitioning
#set_directive_array_partition -factor 64 -type cyclic fft x
#set_directive_array_partition -factor 64 -type cyclic fft y

#unrolling
#set_directive_unroll -factor 8 fft/points_loop
#set_directive_unroll -factor 8 fft/fft_out
#set_directive_unroll -factor 8 fft/fft_mid
#set_directive_unroll -factor 8 fft/fft_in
#set_directive_unroll -factor 8 fft/scale

#pipeline
set_directive_pipeline fft/points_loop
#set_directive_pipeline fft/fft_out
#set_directive_pipeline fft/fft_mid
#set_directive_pipeline fft/fft_in
#set_directive_pipeline fft/scale

#resources
#set_directive_resource -core Mul "fft" T1
#set_directive_resource -core Mul "fft" T2
#set_directive_resource -core Mul "fft" T3
#set_directive_resource -core Mul "fft" T4
#set_directive_resource -core Mul "fft" T5
#set_directive_resource -core Mul "fft" T6
#set_directive_resource -core Mul "fft" T7
#set_directive_resource -core Mul "fft" T8
