#!/bin/bash

function addpath
{
    local new_path=$1
    if ! (echo $PATH | grep $new_path: > /dev/null); then
        export PATH=$new_path:$PATH
    fi
}

if [ -z "$XDG_RUNTIME_DIR" ]; then
    export XDG_RUNTIME_DIR=/run/user/$(id -u)
fi

addpath $HOME/bin

export DOCKER_HOST=unix://$XDG_RUNTIME_DIR/docker.sock

if ! systemctl --user start docker.service; then
    error "Could not start docker service"
    exit 1
fi

