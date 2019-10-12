#!/bin/bash
opt-6.0 -scalarizer -scalarize-load-store ./02_vecmul_b.ll -S -o ./02_vecmul_b.scalarized.ll

