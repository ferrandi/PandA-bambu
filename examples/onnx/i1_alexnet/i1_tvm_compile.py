import tvm
import tvm.relay as relay
import numpy as np
import onnx

onnx_model = onnx.load('i1_alexnet.onnx')
in_shape = (1,3,224,224)
shape_dict = {'data_0': in_shape}
mod, params = relay.frontend.from_onnx(onnx_model, shape_dict)

target = 'llvm'
with relay.build_config(opt_level=0):
    graph, lib, params = relay.build(mod, target, params=params)

out_lib = open("i1_alexnet.ll", "w")
out_lib.write(lib.get_source())
out_lib.close()
out_graph = open("i1_alexnet.json", "w")
out_params = open("i1_alexnet.params", "w")
out_graph.write(graph)
out_graph.close()
print(params, file=out_params)
out_params.close()
