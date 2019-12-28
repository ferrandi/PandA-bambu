#!/bin/bash

wget -q https://s3.amazonaws.com/download.onnx/models/opset_9/bvlc_alexnet.tar.gz
tar -zxvf bvlc_alexnet.tar.gz
mv bvlc_alexnet/model.onnx i1_alexnet.onnx
rm -rf bvlc_alexnet bvlc_alexnet.tar.gz
