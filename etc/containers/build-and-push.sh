#!/bin/bash

REGISTRY="bambuhls"
VARIANTS=("bionic" "focal")
IMAGES=("appimage" "iverilog" "verilator")

DOCKER_BUILD="docker build --pull --quiet"

for base in ${VARIANTS[@]};
do
   for image in ${IMAGES[@]}
   do
      echo "Building image ${image} for Ubuntu ${base}"
      ${DOCKER_BUILD} -f Dockerfile.${image} -t ${REGISTRY}/${image}:${base} --build-arg BASE=ubuntu:${base} . &
   done
done
echo "Building image cppcheck"
${DOCKER_BUILD} -f Dockerfile.cppcheck -t ${REGISTRY}/cppcheck:latest .
docker push ${REGISTRY}/cppcheck:latest
wait

echo "Building image yosys for Ubuntu focal"
${DOCKER_BUILD} -f Dockerfile.yosys -t ${REGISTRY}/yosys:focal --build-arg BASE=ubuntu:focal . &

for base in ${VARIANTS[@]};
do
   for image in ${IMAGES[@]}
   do
      docker push ${REGISTRY}/${image}:${base}
   done
done

wait
docker push --quiet ${REGISTRY}/yosys:focal &
echo "Building image appimage-yosys for Ubuntu focal"
${DOCKER_BUILD} -f Dockerfile.appimage-yosys -t ${REGISTRY}/appimage-yosys:focal --build-arg VARIANT=focal . &
docker push ${REGISTRY}/appimage-yosys:focal
wait
