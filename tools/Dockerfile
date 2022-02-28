FROM ubuntu:20.04

RUN apt-get update -y

# Install tzdata without user input
RUN DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get -y install tzdata

RUN apt-get install -y \
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
RUN apt-get install -y \
    bash \
    python \
    sed \
    vim \
    file \
    git \
    curl \
    wget

# Cross-compiler
RUN dpkg --add-architecture armel
RUN apt install -y crossbuild-essential-armel

# The cross-compiling emulator
RUN apt-get install -y \
    qemu-user \
    qemu-user-static

RUN mkdir /build && cd /build

RUN apt install -y g++-arm-linux-gnueabihf

# Build zeromq for ARM
RUN git clone https://github.com/zeromq/libzmq.git && cd libzmq && \
    ./autogen.sh && \
    mkdir prefix && \
    # http://wiki.zeromq.org/build:arm
    ./configure --host=arm-none-linux-gnueabihf CC=arm-linux-gnueabihf-gcc CXX=arm-linux-gnueabihf-g++ --prefix=/build/libzmq/prefix/ && \
    make -j8 && make install

RUN apt install -y cmake

# Make sure we can find standard lib and zeromq
ENV LD_LIBRARY_PATH=/usr/arm-linux-gnueabihf/lib:/build/libzmq/prefix/lib

WORKDIR /build

# Default command - Build and run hello world
CMD bash -c "cd /build/pmacFilterControl/tools && mkdir -p build && arm-linux-gnueabihf-g++ -o build/hello_arm hello.cpp && qemu-arm -L /usr/arm-linux-gnueabihf/ ./build/hello_arm"
