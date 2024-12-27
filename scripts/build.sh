#!/bin/bash

SCRIPT_DIR=$(dirname -- "$( readlink -f -- "$0"; )")
SOURCE_DIR=${SCRIPT_DIR}/..

COMMON_DIR=${SOURCE_DIR}/builder
CXXFLAGS="-g -DDEBUG_LOG -I${COMMON_DIR}/third-party -I${COMMON_DIR}"
LDFLAGS=""

echo Build common...
declare -a commons=($(find ${COMMON_DIR} -maxdepth 1 -type f -name "*.cpp" -exec basename {} .cpp \;))
declare -a common_objets
for common in "${commons[@]}"
do
    make -C "${COMMON_DIR}" ${common}.o CXXFLAGS="${CXXFLAGS}" && (
        ln -fs "${COMMON_DIR}/${common}.o" "${SOURCE_DIR}/back";
        ln -fs "${COMMON_DIR}/${common}.o" "${SOURCE_DIR}/front"
    ) && common_objets+=" ${common}.o"
done

# add sqlite3
SQLITE3_OBJ="sqlite3.o"
if [ ! -f "${SOURCE_DIR}/back/${SQLITE3_OBJ}" ]; then
    echo Build sqlite3...
    "${SCRIPT_DIR}/build_sqlite3.sh"
    ln -fs ${COMMON_DIR}/${SQLITE3_OBJ} "${SOURCE_DIR}/back"
fi

## add FFmpeg
ZLIB_DIR=${COMMON_DIR}/third-party/zlib
LIBPNG_DIR=${COMMON_DIR}/third-party/libpng
FFMPEG_DIR=${COMMON_DIR}/third-party/ffmpeg
FFMPEG_LIBS=" \
    -L${FFMPEG_DIR}/libavformat -lavformat \
    -L${FFMPEG_DIR}/libavcodec -lavcodec \
    -L${FFMPEG_DIR}/libavdevice -lavdevice \
    -L${FFMPEG_DIR}/libavfilter -lavfilter \
    -L${FFMPEG_DIR}/libswresample -lswresample \
    -L${FFMPEG_DIR}/libswscale -lswscale \
    -L${FFMPEG_DIR}/libavutil -lavutil \
    -L${LIBPNG_DIR}/.libs -lpng \
    -L${ZLIB_DIR} -lz \
"
if [ ! -f "${FFMPEG_DIR}/libavformat/libavformat.a" ]; then
    echo Build FFmpeg...
    "${SCRIPT_DIR}/build_ffmpeg.sh"
fi

echo Build back...
make -C ${SOURCE_DIR}/back -j -k server CXXFLAGS="${CXXFLAGS} -I${FFMPEG_DIR} -I${LIBPNG_DIR}" LDFLAGS="${LDFLAGS} ${common_objets} ${SQLITE3_OBJ} ${FFMPEG_LIBS}"

echo Build front...
make -C ${SOURCE_DIR}/front -j -k server CXXFLAGS="${CXXFLAGS}" LDFLAGS="${LDFLAGS} ${common_objets}"
