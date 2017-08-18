#select functional units you want
#no mults... feel free to sweep other things!

#select memory resources
set_directive_resource -core RAM_1P_BRAM "aes256_encrypt_ecb" ctx
set_directive_resource -core RAM_1P_BRAM "aes256_encrypt_ecb" k
set_directive_resource -core RAM_1P_BRAM "aes256_encrypt_ecb" buf

set_directive_resource -core RAM_1P_BRAM "aes_expandEncKey" k
set_directive_resource -core RAM_1P_BRAM "aes_expandEncKey" rc

set_directive_resource -core RAM_1P_BRAM "aes_mixColumns" buf

set_directive_resource -core RAM_1P_BRAM "aes_shiftRows" buf

set_directive_resource -core RAM_1P_BRAM "aes_addRoundKey_cpy" buf
set_directive_resource -core RAM_1P_BRAM "aes_addRoundKey_cpy" key
set_directive_resource -core RAM_1P_BRAM "aes_addRoundKey_cpy" cpk

set_directive_resource -core RAM_1P_BRAM "add_addRoundKey" buf
set_directive_resource -core RAM_1P_BRAM "aes_addRoundKey" key

set_directive_resource -core RAM_1P_BRAM "aes_subBytes" buf


#loop unrolling
#set_directive_partition -factor 2 -type cyclic aes256_encrypt_ecb ctx
#set_directive_partition -factor 2 -type cyclic aes256_encrypt_ecb k
#set_directive_partition -factor 2 -type cyclic aes256_encrypt_ecb buf

#set_directive_partition -factor 2 -type cyclic aes_expandEncKey k
#set_directive_partition -factor 2 -type cyclic aes_expandEncKey rc

#set_directive_partition -factor 2 -type cyclic aes_mixColumns buf

#set_directive_partition -factor 2 -type cyclic aes_shiftRows buf

#set_directive_partition -factor 2 -type cyclic aes_addRoundKey_cpy buf
#set_directive_partition -factor 2 -type cyclic aes_addRoundKey_cpy key
#set_directive_partition -factor 2 -type cyclic aes_addRoundKey_cpy cpk

#set_directive_partition -factor 2 -type cyclic add_addRoundKey buf
#set_directive_partition -factor 2 -type cyclic aes_addRoundKey key

#set_directive_partition -factor 2 -type cyclic aes_subBytes buf

##
#loop pipelining factors
##

#set_directive_pipeline aes256_encrypt_ecb/ecb1
#set_directive_pipeline aes256_encrypt_ecb/ecb2
#set_directive_pipeline aes256_encrypt_ecb/ecb3

#set_directive_pipeline aes_expandEncKey/exp1
#set_directive_pipeline aes_expandEncKey/exp2

set_directive_pipeline aes_mixColumns/mix

#set_directive_pipeline aes_addRoundKey_cpy/cpkey

#set_directive_pipeline aes_addRoundKey/addkey

#set_directive_pipeline aes_subBytes/sub

#set_directive_pipeline gf_log/glog

#set_directive_pipeline gf_alog/alog

#loop unrolling
#set_directive_unroll -factor 2 aes256_encrypt_ecb/ecb1
#set_directive_unroll -factor 2 aes256_encrypt_ecb/ecb2
#set_directive_unroll -factor 2 aes256_encrypt_ecb/ecb3

#set_directive_unroll -factor 2 aes_expandEncKey/exp1
#set_directive_unroll -factor 2 aes_expandEncKey/exp2

#set_directive_unroll -factor 2 aes_mixColumns/mix

#set_directive_unroll -factor 2 aes_addRoundKey_cpy/cpkey

#set_directive_unroll -factor 2 aes_addRoundKey/addkey

#set_directive_unroll -factor 2 aes_subBytes/sub

#set_directive_unroll -factor 2 gf_log/glog

#set_directive_unroll -factor 2 gf_alog/alog
