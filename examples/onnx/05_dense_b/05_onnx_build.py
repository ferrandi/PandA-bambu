import numpy as np

import onnx
from onnx import helper, shape_inference, optimizer
from onnx import numpy_helper
from onnx import AttributeProto, TensorProto, GraphProto

# Create graph input X
X = helper.make_tensor_value_info('X', TensorProto.FLOAT, [1])

W_info = helper.make_tensor_value_info('W', TensorProto.FLOAT, [1,64])
W = np.ones((1,64)).astype(np.float32)
W = numpy_helper.from_array(W, 'W')

B_info = helper.make_tensor_value_info('B', TensorProto.FLOAT, [64])
B = np.ones((64)).astype(np.float32)
B = numpy_helper.from_array(B, 'B')

# Create graph output Y
Z = helper.make_tensor_value_info('Z', TensorProto.FLOAT, [64])

matmul1 = helper.make_node(
        'MatMul',         # name
        ['X', 'W'],     # inputs
        ['Y'],          # outputs
        )

bias1 = helper.make_node(
        'Add',
        ['Y', 'B'],
        ['Z'],
        )


graph_def = helper.make_graph(
        nodes=[matmul1, bias1],     # graph nodes
        name= 'dense_b_model',   # graph name
        inputs =      [X, W_info, B_info],  # graph inputs
        outputs =     [Z],          # graph outputs
        initializer = [W, B],
        )

model_def = helper.make_model(graph_def, producer_name='benchmarks')

onnx.checker.check_model(model_def)
model_def = shape_inference.infer_shapes(model_def)
onnx.checker.check_model(model_def)
model_def = optimizer.optimize(model_def)
onnx.checker.check_model(model_def)

onnx.save_model(model_def, '05_dense_b.onnx')
