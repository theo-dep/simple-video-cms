FROM ubuntu:25.10

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        autoconf \
        autoconf-archive \
        automake \
        build-essential \
        ca-certificates \
        clang-21 \
        cmake \
        curl \
        git \
        libtool \
        nasm \
        ninja-build \
        pkg-config \
        python3 \
        tar \
        unzip \
        zip

RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-21 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-21 100
