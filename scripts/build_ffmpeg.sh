#!/bin/bash

SCRIPT_DIR=$(dirname -- "$( readlink -f -- "$0"; )")
SOURCE_DIR=${SCRIPT_DIR}/..

FFMPEG_DIR=${SOURCE_DIR}/builder/third-party/ffmpeg

echo Configure FFmpeg
pushd ${FFMPEG_DIR}
./configure \
    --enable-version3 \
    --disable-everything \
    --enable-decoder=h264 \
    --enable-parser=h264 \
    --enable-demuxer=mov,mp4,m4a,3gp,3g2,mj2 \
    --enable-swscale \
    --enable-avformat \
    --enable-avcodec \
    --enable-avutil \
    --disable-zlib \
    --disable-libdrm \
    --disable-programs \
    --disable-doc \
    --disable-network
popd

echo Build FFmpeg
make -C ${FFMPEG_DIR} -j -k
