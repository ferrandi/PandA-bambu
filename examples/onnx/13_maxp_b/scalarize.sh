#!/bin/bash
opt-6.0 -scalarizer -scalarize-load-store ./13_maxp_b.ll -S -o ./13_maxp_b.scalarized.ll

