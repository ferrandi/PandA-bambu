import numpy as np
import onnx
import onnxruntime

from tvm import relay
import tvm

onnx_model = onnx.load('i5_resnet152v1.onnx')

input_name1 = 'data'
input_shape1 = (1,3,224,224)
shape_dict = {input_name1: input_shape1}

mod, params = relay.frontend.from_onnx(model=onnx_model, shape=shape_dict)


# Compilation
opt_level = 0
target = 'llvm'
with relay.build_config(opt_level=opt_level):
    graph, lib, params = relay.build_module.build(
        mod, target, params=params)


#printing some LLVM code
out_lib = open("i5_resnet152v1.ll", "w")
out_lib.write(lib.get_source())
out_lib.close()
out_graph = open("i5_resnet152v1.json", "w")
out_params = open("i5_resnet152v1.params", "w")
out_graph.write(graph)
out_graph.close()
print(params, file=out_params)
out_params.close()

