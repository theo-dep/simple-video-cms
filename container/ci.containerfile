FROM ubuntu:26.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get -qq update && \
    apt-get install -y -qq --no-install-recommends \
        ca-certificates \
        gnupg

RUN gpg --homedir /tmp --no-default-keyring \
        --keyring gnupg-ring:/usr/share/keyrings/mono-official-archive-keyring.gpg \
        --keyserver hkp://keyserver.ubuntu.com:80 \
        --recv-keys 3FA7E0328081BFF6A14DA29AA6A19B38D3D831EF && \
    chmod +r /usr/share/keyrings/mono-official-archive-keyring.gpg && \
    echo "deb [signed-by=/usr/share/keyrings/mono-official-archive-keyring.gpg] https://download.mono-project.com/repo/ubuntu stable-focal main" | \
    tee /etc/apt/sources.list.d/mono-official-stable.list

RUN apt-get -qq update && \
    apt-get -qq dist-upgrade && \
    apt-get install -y -qq --no-install-recommends \
        autoconf \
        autoconf-archive \
        automake \
        build-essential \
        ca-certificates-mono \
        clang-22 \
        clang-tidy-22 \
        cmake \
        curl \
        gcovr \
        git \
        libclang-rt-22-dev \
        libtool \
        linux-libc-dev \
        llvm-22 \
        mono-devel \
        nasm \
        ninja-build \
        npm \
        pkg-config \
        python3 \
        tar \
        unzip \
        zip \
    && apt-get -qq clean

RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-22 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-22 100
    
RUN RT_DIR=/usr/lib/llvm-22/lib/clang/22/lib && \
    mkdir -p ${RT_DIR}/x86_64-pc-linux-gnu && \
    for f in ${RT_DIR}/linux/*-x86_64.*; do \
        base=$(basename "$f" | sed 's/-x86_64//'); \
        ln -sf "$f" "${RT_DIR}/x86_64-pc-linux-gnu/${base}"; \
    done

