#!/bin/bash
bambu proxies.c --top-fname=funcA "$@" |& tee log.txt
