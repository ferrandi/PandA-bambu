#!/bin/bash
opt-6.0 -scalarizer -scalarize-load-store ./01_vecmul_a.ll -S -o ./01_vecmul_a.scalarized.ll

