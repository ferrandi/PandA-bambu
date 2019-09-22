import numpy as np

import onnx
from onnx import helper, shape_inference, optimizer
from onnx import numpy_helper
from onnx import AttributeProto, TensorProto, GraphProto

# Create graph input X
X = helper.make_tensor_value_info('X', TensorProto.FLOAT, [1, 1, 32, 32])

# Create graph output Y
Y = helper.make_tensor_value_info('Y', TensorProto.FLOAT, [1, 1, 10, 10])

maxp1 = helper.make_node(
        'MaxPool',         # name
        ['X'],     # inputs
        ['Y'],          # outputs
        ## Node Attributes
        auto_pad = 'VALID',
        kernel_shape=[3, 3],
        strides=[3, 3],
        )


graph_def = helper.make_graph(
        nodes=[maxp1],     # graph nodes
        name= 'maxp_b_model',   # graph name
        inputs =      [X],  # graph inputs
        outputs =     [Y],          # graph outputs
        )

model_def = helper.make_model(graph_def, producer_name='benchmarks')

onnx.checker.check_model(model_def)
model_def = shape_inference.infer_shapes(model_def)
onnx.checker.check_model(model_def)
model_def = optimizer.optimize(model_def)
onnx.checker.check_model(model_def)

onnx.save_model(model_def, '13_maxp_b.onnx')
