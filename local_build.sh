#!/bin/bash

SCRIPT_DIR=$(dirname -- "$( readlink -f -- "$0"; )")

COMMON_DIR=${SCRIPT_DIR}/builder
CXXFLAGS="-I${COMMON_DIR}/third-party -I${COMMON_DIR}"

echo Build common...
declare -a commons=("servercommon" "serialization")
declare -a common_objets
for common in "${commons[@]}"
do
    make -C ${COMMON_DIR} ${common}.o CXXFLAGS="${CXXFLAGS}" && (
        cp ${COMMON_DIR}/${common}.o ${SCRIPT_DIR}/back;
        cp ${COMMON_DIR}/${common}.o ${SCRIPT_DIR}/front
    ) && common_objets+=" ${common}.o"
done

echo Build back...
# from mysql_config
make -C ${SCRIPT_DIR}/back -j -k server CXXFLAGS="${CXXFLAGS}" LDFLAGS="${common_objets} -lmysqlclient -lz -lzstd -lssl -lcrypto -lresolv -lm"

echo Build front...
make -C ${SCRIPT_DIR}/front -j -k server CXXFLAGS="${CXXFLAGS}" LDFLAGS="${common_objets}"
