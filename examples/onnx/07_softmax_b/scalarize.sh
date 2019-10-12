#!/bin/bash
opt-6.0 -scalarizer -scalarize-load-store ./07_softmax_b.ll -S -o ./07_softmax_b.scalarized.ll

