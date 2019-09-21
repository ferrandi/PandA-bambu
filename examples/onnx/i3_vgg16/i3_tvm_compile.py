import numpy as np
import onnx
import onnxruntime

from tvm import relay
import tvm

onnx_model = onnx.load('i3_vgg16.onnx')

input_name1 = 'data'
input_shape1 = (1,3,224,224)
shape_dict = {input_name1: input_shape1}

mod, params = relay.frontend.from_onnx(model=onnx_model, shape=shape_dict)


# Compilation
opt_level = 3
target = 'llvm'
with relay.build_config(opt_level=opt_level):
    graph, lib, params = relay.build_module.build(
        mod, target, params=params)


#printing some LLVM code
out_file = open("i3_vgg16.ll", "w")
out_file.write(lib.get_source())
out_file.close()


