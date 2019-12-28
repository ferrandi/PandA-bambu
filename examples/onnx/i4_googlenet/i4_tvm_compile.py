import numpy as np
import onnx

from tvm import relay
import tvm

onnx_model = onnx.load('i4_googlenet.onnx')

input_name1 = '0'
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
out_lib = open("i4_googlenet.ll", "w")
out_lib.write(lib.get_source())
out_lib.close()
out_graph = open("i4_googlenet.json", "w")
out_params = open("i4_googlenet.params", "w")
out_graph.write(graph)
out_graph.close()
print(params, file=out_params)
out_params.close()
