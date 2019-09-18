import numpy as np

import onnx
from onnx import helper, shape_inference, optimizer
from onnx import numpy_helper
from onnx import AttributeProto, TensorProto, GraphProto

# Create graph input X
X = helper.make_tensor_value_info('X', TensorProto.FLOAT, [1,64])


# Create graph output Y
Z = helper.make_tensor_value_info('Z', TensorProto.FLOAT, [1,64])

smax1 = helper.make_node(
        'Softmax',         # name
        ['X'],     # inputs
        ['Z'],          # outputs
        )

graph_def = helper.make_graph(
        nodes=[smax1],     # graph nodes
        name= 'softmax_b_model',   # graph name
        inputs =      [X],  # graph inputs
        outputs =     [Z],          # graph outputs
        )

model_def = helper.make_model(graph_def, producer_name='benchmarks')

onnx.checker.check_model(model_def)
model_def = shape_inference.infer_shapes(model_def)
onnx.checker.check_model(model_def)
model_def = optimizer.optimize(model_def)
onnx.checker.check_model(model_def)

onnx.save_model(model_def, '07_softmax_b.onnx')
