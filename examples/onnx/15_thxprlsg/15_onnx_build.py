import numpy as np

import onnx
from onnx import helper, shape_inference, optimizer
from onnx import numpy_helper
from onnx import AttributeProto, TensorProto, GraphProto

# Create graph input X
X = helper.make_tensor_value_info('X', TensorProto.FLOAT, [1, 8])

# Create graph output Z
Z = helper.make_tensor_value_info('Z', TensorProto.FLOAT, [1, 8])

tanh1 = helper.make_node(
        'Tanh',         # name
        ['X'],     # inputs
        ['Y'],          # outputs
        )

exp2 = helper.make_node(
        'Exp',
        ['Y'],
        ['V'],
        )

relu3 = helper.make_node(
        'Relu',
        ['V'],
        ['VV']
        )

sigmoid4 = helper.make_node(
        'Sigmoid',
        ['VV'],
        ['Z'],
        )


graph_def = helper.make_graph(
        nodes=[tanh1, exp2, relu3, sigmoid4],     # graph nodes
        name= 'thxprlsg_model',   # graph name
        inputs =      [X],  # graph inputs
        outputs =     [Z],          # graph outputs
        )

model_def = helper.make_model(graph_def, producer_name='benchmarks')

onnx.checker.check_model(model_def)
model_def = shape_inference.infer_shapes(model_def)
onnx.checker.check_model(model_def)
model_def = optimizer.optimize(model_def)
onnx.checker.check_model(model_def)

onnx.save_model(model_def, '15_thxprlsg.onnx')
