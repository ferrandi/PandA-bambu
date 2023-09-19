#!/bin/bash

REGISTRY="bambuhls"
VARIANTS=("bionic" "focal")
IMAGES=("dev" "iverilog" "verilator")
VERSION="$(date +'%y.%m')"

DOCKER_BUILD="docker build --pull --quiet"

for base in ${VARIANTS[@]};
do
   for image in ${IMAGES[@]}
   do
      echo "Building image ${image} for Ubuntu ${base}"
      ${DOCKER_BUILD} -f Dockerfile.${image} -t ${REGISTRY}/${image}:${base}-${VERSION} --build-arg BASE=ubuntu:${base} . &
   done
done
echo "Building image cppcheck"
${DOCKER_BUILD} -f Dockerfile.cppcheck -t ${REGISTRY}/cppcheck:${VERSION} .
docker push ${REGISTRY}/cppcheck:${VERSION}
wait

echo "Building image yosys for Ubuntu focal"
${DOCKER_BUILD} -f Dockerfile.yosys -t ${REGISTRY}/yosys:focal-${VERSION} --build-arg BASE=ubuntu:focal . &

for base in ${VARIANTS[@]};
do
   for image in ${IMAGES[@]}
   do
      docker push ${REGISTRY}/${image}:${base}-${VERSION}
   done
done
wait

docker push --quiet ${REGISTRY}/yosys:focal-${VERSION} &
echo "Building image dev-yosys for Ubuntu focal"
${DOCKER_BUILD} -f Dockerfile.dev-yosys -t ${REGISTRY}/dev-yosys:focal-${VERSION} --build-arg VARIANT=focal-${VERSION} .
docker push ${REGISTRY}/dev-yosys:focal-${VERSION}
wait
