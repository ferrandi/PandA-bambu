import onnx

from tvm import relay
import tvm
import os

onnx_model = onnx.load('lenet.onnx')

input_name = 'import/Placeholder:0'
input_shape = (1,1,28,28)
shape_dict = {input_name: input_shape}

mod, params = relay.frontend.from_onnx(model=onnx_model, shape=shape_dict)

opt_level=0
target = 'llvm'
with relay.build_config(opt_level=opt_level):
    graph, lib, params = relay.build_module.build(mod, target, params=params)


out_lib = open("lenet.ll", "w")
out_graph = open("lenet.json", "w")
out_params = open("lenet.params", "w")
out_lib.write(lib.get_source())
out_lib.close()
out_graph.write(graph)
out_graph.close()
print(params, file=out_params)
out_params.close()
