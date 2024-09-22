#!/bin/bash

SCRIPT_DIR=$(dirname -- "$( readlink -f -- "$0"; )")
SOURCE_DIR=${SCRIPT_DIR}/..

COMMON_DIR=${SOURCE_DIR}/builder
CXXFLAGS="-g -I${COMMON_DIR}/third-party -I${COMMON_DIR}"

echo Build common...
declare -a commons=("servercommon" "stringutils")
declare -a common_objets
for common in "${commons[@]}"
do
    make -C ${COMMON_DIR} ${common}.o CXXFLAGS="${CXXFLAGS}" && (
        cp -f ${COMMON_DIR}/${common}.o ${SOURCE_DIR}/back;
        cp -f ${COMMON_DIR}/${common}.o ${SOURCE_DIR}/front
    ) && common_objets+=" ${common}.o"
done

echo Build back...
# from mysql_config
make -C ${SOURCE_DIR}/back -j -k server CXXFLAGS="${CXXFLAGS}" LDFLAGS="${common_objets} -lmysqlclient -lz -lzstd -lssl -lcrypto -lresolv -lm"

echo Build front...
make -C ${SOURCE_DIR}/front -j -k server CXXFLAGS="${CXXFLAGS}" LDFLAGS="${common_objets} -lcrypto"
