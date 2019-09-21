import numpy as np
import onnx
import onnxruntime

from tvm import relay
import tvm

onnx_model = onnx.load('01_vecmul_a.onnx')

input_name1 = 'X'
input_shape1 = (1,8)
input_name2 = 'Y'
input_shape2 = (1,8)
shape_dict = {input_name1: input_shape1, input_name2: input_shape2}


mod, params = relay.frontend.from_onnx(model=onnx_model, shape=shape_dict)


# Compilation
opt_level = 3
target = 'llvm'
with relay.build_config(opt_level=opt_level):
    graph, lib, params = relay.build_module.build(
        mod, target, params=params)


#printing some LLVM code
out_file = open("01_vecmul_a.ll", "w")
out_file.write(lib.get_source())
out_file.close()


