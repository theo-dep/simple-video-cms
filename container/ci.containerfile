FROM ubuntu:25.10

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get -qq update && \
    apt-get -qq dist-upgrade && \
    apt-get install -y -qq --no-install-recommends \
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
        linux-libc-dev \
        mono-complete \
        nasm \
        ninja-build \
        npm \
        pkg-config \
        python3 \
        tar \
        unzip \
        zip \
    && apt-get -qq clean

RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-21 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-21 100
