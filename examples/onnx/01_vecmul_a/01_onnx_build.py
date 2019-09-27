import numpy as np

import onnx
from onnx import helper, shape_inference, optimizer
from onnx import numpy_helper
from onnx import AttributeProto, TensorProto, GraphProto

# Create graph input X
X = helper.make_tensor_value_info('X', TensorProto.FLOAT, [8])
Y = helper.make_tensor_value_info('Y', TensorProto.FLOAT, [8])


# Create graph output Y
Z = helper.make_tensor_value_info('Z', TensorProto.FLOAT, [8])

mul1 = helper.make_node(
        'Mul',         # name
        ['X', 'Y'],     # inputs
        ['Z'],          # outputs
        )

graph_def = helper.make_graph(
        nodes=[mul1],     # graph nodes
        name= 'vecmul_a_model',   # graph name
        inputs =      [X, Y],  # graph inputs
        outputs =     [Z],          # graph outputs
        )

model_def = helper.make_model(graph_def, producer_name='benchmarks')

onnx.checker.check_model(model_def)
model_def = shape_inference.infer_shapes(model_def)
onnx.checker.check_model(model_def)
model_def = optimizer.optimize(model_def)
onnx.checker.check_model(model_def)

onnx.save_model(model_def, '01_vecmul_a.onnx')
