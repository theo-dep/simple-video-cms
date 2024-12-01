#!/bin/bash

SCRIPT_DIR=$(dirname -- "$( readlink -f -- "$0"; )")
SOURCE_DIR=${SCRIPT_DIR}/..

ZLIB_DIR=${SOURCE_DIR}/builder/third-party/zlib

echo Configure zlib
pushd ${ZLIB_DIR}
./configure --static
popd

echo Build zlib
make -C ${ZLIB_DIR} -j -k

LIBPNG_DIR=${SOURCE_DIR}/builder/third-party/libpng

echo Configure libpng
pushd ${LIBPNG_DIR}
export LDFLAGS=-L${ZLIB_DIR}
./configure --disable-shared --enable-static
popd

echo Build libpng
make -C ${LIBPNG_DIR} -j -k

FFMPEG_DIR=${SOURCE_DIR}/builder/third-party/ffmpeg

echo Configure FFmpeg
pushd ${FFMPEG_DIR}
./configure \
    --enable-version3 \
    --enable-swscale \
    --enable-avformat \
    --enable-avcodec \
    --enable-avutil \
    --disable-protocols \
    --disable-zlib \
    --disable-bzlib \
    --disable-lzma \
    --disable-libdrm \
    --disable-programs \
    --disable-doc \
    --disable-network
popd

echo Build FFmpeg
make -C ${FFMPEG_DIR} -j -k
