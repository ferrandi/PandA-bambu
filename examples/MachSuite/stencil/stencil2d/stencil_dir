#select functional units you want
set_directive_resource -core Mul "stencil" mul

#select memory resources
set_directive_resource -core RAM_1P_BRAM "stencil" orig
set_directive_resource -core RAM_1P_BRAM "stencil" sol

#loop pipelining factors
#set_directive_pipeline stencil/stencil_label1
#set_directive_pipeline stencil/stencil_label2
#set_directive_pipeline stencil/stencil_label3
set_directive_pipeline stencil/stencil_label4

#loop unrolling
#set_directive_unroll -factor 2  stencil/stencil_label1 

#Array partitioning
#set_directive_array_partition -factor 2 -type cyclic stencil sol 
#set_directive_array_partition -factor 2 -type cyclic stencil orig 
#set_directive_array_partition -type complete stencil filter 
