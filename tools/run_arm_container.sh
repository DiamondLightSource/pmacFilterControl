#!/bin/bash

ARM_PMACFILTERCONTROL="/build/pmacFilterControl"
ARM_BUILD="${ARM_PMACFILTERCONTROL}/arm_build"
ARM_PREFIX="${ARM_PMACFILTERCONTROL}/arm_prefix"

CONTAINER_RUN="podman run --rm -it -v ${PMACFILTERCONTROL}:${ARM_PMACFILTERCONTROL} --privileged --net=host arm-crosscompiler"

if [ -z "$1" ]; then
  ${CONTAINER_RUN}
elif [ $1 == "bash" ]; then
  ${CONTAINER_RUN} bash
elif [ $1 == "build" ]; then
  ${CONTAINER_RUN} bash -c "\
mkdir -p ${ARM_BUILD} && rm -rf ${ARM_BUILD}/* && cd ${ARM_BUILD} && \
mkdir -p ${ARM_PREFIX} && rm -rf ${ARM_PREFIX}/* && \
cmake .. -DCMAKE_TOOLCHAIN_FILE=${ARM_PMACFILTERCONTROL}/cmake/arm-gcc-toolchain.cmake -DCMAKE_INSTALL_PREFIX=${ARM_PREFIX} -DZEROMQ_ROOTDIR=/build/libzmq/prefix/ && \
make VERBOSE=1 && make install"
elif [ $1 == "rebuild" ]; then
${CONTAINER_RUN} bash -c "\
cd ${ARM_BUILD} && make VERBOSE=1 && make install"
elif [ $1 == "pmacFilterControl" ]; then
  ${CONTAINER_RUN} qemu-arm -L /usr/arm-linux-gnueabihf/ ${ARM_PREFIX}/bin/pmacFilterControl 10001 127.0.0.1:10000
else
  echo "Unknown command: '$1'"
  exit 1
fi
