#!/bin/bash

wget -q https://s3.amazonaws.com/download.onnx/models/opset_9/zfnet512.tar.gz 
tar -zxvf zfnet512.tar.gz
mv zfnet512/model.onnx i2_zfnet.onnx
rm -rf zfnet512.tar.gz zfnet512
