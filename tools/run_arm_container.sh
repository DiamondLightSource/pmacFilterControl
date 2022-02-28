#!/bin/bash

CONTAINER_RUN="podman run --rm -it -v ${PMACFILTERCONTROL_ROOT}/:/build/pmacFilterControl --privileged --net=host arm-crosscompiler"

if [ -z "$1" ]; then
  ${CONTAINER_RUN}
elif [ $1 == "bash" ]; then
  ${CONTAINER_RUN} bash
elif [ $1 == "build" ]; then
  ${CONTAINER_RUN} bash -c "mkdir -p /build/pmacFilterControl/arm_build && cd /build/pmacFilterControl/arm_build && mkdir -p /build/pmacFilterControl/arm_prefix && cmake .. -DCMAKE_TOOLCHAIN_FILE=/build/pmacFilterControl/cmake/arm-gcc-toolchain.cmake -DCMAKE_INSTALL_PREFIX=/build/pmacFilterControl/arm_prefix -DZEROMQ_ROOTDIR=/build/libzmq/prefix/ && make VERBOSE=1 && make install"
elif [ $1 == "pmacFilterControl" ]; then
  ${CONTAINER_RUN} qemu-arm -L /usr/arm-linux-gnueabihf/ /build/pmacFilterControl/arm_prefix/bin/pmacFilterControl 10001 127.0.0.1:10000
else
  echo "Unknown command: '$1'"
  exit 1
fi
