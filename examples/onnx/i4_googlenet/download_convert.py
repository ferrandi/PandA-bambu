import torch

model = torch.hub.load('pytorch/vision:v0.4.2', 'googlenet', pretrained=True)

dummy_input = torch.randn(1,3,224,224)

torch.onnx.export(model, dummy_input, "i4_googlenet.onnx")

