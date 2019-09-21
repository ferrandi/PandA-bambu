import mxnet as mx
import tvm
import tvm.relay as relay
import numpy as np

# Download AlexNet model from Gluon Model Zoo
from mxnet.gluon.model_zoo.vision import get_model
block = get_model('alexnet', pretrained=True)

in_shape = (1,3,224,224)
shape_dict = {'data': in_shape}
mod, params = relay.frontend.from_mxnet(block, shape_dict)

target = 'llvm'
with relay.build_config(opt_level=3):
    graph, lib, params = relay.build(mod, target, params=params)

out_file = open("i1_alexnet.ll", "w")
out_file.write(lib.get_source())
out_file.close()

