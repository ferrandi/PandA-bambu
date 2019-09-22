#!/bin/bash
opt-6.0 -scalarizer -scalarize-load-store ./06_softmax_a.ll -S -o ./06_softmax_a.scalarized.ll

