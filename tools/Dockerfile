FROM ubuntu:20.04

RUN apt update -y

# Install tzdata without user input
RUN DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt -y install tzdata

# General packages
RUN apt install -y \
    apt-utils \
    build-essential \
    automake \
    make \
    autogen \
    bison \
    flex \
    libtool \
    pkg-config \
    build-essential \
    bc \
    bzip2 \
    gzip \
    tar \
    xz-utils \
    ca-certificates \
    runit
# Split to avoid too many files error in build
RUN apt install -y \
    bash \
    python \
    sed \
    vim \
    file \
    git \
    curl \
    wget

# Cross-compiler and emulator
RUN dpkg --add-architecture armel
RUN apt install -y \
    crossbuild-essential-armel \
    g++-arm-linux-gnueabihf \
    qemu-user \
    qemu-user-static

# CMake to build pmacFilterControl
RUN apt install -y cmake

RUN mkdir /build && cd /build

# Build zeromq for ARM
RUN git clone https://github.com/zeromq/libzmq.git && cd libzmq && \
    ./autogen.sh && \
    mkdir prefix && \
    # http://wiki.zeromq.org/build:arm
    ./configure --host=arm-none-linux-gnueabihf CC=arm-linux-gnueabihf-gcc CXX=arm-linux-gnueabihf-g++ --prefix=/build/libzmq/prefix/ && \
    make -j8 && make install

# The built files will be copied to PowerPMAC:/root/lib
# Link ARM toolchain libs to /root/lib inside the container to build in the same location
RUN ln -s /usr/arm-linux-gnueabihf/lib /root/lib

# Make sure we can find standard lib, zeromq and pmacFilterControl libs
ENV LD_LIBRARY_PATH=/usr/arm-linux-gnueabihf/lib:/build/libzmq/prefix/lib:/build/pmacFilterControl/arm_prefix/lib

WORKDIR /build

# Default command - Build and run hello world
CMD bash -c "cd /build/pmacFilterControl/tools && mkdir -p build && arm-linux-gnueabihf-g++ -o build/hello_arm hello.cpp && qemu-arm -L /root/ ./build/hello_arm"
