#!/bin/bash
opt-6.0 -scalarizer -scalarize-load-store ./e1_mlp.ll -S -o ./e1_mlp.scalarized.ll

