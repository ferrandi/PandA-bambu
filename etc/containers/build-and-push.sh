#!/bin/bash

REGISTRY="bambuhls"
VERSION="$(date +'%y.%m')"

function build_and_push() {
   target="${REGISTRY}/$1-${VERSION}"
   shift
   docker build --pull -t ${target} $@
   docker push ${target} &
}

echo "Build dev toolchain Ubuntu Xenial"
build_and_push dev:xenial -f Dockerfile --build-arg BASE=ubuntu:xenial --target dev .
build_and_push toolchain:xenial -f Dockerfile --build-arg BASE=ubuntu:xenial --target toolchain .
build_and_push base:xenial -f Dockerfile.base --build-arg BASE=ubuntu:xenial .

echo "Build dev toolchain Debian Bookworm"
build_and_push dev:bookworm -f Dockerfile --build-arg BASE=debian:bookworm-slim --target dev .
build_and_push toolchain:bookworm -f Dockerfile --build-arg BASE=debian:bookworm-slim --target toolchain .
build_and_push base:bookworm -f Dockerfile.base --build-arg BASE=debian:bookworm-slim .

echo "Build Verilator"
build_and_push verilator:focal -f Dockerfile.verilator --build-arg BASE=ubuntu:focal .
build_and_push verilator:bookworm -f Dockerfile.verilator --build-arg BASE=debian:bookworm-slim .

echo "Build CPP Check"
build_and_push codechecks:bookworm -f Dockerfile --build-arg BASE=debian:bookworm-slim --target codechecks .

echo "Build Yosys Ubuntu Focal"
build_and_push yosys:bullseye -f Dockerfile.yosys --build-arg BASE=debian:bullseye .

wait
