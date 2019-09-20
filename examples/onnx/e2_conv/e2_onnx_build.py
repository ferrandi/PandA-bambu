import numpy as np

import onnx
from onnx import helper, shape_inference, optimizer
from onnx import numpy_helper
from onnx import AttributeProto, TensorProto, GraphProto

# Create graph input X
X = helper.make_tensor_value_info('X', TensorProto.FLOAT, [1, 1, 32, 32])

W_info = helper.make_tensor_value_info('W', TensorProto.FLOAT, [5, 1, 3, 3])
W = np.array([[[[-1,-1,-1],[-1,8.1,-1],[-1,-1,-1]]],[[[0,-1,0],[-1,5,-1],[0,-1,0]]],[[[-2,-1,0],[-1,1,1],[0,1,2]]],[[[1,2,1],[0,0,0],[-1,-2,-1]]],[[[1,0,-1],[2,0,-2],[1,0,-1]]]]).astype(np.float32)
W = numpy_helper.from_array(W, 'W')


# Create graph output Y
Y = helper.make_tensor_value_info('Y', TensorProto.FLOAT, [1, 5, 32, 32])

conv1 = helper.make_node(
        'Conv',         # name
        ['X', 'W'],     # inputs
        ['Y'],          # outputs
        ## Node Attributes
        kernel_shape=[3, 3],
        strides=[1, 1],
        pads=[1, 1, 1, 1],
        )


graph_def = helper.make_graph(
        nodes=[conv1],     # graph nodes
        name= 'conv_model',   # graph name
        inputs =      [X, W_info],  # graph inputs
        outputs =     [Y],          # graph outputs
        initializer = [W],
        )

model_def = helper.make_model(graph_def, producer_name='benchmarks')

onnx.checker.check_model(model_def)
model_def = shape_inference.infer_shapes(model_def)
onnx.checker.check_model(model_def)
model_def = optimizer.optimize(model_def)
onnx.checker.check_model(model_def)

onnx.save_model(model_def, 'e2_conv.onnx')
