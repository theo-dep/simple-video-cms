#!/bin/bash

SCRIPT_DIR=$(dirname -- "$( readlink -f -- "$0"; )")
SOURCE_DIR=${SCRIPT_DIR}/..

COMMON_DIR=${SOURCE_DIR}/builder
CXXFLAGS="-g -DDEBUG_LOG -I${COMMON_DIR}/third-party -I${COMMON_DIR}"
LDFLAGS=""

echo Build common...
declare -a commons=($(find ${COMMON_DIR} -type f -name "*.cpp" -exec basename {} .cpp \;))
declare -a common_objets
for common in "${commons[@]}"
do
    make -C ${COMMON_DIR} ${common}.o CXXFLAGS="${CXXFLAGS}" && (
        cp -f ${COMMON_DIR}/${common}.o ${SOURCE_DIR}/back;
        cp -f ${COMMON_DIR}/${common}.o ${SOURCE_DIR}/front
    ) && common_objets+=" ${common}.o"
done

echo Build back...
FFMPEG_DIR=${COMMON_DIR}/third-party/ffmpeg
FFMPEG_LIBS=" \
    -L${FFMPEG_DIR}/libavformat -lavformat \
    -L${FFMPEG_DIR}/libavcodec -lavcodec \
    -L${FFMPEG_DIR}/libavdevice -lavdevice \
    -L${FFMPEG_DIR}/libavfilter -lavfilter \
    -L${FFMPEG_DIR}/libavutil -lavutil \
    -L${FFMPEG_DIR}/libswresample -lswresample \
    -L${FFMPEG_DIR}/libswscale -lswscale \
"
make -C ${SOURCE_DIR}/back -j -k server CXXFLAGS="${CXXFLAGS} -I${FFMPEG_DIR}" LDFLAGS="${LDFLAGS} ${common_objets} ${FFMPEG_LIBS}"

echo Build front...
make -C ${SOURCE_DIR}/front -j -k server CXXFLAGS="${CXXFLAGS}" LDFLAGS="${LDFLAGS} ${common_objets}"
