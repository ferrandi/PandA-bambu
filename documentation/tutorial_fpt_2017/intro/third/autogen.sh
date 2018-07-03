#!/bin/bash
if ! test  -d m4; then
   mkdir m4
fi
autoreconf --force --install
